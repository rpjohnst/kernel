#include "tsc.h"
#include "apic.h"
#include "hpet.h"
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
#include <stdbool.h>

void kernel_init(void *memory_map, size_t map_size, size_t desc_size, void *Rsdp) {
	interrupt_init();
	serial_init(COM1);
	paging_init(memory_map, map_size, desc_size);

	page_alloc_init();
	cache_init();

	acpi_parse(Rsdp);

	hpet_enable();
	apic_timer_calibrate();
	tsc_calibrate();


	__asm__ volatile ("sti");
	while (true) __asm__ ("hlt");
}
