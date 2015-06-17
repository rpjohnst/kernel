	.text

	.global load_idt
load_idt:
	lidt (%rdi)
	ret

	.data
message: .asciz "exception\n"
timer: .asciz "|"
	.previous

	.global default_exception
default_exception:
	mov $message, %rdi
	call kprintf
1:
	cli
	hlt
	jmp 1b

	.global default_interrupt
default_interrupt:
	mov $timer, %rdi
	call kprintf
	movl $0, 0xffffffffc00000b0
	iretq

	.macro isr handler error_code
	.align 8
	.global isr_\handler
isr_\handler:
	.if \error_code
	mov (%rsp), %rsi
	.else
	push $0
	xor %rsi, %rsi
	.endif

	push %r15
	push %r14
	push %r13
	push %r12
	push %r11
	push %r10
	push %r9
	push %r8
	push %rbp
	push %rsi
	push %rdi
	push %rdx
	push %rcx
	push %rbx
	push %rax

	mov $0x10, %ax
	mov %ax, %ds
	mov %ax, %ss

	mov %rsp, %rdi
	call \handler

	pop %rax
	pop %rbx
	pop %rcx
	pop %rdx
	pop %rdi
	pop %rsi
	pop %rbp
	pop %r8
	pop %r9
	pop %r10
	pop %r11
	pop %r12
	pop %r13
	pop %r14
	pop %r15

	add $0x8, %rsp

	iretq
	.endm

isr divide_error 0
isr general_protection_fault 1
isr page_fault 1
