#include "serial.h"
#include "cpu.h"
#include "interrupt.h"
#include "memory.h"
#include "page.h"
#include <cache.h>
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
	paging_init(memory_map, map_size, desc_size);

	page_alloc_init();
	cache_init();

	// acpi

	static ACPI_TABLE_DESC acpi_tables[128];
	extern ACPI_TABLE_RSDP *AcpiOsRsdp;
	AcpiOsRsdp = Rsdp;
	ACPI_STATUS status = AcpiInitializeTables(
		acpi_tables, sizeof(acpi_tables) / sizeof(acpi_tables[0]), false
	);
	if (ACPI_FAILURE(status)) {
		panic("acpi error %d\n", status);
	}
	serial_write_chars("\n", 1);

	ACPI_TABLE_MADT *Madt = NULL;
	AcpiGetTable(ACPI_SIG_MADT, 0, (ACPI_TABLE_HEADER**)&Madt);
	kprintf("lapic address: %#08x\n", Madt->Address);

	// apic

	// TODO: factor out temporary mappings
	extern uint64_t kernel_pml4[], pt_map[];
	pt_map[0] = (uint64_t)Madt->Address | PAGE_PRESENT | PAGE_WRITE | PAGE_LARGE | PAGE_GLOBAL;
	write_cr3((uint64_t)kernel_pml4 - KERNEL_BASE);

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

	// mask pic
	kprintf("madt flags: %x\n", Madt->Flags);
	if (Madt->Flags & ACPI_MADT_PCAT_COMPAT) {
		outb(0xa1, 0xff);
		outb(0x21, 0xff);
	}

	__asm__ volatile ("sti");
	for (;;) {
		char c = serial_read(COM1);
		serial_write(COM1, c);
	}
}
