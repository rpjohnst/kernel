#include <stdint.h>

#define SMP_PERCPU __attribute__((section(".percpu")))

#define SMP_PERCPU_READ(sym) __extension__ ({ \
	__typeof__(sym) out; \
	__asm__ volatile ("mov %%gs:%1, %0" : "=r"(out) : "m"(sym)); \
	out; \
})

#define SMP_PERCPU_WRITE(sym, val) \
	__asm__ volatile ("mov %1, %%gs:%0" : "=m"(sym) : "ir"(val))

#define SMP_PERCPU_SYM(cpu, sym) \
	*(__typeof__(sym)*)((char*)percpu_data[cpu] + (uintptr_t)&(sym))

void smp_init(void);

extern uint8_t lapic_by_cpu[256];
extern void *percpu_data[256];
