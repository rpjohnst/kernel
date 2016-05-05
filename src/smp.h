#include <stdint.h>

#define SMP_PERCPU __attribute__((section(".percpu")))

void smp_init(void);

extern uint8_t lapic_by_cpu[256];
extern void *percpu_data[256];
