#include "smp.h"
#include <stdint.h>
#include <stdbool.h>

void apic_init(uint32_t lapic_address, bool legacy_pic);
void apic_timer_calibrate(void);

enum apic_register {
	apic_id = 0x20 >> 2,
	apic_version = 0x030 >> 2,

	apic_tpr = 0x080 >> 2,
	apic_apr = 0x090 >> 2,
	apic_ppr = 0x0a0 >> 2,

	apic_eoi = 0x0b0 >> 2,

	apic_rrr = 0x0c0 >> 2,
	apic_ldr = 0x0d0 >> 2,
	apic_dfr = 0x0e0 >> 2,

	apic_spurious = 0x0f0 >> 2,

	apic_isr = 0x100 >> 2,
	apic_tmr = 0x180 >> 2,
	apic_irr = 0x210 >> 2,

	apic_esr = 0x280 >> 2,

	apic_icr_low = 0x300 >> 2,
	apic_icr_high = 0x310 >> 2,

	apic_lvt_timer = 0x320 >> 2,
	apic_lvt_thermal = 0x330 >> 2,
	apic_lvt_perf = 0x340 >> 2,
	apic_lvt_lint0 = 0x350 >> 2,
	apic_lvt_lint1 = 0x360 >> 2,
	apic_lvt_err = 0x370 >> 2,

	apic_timer_init = 0x380 >> 2,
	apic_timer_current = 0x390 >> 2,
	apic_timer_divide = 0x3e0 >> 2,
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

uint32_t apic_read(uint32_t reg);

void apic_icr_write(uint32_t high, uint32_t low);
bool apic_icr_wait_idle(uint32_t msecs);

uint32_t apic_esr_read(void);

extern uint32_t lapic_count;
extern SMP_PERCPU uint32_t lapic_id;
