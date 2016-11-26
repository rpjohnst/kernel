#include "hpet.h"
#include "cpu.h"
#include <paging.h>
#include <kprintf.h>

struct hpet {
	uint64_t capabilities;
	uint64_t reserved0;
	uint64_t config;
	uint64_t reserved1;
	uint64_t isr;
	uint64_t reserved2[25];
	uint64_t counter;
	uint64_t reserved3;

	struct hpet_comparator {
		uint64_t conf;
		uint64_t value;
		uint64_t route;
		uint64_t reserved;
	} timers[];
};

enum hpet_capabilites_fields {
	hpet_timers_shift = 8,
	hpet_timers_mask = 0xf,

	hpet_period_shift = 32,
};

enum hpet_config_flags {
	hpet_config_enable = 1 << 0,
};

enum hpet_timer_flags {
	hpet_timer_periodic = 1 << 4,
	hpet_timer_routes_shift = 32,
};

static volatile struct hpet *hpet;

void hpet_init(uint64_t hpet_address) {
	// TODO: factor out temporary mappings
	extern uint64_t kernel_pml4[], pt_map[];
	pt_map[1] = hpet_address | PAGE_PRESENT | PAGE_WRITE | PAGE_CACHE_UC | PAGE_GLOBAL;
	write_cr3(PHYS_KERNEL(kernel_pml4));
	hpet = (volatile struct hpet*)0xffffffffc0001000;

	uint8_t max_timer = (hpet->capabilities >> hpet_timers_shift) & hpet_timers_mask;

	uint32_t period = hpet->capabilities >> hpet_period_shift;
	uint32_t frequency = 1000000000000000ULL / period;

	kprintf(
		"hpet: %#015lx - %d comparators @ %u.%06uMHz\n",
		hpet_address, max_timer + 1,
		frequency / 1000000, frequency % 1000000
	);

	volatile struct hpet_comparator *timers = &hpet->timers[0];
	for (int i = 0; i <= max_timer; i++) {
		kprintf(
			" comparator %d: %4.4speriodic (irqs %08x)\n", i,
			(uint32_t)(timers[i].conf & hpet_timer_periodic) ? "" : "non-",
			(uint32_t)(timers[i].conf >> hpet_timer_routes_shift)
		);
	}
}

void hpet_enable(void) {
	hpet->config |= hpet_config_enable;
}

uint64_t hpet_now(void) {
	return hpet->counter;
}

uint64_t hpet_period(void) {
	return hpet->capabilities >> hpet_period_shift;
}
