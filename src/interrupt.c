#include "interrupt.h"
#include "segment.h"
#include <kprintf.h>
#include <stdint.h>

enum {
	IDT_INTERRUPT = 0xe,
	IDT_TRAP = 0xf,
};

static struct idt_entry {
	uint16_t offset_low;
	uint16_t selector;
	uint8_t ist: 3, reserved0: 5;
	uint8_t type: 5, dpl: 2, p: 1;
	uint16_t offset_middle;
	uint32_t offset_high;
	uint32_t reserved1;
} idt[256];

#define IDT_ENTRY(offset, segment, t) (struct idt_entry){ \
	.offset_low = (offset) & 0xffff, \
	.offset_middle = ((offset) >> 16) & 0xffff, \
	.offset_high = (offset) >> 32, \
	.selector = (segment), \
	.type = (t), \
	.p = 1, \
}

struct registers {
	uint64_t rax;
	uint64_t rbx;
	uint64_t rcx;
	uint64_t rdx;
	uint64_t rdi;
	uint64_t rsi;
	uint64_t rbp;
	uint64_t r8;
	uint64_t r9;
	uint64_t r10;
	uint64_t r11;
	uint64_t r12;
	uint64_t r13;
	uint64_t r14;
	uint64_t r15;
	uint64_t error_code;
	uint64_t rip;
	uint64_t cs;
	uint64_t rflags;
	uint64_t rsp;
	uint64_t ss;
};

extern void default_exception();
extern void default_interrupt();
extern void spurious_interrupt();

extern void isr_divide_error();
void divide_error(struct registers *registers) {
	kprintf("divide error\n");
	for (;;);
}

extern void isr_general_protection_fault();
void general_protection_fault(struct registers *registers) {
	kprintf("general protection fault %#lx\n", registers->error_code);
	for (;;);
}

extern void isr_page_fault();
void page_fault(struct registers *registers) {
	uint64_t address;
	__asm__ volatile ("mov %%cr2, %0" : "=r"(address));

	kprintf("page fault %#016lx %#lx\n", address, registers->error_code);
	for (;;);
}

void interrupt_init() {
	for (int i = 0; i < 32; i++) {
		idt[i] = IDT_ENTRY((uint64_t)default_exception, SEG_KERNEL_CODE, IDT_TRAP);
	}

	for (int i = 32; i < 256; i++) {
		idt[i] = IDT_ENTRY((uint64_t)default_interrupt, SEG_KERNEL_CODE, IDT_TRAP);
	}

	idt[0] = IDT_ENTRY((uint64_t)isr_divide_error, SEG_KERNEL_CODE, IDT_TRAP);
	idt[13] = IDT_ENTRY((uint64_t)isr_general_protection_fault, SEG_KERNEL_CODE, IDT_TRAP);
	idt[14] = IDT_ENTRY((uint64_t)isr_page_fault, SEG_KERNEL_CODE, IDT_TRAP);
	idt[39] = IDT_ENTRY((uint64_t)spurious_interrupt, SEG_KERNEL_CODE, IDT_TRAP);

	struct idt_pointer {
		uint16_t limit;
		uint64_t base;
	} __attribute__((packed)) idt_ptr = {
		.limit = sizeof(idt) - sizeof(*idt),
		.base = (uint64_t)idt,
	};

	extern void load_idt(struct idt_pointer*);
	load_idt(&idt_ptr);
}
