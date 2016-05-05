#include "apic.h"
#include "tsc.h"
#include "cpu.h"
#include "hpet.h"
#include <paging.h>
#include <kprintf.h>

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

uint32_t lapic_count;
SMP_PERCPU uint32_t lapic_id;

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

uint32_t apic_read(uint32_t reg) {
	return lapic[reg];
}

void apic_icr_write(uint32_t high, uint32_t low) {
	lapic[apic_icr_high] = high;
	lapic[apic_icr_low] = low;
}

bool apic_icr_wait_idle(uint32_t msecs) {
	bool pending;
	for (uint32_t timeout = 0; timeout < 10 * msecs; timeout++) {
		pending = lapic[apic_icr_low] & apic_icr_pending;
		if (!pending)
			break;

		tsc_udelay(100);
	}

	return !pending;
}

uint32_t apic_esr_read(void) {
	return lapic[apic_esr] & 0xef;
}
