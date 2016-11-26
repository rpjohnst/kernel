#include "pci.h"
#include "smp.h"
#include "tsc.h"
#include "hpet.h"
#include "apic.h"
#include "acpi/parse.h"
#include "serial.h"
#include "cpu.h"
#include "interrupt.h"
#include "page.h"
#include <cache.h>
#include <paging.h>
#include <kprintf.h>
#include <assert.h>
#include <stdint.h>
#include <stddef.h>

void kernel_init(void *memory_map, size_t map_size, size_t desc_size, void *Rsdp) {
	interrupt_init();
	serial_init(COM1);
	paging_init(memory_map, map_size, desc_size);

	acpi_parse(Rsdp);

	hpet_enable();
	apic_timer_calibrate();
	tsc_calibrate();

	smp_init();

	page_alloc_init();
	cache_init();

	pci_enumerate();

#if 0
	ACPI_STATUS status = AcpiInitializeSubsystem();
	if (ACPI_FAILURE(status))
		panic("acpi error %d\n", status);
#endif

	// clear identity mapping
	extern uint64_t kernel_pml4[];
	kernel_pml4[0] = 0;
	write_cr3(PHYS_KERNEL(kernel_pml4));

	__asm__ volatile ("sti");
	while (true) __asm__ ("hlt");
}
