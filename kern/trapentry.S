/* See COPYRIGHT for copyright information. */

#include <inc/mmu.h>
#include <inc/memlayout.h>
#include <inc/trap.h>

#include <kern/picirq.h>


###################################################################
# exceptions/interrupts
###################################################################

/* TRAPHANDLER defines a globally-visible function for handling a trap.
 * It pushes a trap number onto the stack, then jumps to _alltraps.
 * Use TRAPHANDLER for traps where the CPU automatically pushes an error code.
 *
 * You shouldn't call a TRAPHANDLER function from C, but you may
 * need to _declare_ one in C (for instance, to get a function pointer
 * during IDT setup).  You can declare the function with
 *   void NAME();
 * where NAME is the argument passed to TRAPHANDLER.
 */
#define TRAPHANDLER(name, num)						\
	.globl name;		/* define global symbol for 'name' */	\
	.type name, @function;	/* symbol type is function */		\
	.align 2;		/* align function definition */		\
	name:			/* function starts here */		\
	pushl $(num);							\
	jmp _alltraps

/* Use TRAPHANDLER_NOEC for traps where the CPU doesn't push an error code.
 * It pushes a 0 in place of the error code, so the trap frame has the same
 * format in either case.
 */
#define TRAPHANDLER_NOEC(name, num)					\
	.globl name;							\
	.type name, @function;						\
	.align 2;							\
	name:								\
	pushl $0;							\
	pushl $(num);							\
	jmp _alltraps

.text

/*
 * Lab 3: Your code here for generating entry points for the different traps.
 */
TRAPHANDLER_NOEC(__idt_divide, T_DIVIDE)		// divide error
TRAPHANDLER_NOEC(__idt_debug, T_DEBUG)		// debug exception
TRAPHANDLER_NOEC(__idt_nmi, T_NMI)		// non-maskable interrupt
TRAPHANDLER_NOEC(__idt_breakpoint, T_BRKPT)		// breakpoint
TRAPHANDLER_NOEC(__idt_overflow, T_OFLOW)		// overflow
TRAPHANDLER_NOEC(__idt_bound, T_BOUND)		// bounds check
TRAPHANDLER_NOEC(__idt_illop, T_ILLOP)		// illegal opcode
TRAPHANDLER_NOEC(__idt_device, T_DEVICE)		// device not available
TRAPHANDLER(__idt_dblflt, T_DBLFLT)		// double fault
TRAPHANDLER(__idt_tss, T_TSS)		// invalid task switch segment
TRAPHANDLER(__idt_segnp, T_SEGNP)		// segment not present
TRAPHANDLER(__idt_stack, T_STACK)		// stack exception
TRAPHANDLER(__idt_gpflt, T_GPFLT)		// general protection fault
TRAPHANDLER(__idt_pgflt, T_PGFLT)		// page fault
TRAPHANDLER_NOEC(__idt_fperr, T_FPERR)		// floating point error
TRAPHANDLER(__idt_align, T_ALIGN)		// aligment check
TRAPHANDLER_NOEC(__idt_mchk, T_MCHK)		// machine check
TRAPHANDLER_NOEC(__idt_simd, T_SIMDERR)		// SIMD floating point error
TRAPHANDLER_NOEC(__idt_syscall, T_SYSCALL)		// system call



TRAPHANDLER_NOEC(__idt_irq0, IRQ_OFFSET + 0)		// system call
TRAPHANDLER_NOEC(__idt_irq1, IRQ_OFFSET + 1)		// system call
TRAPHANDLER_NOEC(__idt_irq2, IRQ_OFFSET + 2)		// system call
TRAPHANDLER_NOEC(__idt_irq3, IRQ_OFFSET + 3)		// system call
TRAPHANDLER_NOEC(__idt_irq4, IRQ_OFFSET + 4)		// system call
TRAPHANDLER_NOEC(__idt_irq5, IRQ_OFFSET + 5)		// system call
TRAPHANDLER_NOEC(__idt_irq6, IRQ_OFFSET + 6)		// system call
TRAPHANDLER_NOEC(__idt_irq7, IRQ_OFFSET + 7)		// system call
TRAPHANDLER_NOEC(__idt_irq8, IRQ_OFFSET + 8)		// system call
TRAPHANDLER_NOEC(__idt_irq9, IRQ_OFFSET + 9)		// system call
TRAPHANDLER_NOEC(__idt_irq10, IRQ_OFFSET + 10)		// system call
TRAPHANDLER_NOEC(__idt_irq11, IRQ_OFFSET + 11)		// system call
TRAPHANDLER_NOEC(__idt_irq12, IRQ_OFFSET + 12)		// system call
TRAPHANDLER_NOEC(__idt_irq13, IRQ_OFFSET + 13)		// system call
TRAPHANDLER_NOEC(__idt_irq14, IRQ_OFFSET + 14)		// system call


TRAPHANDLER(__idt_default, T_DEFAULT)		// catchall



/*
 * Lab 3: Your code here for _alltraps
 */
.globl _alltraps
_alltraps:
    #cli
    pushl %ds
    pushl %es
    pushal
    movl $GD_KD, %eax
    movw %ax, %ds
    movw %ax, %es

    pushfl
    popl %eax
    movl $FL_IF, %ebx
    notl %ebx
    andl %ebx, %eax
    pushl %eax
    popfl

    pushl %esp
    call trap
    pop %esp
    popal
    pop %es
    pop %ds
    addl $0x8, %esp
    iret

