#include <stdint.h>
#include <stdbool.h>

void apic_init(uint32_t lapic_address, bool legacy_pic);
void apic_timer_calibrate(void);

enum apic_register {
	apic_id = 0x02,
	apic_version = 0x03,

	apic_tpr = 0x08,
	apic_apr = 0x09,
	apic_ppr = 0x0a,

	apic_eoi = 0x0b,

	apic_rrr = 0x0c,
	apic_ldr = 0x0d,
	apic_dfr = 0x0e,

	apic_spurious = 0x0f,

	apic_isr = 0x10,
	apic_tmr = 0x18,
	apic_irr = 0x21,

	apic_esr = 0x28,

	apic_icr_low = 0x30,
	apic_icr_high = 0x31,

	apic_lvt_timer = 0x32,
	apic_lvt_thermal = 0x33,
	apic_lvt_perf = 0x34,
	apic_lvt_lint0 = 0x35,
	apic_lvt_lint1 = 0x36,
	apic_lvt_err = 0x37,

	apic_timer_init = 0x38,
	apic_timer_current = 0x39,
	apic_timer_divide = 0x3e,
};
static const int apic_reg = 0x10 >> 2;

enum apic_icr_flags {
	apic_icr_dest_shift = 24,

	apic_icr_shorthand_none = 0 << 18,
	apic_icr_shorthand_self = 1 << 18,
	apic_icr_shorthand_all = 2 << 18,
	apic_icr_shorthand_others = 3 << 18,

	apic_icr_edge = 0 << 15,
	apic_icr_level = 1 << 15,

	apic_icr_deassert = 0 << 14,
	apic_icr_assert = 1 << 14,

	apic_icr_idle = 0 << 12,
	apic_icr_pending = 1 << 12,

	apic_icr_physical = 0 << 11,
	apic_icr_logical = 1 << 11,

	apic_icr_fixed = 0 << 8,
	apic_icr_lowest = 1 << 8,
	apic_icr_smi = 2 << 8,
	apic_icr_nmi = 4 << 8,
	apic_icr_init = 5 << 8,
	apic_icr_startup = 6 << 8,
};

struct apic {
	const char *name;

	uint32_t (*read)(uint32_t reg);
	void (*write)(uint32_t reg, uint32_t val);

	void (*icr_write)(uint32_t high, uint32_t low);
	bool (*icr_wait_idle)(uint32_t msecs);
};

extern struct apic *apic;

static inline uint32_t apic_read(uint32_t reg) {
	return apic->read(reg);
}

static inline void apic_write(uint32_t reg, uint32_t val) {
	apic->write(reg, val);
}

static inline void apic_icr_write(uint32_t high, uint32_t low) {
	apic->icr_write(high, low);
}

static inline bool apic_icr_wait_idle(uint32_t msecs) {
	return apic->icr_wait_idle(msecs);
}

static inline uint32_t apic_esr_read(void) {
	apic->write(apic_esr, 0);
	return apic->read(apic_esr) & 0xef;
}

extern uint32_t lapic_count;
