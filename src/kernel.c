#include "serial.h"
#include "memory.h"
#include "interrupt.h"
#include "io.h"
#include <paging.h>
#include <kprintf.h>
#include <efi.h>
#include <acpi/acpi.h>
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

void kernel_init(
	struct efi_memory_descriptor *memory_map, size_t map_size, size_t desc_size,
	ACPI_TABLE_RSDP *Rsdp
) {
	interrupt_init();

	serial_init(COM1);

	// physical memory manager

	// TODO: factor out temporary mappings
	extern uint64_t pml4[], pd_map[];
	for (unsigned int i = 0; i < (map_size + PD_SIZE) / PD_SIZE; i++) {
		uint64_t address = ((uint64_t)memory_map & PD_MASK) + i * PD_SIZE;
		pd_map[1 + i] = address | PAGE_PRESENT | PAGE_WRITE | PAGE_LARGE;
	}
	write_cr3((uint64_t)pml4 - KERNEL_BASE);
	memory_map = (struct efi_memory_descriptor*)(
		0xffffffffc0200000 + ((uint64_t)memory_map & ~PD_MASK)
	);

	// transfer efi memory map to our memory map
	void *map = memory_map;
	for (char *p = map, *end = (char*)map + map_size; p < end; p += desc_size) {
		struct efi_memory_descriptor *mem = (void*)p;

		static const char *efi_memory_name[] = {
			[efi_reserved] = "reserved",
			[efi_loader_code] = "loader code",
			[efi_loader_data] = "loader data",
			[efi_boot_code] = "boot code",
			[efi_boot_data] = "boot data",
			[efi_runtime_code] = "runtime code",
			[efi_runtime_data] = "runtime data",
			[efi_conventional] = "free ram",
			[efi_unusable] = "unusable",
			[efi_acpi_reclaim] = "acpi reclaimable",
			[efi_acpi_nvs] = "acpi nvs",
			[efi_memory_mapped_io] = "mmio",
			[efi_memory_mapped_port] = "mm port",
			[efi_pal] = "pal",
		};

		uint64_t size = mem->pages * 0x1000;

		kprintf(
			"efi memory: %#016lx-%#016lx %016lx %s\n",
			mem->physical, mem->physical + size,
			mem->flags, efi_memory_name[mem->type]
		);

		if (mem->flags | efi_memory_runtime) {
			// TODO: remap efi runtime memory and call SetVirtualAddressMap
		}

		memory_add(mem->physical, size);

		if (
			mem->type != efi_conventional &&
			mem->type != efi_loader_code &&
			mem->type != efi_loader_data &&
			mem->type != efi_boot_code &&
			mem->type != efi_boot_data
		)
			memory_reserve(mem->physical, size);
	}

	// reserve kernel image
	extern char kernel_start[], kernel_end[];
	memory_reserve((uint64_t)kernel_start - KERNEL_BASE, kernel_end - kernel_start);

	// direct mapping of ram
	uint64_t i = 0, start_frame, end_frame;
	while (memory_pages_next(&i, &start_frame, &end_frame), i != (uint64_t)-1) {
		uint64_t start_phys = start_frame << PAGE_SHIFT;
		uint64_t end_phys = end_frame << PAGE_SHIFT;
		kprintf("mem [%#010lx-%#010lx]\n", start_phys, end_phys - 1);

		direct_map(start_phys, end_phys);
	}

	// TODO: completely clean up virtual address space
	pml4[0] = 0;
	write_cr3((uint64_t)pml4 - KERNEL_BASE);

	// acpi

	static ACPI_TABLE_DESC acpi_tables[128];
	extern ACPI_TABLE_RSDP *AcpiOsRsdp;
	AcpiOsRsdp = Rsdp;
	ACPI_STATUS status = AcpiInitializeTables(
		acpi_tables, sizeof(acpi_tables) / sizeof(acpi_tables[0]), false
	);
	if (ACPI_FAILURE(status)) {
		kprintf("acpi error %d\n", status);
		for (;;);
	}
	serial_write_chars("\n", 1);

	ACPI_TABLE_MADT *Madt = NULL;
	AcpiGetTable(ACPI_SIG_MADT, 0, (ACPI_TABLE_HEADER**)&Madt);
	kprintf("lapic address: %#08x\n", Madt->Address);

	// apic

	// TODO: factor out temporary mappings
	pd_map[0] = (uint64_t)Madt->Address | PAGE_PRESENT | PAGE_WRITE | PAGE_LARGE | PAGE_GLOBAL;
	write_cr3((uint64_t)pml4 - KERNEL_BASE);

	// TODO: factor out apic (also see entry.S:default_interrupt for EOI)

	uint32_t lapic_id = *(uint32_t*)0xffffffffc0000020;
	uint32_t lapic_version = *(uint32_t*)0xffffffffc0000030;
	kprintf("id: %d\nversion: %#x\n\n", lapic_id, lapic_version);

	uint32_t eax, edx;
	rdmsr(0x1b, &eax, &edx);
	wrmsr(0x1b, (eax & 0xfffff100) | 0x800, 0);
	*(uint32_t*)0xffffffffc00000f0 = *(uint32_t*)0xffffffffc00000f0 | 0x100;

	*(uint32_t*)0xffffffffc0000320 = 0x00020030;
	*(uint32_t*)0xffffffffc0000380 = 30000000;
	__asm__ volatile ("sti");

	// mask pic
	kprintf("madt flags: %x\n", Madt->Flags);
	if (Madt->Flags & ACPI_MADT_PCAT_COMPAT) {
		outb(0xa1, 0xff);
		outb(0x21, 0xff);
	}

	for (;;);
}
