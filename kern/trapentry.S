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
#TRAPHANDLER(__idt_default, T_DEFAULT)		// catchall



/*
 * Lab 3: Your code here for _alltraps
 */
.globl _alltraps
_alltraps:
    pushl %ds
    pushl %es
    pushal
    movl $GD_KD, %eax
    movw %ax, %ds
    movw %ax, %es
    pushl %esp
    call trap
    pop %esp
    popal
    pop %es
    pop %ds
    addl $0x8, %esp
    iret

