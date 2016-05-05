#include <stdint.h>

void tsc_calibrate(void);

void tsc_udelay(uint64_t usecs);

extern uint32_t tsc_frequency;

static inline uint64_t rdtsc(void) {
	uint64_t result[2];
	__asm__ volatile ("rdtsc" : "=a"(result[0]), "=d"(result[1]));
	return (result[1] << 32) | result[0];
}
