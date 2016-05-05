#include "tsc.h"
#include "hpet.h"
#include "cpu.h"
#include <assert.h>
#include <kprintf.h>

uint32_t tsc_frequency;

// TODO: cpuid flag constants
void tsc_calibrate(void) {
	uint32_t eax, ebx, ecx, edx;

	cpuid(0x80000007, &eax, &ebx, &ecx, &edx);
	if (!(edx & (1 << 8))) {
		panic("invariant tsc unavailable");
	}

	cpuid(0x01, &eax, &ebx, &ecx, &edx);
	if (!(ecx & (1 << 24))) {
		panic("tsc deadline unavailable");
	}

	// TODO: move femtoseconds constant
	uint32_t wait_time = 1000000000000000ULL / hpet_period() / 100;

	uint64_t hpet_start = hpet_now();
	uint64_t tsc_start = rdtsc();

	while (hpet_now() - hpet_start < wait_time) continue;

	uint32_t tsc_end = rdtsc();

	tsc_frequency = (tsc_end - tsc_start) * 100;

	kprintf("tsc: %u.%06uMHz\n", tsc_frequency / 1000000, tsc_frequency % 1000000);
}

void tsc_udelay(uint64_t usecs) {
	uint64_t wait_time = tsc_frequency / 1000000 * usecs;

	uint64_t start_time = rdtsc();
	while (rdtsc() - start_time < wait_time) continue;

	// TODO: handle accidental core switches
}
