#include "smp.h"
#include "spinlock.h"
#include "apic.h"
#include "tsc.h"
#include "memory.h"
#include <paging.h>
#include <kprintf.h>
#include <string.h>
#include <stdint.h>

extern char trampoline_begin[], trampoline_end[];
extern char percpu_begin[], percpu_end[];

extern volatile uint64_t startup_stack;
extern volatile uint64_t startup_code;
extern volatile uint64_t startup_gs;

#define TRAMPOLINE_SYM(base, sym) \
	*(__typeof__(sym)*)((char*)(base) + (uintptr_t)&(sym))

extern volatile uint32_t smp_ap_started;

// TODO: put in .startup.data
uint8_t lapic_by_cpu[256];

void *percpu_data[256];

SMP_PERCPU uint32_t smp_id;

static struct spinlock print_lock;
static volatile bool ap_initialized = true;
void smp_start(void) {
	ap_initialized = true;

	struct spinlock_node node;
	spin_lock(&print_lock, &node);
	kprintf("cpu %d started\n", SMP_PERCPU_READ(smp_id));
	spin_unlock(&print_lock, &node);

	while (true) __asm__ ("hlt");
}

void smp_init(void) {
	// TODO: memory_find from 0 with a better error code?
	uint64_t trampoline_size = trampoline_end - trampoline_begin;
	uint64_t trampoline = memory_find(PAGE_SIZE, 0x100000, trampoline_size, PAGE_SIZE);
	memory_reserve(trampoline, trampoline_size);
	memcpy((void*)trampoline, trampoline_begin, trampoline_size);

	// allocate percpu data now that we have a number from acpi
	uint64_t percpu_size = percpu_end - percpu_begin + PAGE_SIZE;
	void *percpu = memory_alloc(0x100000, memory_end(), lapic_count * percpu_size, PAGE_SIZE);

	volatile uint32_t *ap_started = &TRAMPOLINE_SYM(trampoline, smp_ap_started);
	startup_code = (uintptr_t)smp_start;

	uint32_t bsp_id = apic_read(apic_id);
	for (unsigned i = 0; i < lapic_count; i++) {
		uint32_t apic_id = lapic_by_cpu[i];
		if (apic_id == bsp_id) {
			continue;
		}

		percpu_data[i] = (char*)percpu + i * percpu_size;
		memcpy(percpu_data[i], percpu_begin, percpu_size);

		SMP_PERCPU_SYM(i, smp_id) = i;

		// wait for the last AP to finish using the trampoline
		while (!ap_initialized) {
			continue;
		}
		ap_initialized = false;

		*ap_started = 0;
		startup_gs = (uintptr_t)percpu_data[i];
		startup_stack = (uintptr_t)percpu_data[i] + percpu_size;

		apic_icr_write(apic_id, apic_icr_level | apic_icr_assert | apic_icr_init);
		apic_icr_wait_idle(100);
		apic_icr_write(apic_id, apic_icr_level | apic_icr_deassert | apic_icr_init);
		apic_icr_wait_idle(100);

		// MP spec says wait 10ms here, but newer CPUs don't need it (e.g. intel family 6+)

		bool sent = false;
		uint8_t error = 0;
		for (int i = 0; i < 2; i++) {
			// start execution on the target core at CS:IP = (trampoline >> 4):0
			apic_icr_write(apic_id, apic_icr_startup | (trampoline >> PAGE_SHIFT));
			tsc_udelay(10);
			sent = apic_icr_wait_idle(100);
			tsc_udelay(200);

			error = apic_esr_read();
			if (!sent || error || *ap_started)
				break;
		}

		if (!sent)
			kprintf("apic: [%d] startup ipi was not delivered\n", i);
		if (error)
			kprintf("apic: [%d] delivery error %x\n", i, error);
		if (!*ap_started)
			kprintf("apic: [%d] ap didn't set flag\n", i);
	}
}
