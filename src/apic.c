#include "apic.h"
#include "cpu.h"
#include "hpet.h"
#include <paging.h>
#include <kprintf.h>

enum apic_registers {
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

	apic_ecr = 0x280 >> 2,

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

enum apic_spurious_flags {
	apic_sw_enable = 1 << 8,
	apic_focus_check = 1 << 9,
	acpi_eoi_supression = 1 << 12,
};

enum apic_lvt_flags {
	apic_lvt_status = 1 << 12,
	apic_lvt_mask = 1 << 16,

	apic_lvt_fixed = 0x0 << 8,
	apic_lvt_smi = 0x2 << 8,
	apic_lvt_nmi = 0x4 << 8,
	apic_lvt_extint = 0x7 << 8,
	apic_lvt_init = 0x5 << 8,

	apic_timer_oneshot = 0x0 << 17,
	apic_timer_periodic = 0x1 << 17,
	apic_timer_tsc = 0x2 << 17,
};

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

static void pic_disable(void) {
	kprintf("apic: disabling legacy pic\n");

	enum pic_port {
		pic_master_cmd = 0x20,
		pic_master_imr = 0x21,

		pic_slave_cmd = 0xa0,
		pic_slave_imr = 0xa1,
	};

	enum pic_command {
		pic_icw1_init = 0x10,
		pic_icw1_icw4 = 0x01,
		pic_icw4_default = 0x01,
	};

	// remap PIC vectors to [0x20,0x30) so they don't overlap CPU traps
	// this is used only for spurious interrupts

	outb(pic_master_cmd, pic_icw1_init + pic_icw1_icw4);
	outb(pic_master_imr, 0x20); // pic 1 offset
	outb(pic_master_imr, 1 << 2); // pic 2 cascade line
	outb(pic_master_imr, pic_icw4_default); // no auto-eoi

	outb(pic_slave_cmd, pic_icw1_init + pic_icw1_icw4);
	outb(pic_slave_imr, 0x28); // pic 2 offset
	outb(pic_slave_imr, 2); // pic 2 cascade line
	outb(pic_slave_imr, pic_icw4_default); // no auto-eoi

	// mask all PIC interrupts
	outb(pic_slave_imr, 0xff);
	outb(pic_master_imr, 0xff);
}

static volatile uint32_t *lapic;
static uint32_t lapic_frequency;

void apic_init(uint32_t lapic_address, bool legacy_pic) {
	if (legacy_pic)
		pic_disable();

	// TODO: factor out temporary mappings
	extern uint64_t kernel_pml4[], pt_map[];
	pt_map[0] = lapic_address | PAGE_PRESENT | PAGE_WRITE;
	write_cr3(PHYS_KERNEL(kernel_pml4));
	lapic = (uint32_t*)0xffffffffc0000000;

	uint32_t lapic_version = lapic[apic_version];

	uint32_t eax, edx;
	rdmsr(ia32_apic_base, &eax, &edx);
	wrmsr(ia32_apic_base, eax | apic_global_enable, 0);

	// TODO: set spurious interrupt vector explicitly
	lapic[apic_spurious] |= apic_sw_enable;

	kprintf("apic: %#010x - version %d\n", lapic_address, lapic_version & 0xf);
}

void apic_timer_calibrate(void) {
	lapic[apic_lvt_timer] = apic_timer_oneshot | 0x30;

	// TODO: move femtoseconds constant
	uint64_t wait_time = 100000000000000ULL / hpet_period() / 100;

	uint64_t hpet_start = hpet_now();
	lapic[apic_timer_init] = 0xffffffff;

	while (hpet_now() - hpet_start < wait_time) continue;

	lapic[apic_lvt_timer] = apic_lvt_mask;
	uint32_t lapic_end = lapic[apic_timer_current];

	lapic_frequency = (0xffffffff - lapic_end) * 100;

	kprintf("apic timer: %u.%06uMHz\n", lapic_frequency / 1000000, lapic_frequency % 1000000);
}
