/* See COPYRIGHT for copyright information. */

#include <inc/mmu.h>
#include <inc/memlayout.h>
#include <inc/trap.h>
#include <kern/macro.h>



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
	pushq $(num);							\
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
	pushq $0;							\
	pushq $(num);							\
	jmp _alltraps

.text

/*
 * Lab 3: Your code here for generating entry points for the different traps.
   Find the PUSHA,POPA macros in kern/macro.h. Since amd64 doesn't support
   pusha,popa so you will find these macros handy.
 */


TRAPHANDLER_NOEC(handle_0, T_DIVIDE )
TRAPHANDLER_NOEC(handle_1, T_DEBUG  )
TRAPHANDLER_NOEC(handle_2, T_NMI  )
TRAPHANDLER_NOEC(handle_3, T_BRKPT )
TRAPHANDLER_NOEC(handle_4, T_OFLOW )
TRAPHANDLER_NOEC(handle_5, T_BOUND )
TRAPHANDLER_NOEC(handle_6, T_ILLOP )
TRAPHANDLER_NOEC(handle_7, T_DEVICE )

TRAPHANDLER(handle_8, T_DBLFLT)	


TRAPHANDLER(handle_10, T_TSS )
TRAPHANDLER(handle_11, T_SEGNP  )
TRAPHANDLER(handle_12, T_STACK  )
TRAPHANDLER(handle_13, T_GPFLT )
TRAPHANDLER(handle_14, T_PGFLT )

TRAPHANDLER_NOEC(handle_16, T_FPERR )

TRAPHANDLER(handle_17, T_ALIGN )


TRAPHANDLER_NOEC(handle_18, T_MCHK )
TRAPHANDLER_NOEC(handle_19, T_SIMDERR )

TRAPHANDLER_NOEC(handle_32, IRQ_TIMER )
TRAPHANDLER_NOEC(handle_33, IRQ_KBD )
TRAPHANDLER_NOEC(handle_36, IRQ_SERIAL )
TRAPHANDLER_NOEC(handle_39, IRQ_SPURIOUS )
TRAPHANDLER_NOEC(handle_46, IRQ_IDE )
TRAPHANDLER_NOEC(handle_51, IRQ_ERROR )
	
TRAPHANDLER_NOEC(handle_default, T_DEFAULT)




/*
 * Lab 3: Your code here for _alltraps
 *
 * Hint: Be sure to review the x64 calling convention from lab1
 *       for how to pass a pointer to the trapframe.
 */
_alltraps:
	 movq $0,%rax
	 movw %ds,%ax
	 pushq %rax
	 movw %es,%ax
	 pushq %rax

	 PUSHA

	 movw $GD_KD,%ax
	 movw %ax,%ds
	 movw $GD_KD,%ax
	 movw %ax,%es

	 movq %rsp,%rdi
	 call trap
