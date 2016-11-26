#include <stdint.h>

// io ports

static inline void outb(uint16_t port, uint8_t value) {
	__asm__ volatile ("outb %0, %1" :: "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
	uint8_t result;
	__asm__ volatile ("inb %1, %0" : "=a"(result) : "Nd"(port));
	return result;
}

// cpuid

static inline void cpuid(uint32_t i, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx) {
	__asm__ volatile ("cpuid" : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx) : "a"(i));
}

enum cpuid_flags {
	cpuid_01_ecx_x2apic = 1 << 21,
};

// msrs

enum msr {
	ia32_apic_base = 0x1b,
	x2apic_base = 0x800,
};

enum msr_flags {
	apic_x2apic_enable = 1 << 10,
	apic_global_enable = 1 << 11,
};

static inline void rdmsr(uint32_t msr, uint32_t *low, uint32_t *high) {
	__asm__ volatile ("rdmsr" : "=a"(*low), "=d"(*high) : "c"(msr));
}

static inline void wrmsr(uint32_t msr, uint32_t low, uint32_t high) {
	__asm__ volatile ("wrmsr" :: "a"(low), "d"(high), "c"(msr));
}
