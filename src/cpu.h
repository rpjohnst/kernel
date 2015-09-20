// io ports

static inline void outb(uint16_t port, uint8_t value) {
	__asm__ volatile ("outb %0, %1" :: "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
	uint8_t result;
	__asm__ volatile ("inb %1, %0" : "=a"(result) : "Nd"(port));
	return result;
}

// msrs

enum msr {
	ia32_apic_base = 0x1b,
};

enum msr_flags {
	apic_global_enable = 1 << 11,
};

static inline void rdmsr(uint32_t msr, uint32_t *low, uint32_t *high) {
	__asm__ volatile ("rdmsr" : "=a"(*low), "=d"(*high) : "c"(msr));
}

static inline void wrmsr(uint32_t msr, uint32_t low, uint32_t high) {
	__asm__ volatile ("wrmsr" :: "a"(low), "d"(high), "c"(msr));
}
