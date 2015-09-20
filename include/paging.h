#define DIRECT_BASE 0xffff800000000000 
#define VIRT_DIRECT(phys) ((void*)(DIRECT_BASE + (phys)))
#define PHYS_DIRECT(virt) ((uint64_t)(virt) - DIRECT_BASE)

#define KERNEL_BASE 0xffffffff80000000
#define VIRT_KERNEL(phys) ((void*)(KERNEL_BASE + (phys)))
#define PHYS_KERNEL(phys) ((uint64_t)(phys) - KERNEL_BASE)

#define PAGE_ENTRIES 512

#define PAGE_SHIFT 12
#define PAGE_SIZE (1UL << PAGE_SHIFT)
#define PAGE_MASK (~(PAGE_SIZE - 1))
#define PAGE_INDEX(virt) (((virt) >> PAGE_SHIFT) & (PAGE_ENTRIES - 1))

#define PD_SHIFT 21
#define PD_SIZE (1UL << PD_SHIFT)
#define PD_MASK (~(PD_SIZE - 1))
#define PD_INDEX(virt) (((virt) >> PD_SHIFT) & (PAGE_ENTRIES - 1))

#define PDPT_SHIFT 30
#define PDPT_SIZE (1UL << PDPT_SHIFT)
#define PDPT_MASK (~(PDPT_SIZE - 1))
#define PDPT_INDEX(virt) (((virt) >> PDPT_SHIFT) & (PAGE_ENTRIES - 1))

#define PML4_SHIFT 39
#define PML4_SIZE (1UL << PML4_SHIFT)
#define PML4_MASK (~(PML4_SIZE - 1))
#define PML4_INDEX(virt) (((virt) >> PML4_SHIFT) & (PAGE_ENTRIES - 1))

#define PAGE_PRESENT (1 << 0)
#define PAGE_WRITE (1 << 1)
#define PAGE_USER (1 << 2)
#define PAGE_CACHE_WT (1 << 3)
#define PAGE_CACHE_UC (1 << 4)
#define PAGE_ACCESSED (1 << 5)
#define PAGE_DIRTY (1 << 6)
#define PAGE_LARGE (1 << 7)
#define PAGE_GLOBAL (1 << 8)
#define PAGE_NX (1 << 63)

#ifndef __ASSEMBLY__

#define round_up(x, y) ((((x) - 1) | ((__typeof__(x))((y) - 1))) + 1)
#define round_down(x, y) ((x) & ~((__typeof__(x))((y) - 1)))

#include <stdint.h>
#include <stddef.h>

void paging_init(void *map_address, size_t map_size, size_t desc_size);

static inline void write_cr3(uint64_t cr3) {
	__asm__ volatile ("mov %0, %%cr3" :: "r"(cr3));
}

#endif
