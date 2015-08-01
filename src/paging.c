#include "memory.h"
#include <paging.h>
#include <kprintf.h>
#include <string.h>
#include <stdint.h>

// defined in startup.S
// TODO: consider renaming to kernel_pml4 or something
extern uint64_t pml4[PAGE_ENTRIES];

#define round_up(x, y) ((((x) - 1) | ((__typeof__(x))((y) - 1))) + 1)
#define round_down(x, y) ((x) & ~((__typeof__(x))((y) - 1)))

struct range {
	uint64_t start;
	uint64_t end;
	uint8_t level;
};

#define RANGE(start, end, level) ((struct range){ (start), (end), (level) })

static int split_range(struct range ranges[5], uint64_t start, uint64_t end) {
	int range = 0;

	uint64_t start_frame = start >> PAGE_SHIFT;
	uint64_t end_frame = end >> PAGE_SHIFT;

	uint64_t first_frame, last_frame;

	// 4k pages from start to 2M-aligned or end
	first_frame = start_frame;
	last_frame = round_up(start_frame, PD_SIZE >> PAGE_SHIFT);
	if (last_frame > end_frame)
		last_frame = end_frame;
	if (first_frame < last_frame) {
		ranges[range++] = RANGE(first_frame << PAGE_SHIFT, last_frame << PAGE_SHIFT, 0);
		start_frame = last_frame;
	}

	// 2M pages from 2M-aligned to 1G-aligned or 2M-aligned end
	first_frame = round_up(start_frame, PD_SIZE >> PAGE_SHIFT);
	last_frame = round_up(start_frame, PDPT_SIZE >> PAGE_SHIFT);
	if (last_frame > round_down(end_frame, PD_SIZE >> PAGE_SHIFT))
		last_frame = round_down(end_frame, PD_SIZE >> PAGE_SHIFT);
	if (first_frame < last_frame) {
		ranges[range++] = RANGE(first_frame << PAGE_SHIFT, last_frame << PAGE_SHIFT, 1);
		start_frame = last_frame;
	}

	// 1G pages from 1G-aligned to 1G-aligned end
	first_frame = round_up(start_frame, PDPT_SIZE >> PAGE_SHIFT);
	last_frame = round_down(end_frame, PDPT_SIZE >> PAGE_SHIFT);
	if (first_frame < last_frame) {
		ranges[range++] = RANGE(first_frame << PAGE_SHIFT, last_frame << PAGE_SHIFT, 2);
		start_frame = last_frame;
	}

	// 2M pages to 2M-aligned end
	first_frame = round_up(start_frame, PD_SIZE >> PAGE_SHIFT);
	last_frame = round_down(end_frame, PD_SIZE >> PAGE_SHIFT);
	if (first_frame < last_frame) {
		ranges[range++] = RANGE(first_frame << PAGE_SHIFT, last_frame << PAGE_SHIFT, 1);
		start_frame = last_frame;
	}

	// 4k pages to end
	first_frame = start_frame;
	last_frame = end_frame;
	if (first_frame < last_frame) {
		ranges[range++] = RANGE(first_frame << PAGE_SHIFT, last_frame << PAGE_SHIFT, 0);
	}

	// merge any ranges of the same level that crossed a higher alignment boundary
	for (int i = 0; i < range - 1; i++) {
		if (ranges[i].level != ranges[i + 1].level)
			continue;

		uint64_t start = ranges[i].start;
		memcpy(&ranges[i], &ranges[i + 1], (range - 1 - i) * sizeof(*ranges));
		ranges[i--].start = start;
		range--;
	}

	static const char *const sizes[] = { "4k", "2M", "1G" };
	for (int i = 0; i < range; i++) {
		kprintf(
			" [%#013lx-%#013lx) %s\n", ranges[i].start, ranges[i].end, sizes[ranges[i].level]
		);
	}

	return range;
}

// reserve 2 page tables to get started
// TODO: how many do we really need, given the boostrap direct mapping?
static uint64_t init_page_tables[2][PAGE_ENTRIES] __attribute__((aligned(PAGE_SIZE)));
static unsigned int init_page_table = 0;

static uint64_t mapped_top = 0;

static void *alloc_page_direct() {
	if (init_page_table < sizeof(init_page_tables) / sizeof(*init_page_tables))
		return VIRT_DIRECT(PHYS_KERNEL(init_page_tables[init_page_table++]));

	// TODO: what to do about the bottom page?
	void *page = memory_alloc(PAGE_SIZE, mapped_top, PAGE_SIZE, PAGE_SIZE);

	kprintf(" page table %#013lx\n", PHYS_DIRECT(page));

	return page;
}

static void direct_map_pt(uint64_t *pt, uint64_t start_phys, uint64_t end_phys, uint8_t level) {
	for (uint64_t phys = start_phys; phys < end_phys; phys = (phys & PAGE_MASK) + PAGE_SIZE) {
		uint64_t pte = PAGE_INDEX(phys);
		if (pte >= PAGE_ENTRIES)
			break;

		pt[pte] = phys | PAGE_PRESENT | PAGE_WRITE | PAGE_GLOBAL;
	}
}

static void direct_map_pd(uint64_t *pd, uint64_t start_phys, uint64_t end_phys, uint8_t level) {
	for (uint64_t phys = start_phys; phys < end_phys; phys = (phys & PD_MASK) + PD_SIZE) {
		uint64_t pde = PD_INDEX(phys);
		if (pde >= PAGE_ENTRIES)
			break;

		if (level == 1) {
			pd[pde] = phys | PAGE_PRESENT | PAGE_WRITE | PAGE_LARGE | PAGE_GLOBAL;
			continue;
		}

		uint64_t *pt;
		if (pd[pde] == 0) {
			pt = alloc_page_direct();
			pd[pde] = PHYS_DIRECT(pt) | PAGE_PRESENT | PAGE_WRITE | PAGE_GLOBAL;
		}
		else {
			pt = VIRT_DIRECT(pd[pde] & PAGE_MASK);
		}

		direct_map_pt(pt, phys, end_phys, level);
	}
}

static void direct_map_pdpt(uint64_t *pdpt, uint64_t start_phys, uint64_t end_phys, uint8_t level) {
	for (uint64_t phys = start_phys; phys < end_phys; phys = (phys & PDPT_MASK) + PDPT_SIZE) {
		uint64_t pdpte = PDPT_INDEX(phys);
		if (pdpte >= PAGE_ENTRIES)
			break;

		if (level == 2) {
			pdpt[pdpte] = phys | PAGE_PRESENT | PAGE_WRITE | PAGE_LARGE | PAGE_GLOBAL;
			continue;
		}

		uint64_t *pd;
		if (pdpt[pdpte] == 0) {
			pd = alloc_page_direct();
			pdpt[pdpte] = PHYS_DIRECT(pd) | PAGE_PRESENT | PAGE_WRITE | PAGE_GLOBAL;
		}
		else {
			pd = VIRT_DIRECT(pdpt[pdpte] & PAGE_MASK);
		}

		direct_map_pd(pd, phys, end_phys, level);
	}
}

static void direct_map_pml4(uint64_t start_phys, uint64_t end_phys, uint8_t level) {
	uint64_t start_virt = (uint64_t)VIRT_DIRECT(start_phys);
	uint64_t end_virt = (uint64_t)VIRT_DIRECT(end_phys);

	for (uint64_t virt = start_virt; virt < end_virt; virt = (virt & PML4_MASK) + PML4_SIZE) {
		uint64_t pml4e = PML4_INDEX(virt);

		uint64_t *pdpt;
		if (pml4[pml4e] == 0) {
			pdpt = alloc_page_direct();
			pml4[pml4e] = PHYS_DIRECT(pdpt) | PAGE_PRESENT | PAGE_WRITE | PAGE_GLOBAL;
		}
		else {
			// initial page tables are all already in the direct map (see startup.S)
			pdpt = VIRT_DIRECT(pml4[pml4e] & PAGE_MASK);
		}

		direct_map_pdpt(pdpt, PHYS_DIRECT(virt), PHYS_DIRECT(end_virt), level);
	}

	write_cr3(PHYS_KERNEL(pml4));
	mapped_top = end_phys;
}

// TODO: unmap memory in the first GB that's hard-coded in startup.S
void direct_map(uint64_t start_phys, uint64_t end_phys) {
	kprintf("mem [%#013lx-%#013lx)\n", start_phys, end_phys);

	struct range ranges[5];
	int n = split_range(ranges, start_phys, end_phys);

	for (int i = 0; i < n; i++)
		direct_map_pml4(ranges[i].start, ranges[i].end, ranges[i].level);
}
