/* ----------------------------------------------------------------------------
 *         ATMEL Microcontroller Software Support  -  ROUSSET  -
 * ----------------------------------------------------------------------------
 * Copyright (c) 2006, Atmel Corporation

 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * - Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the disclaimer below.
 * 
 * Atmel's name may not be used to endorse or promote products derived from
 * this software without specific prior written permission. 
 * 
 * DISCLAIMER: THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * ----------------------------------------------------------------------------
 * File Name           : crt0_gnu.S
 * Object              : 
 * Creation            : ODi Apr 19th 2006
 *-----------------------------------------------------------------------------
 */

.section start
	.text

 	#include "include/part.h"
	
	
/*----------------------------------------------------------------------------
 Area Definition
----------------
 Must be defined as function to put first in the code as it must be mapped
 at offset 0 of the flash EBI_CSR0, ie. at address 0 before remap.
_---------------------------------------------------------------------------*/

/* Application startup entry point */

	.globl reset
	.align 4
reset:

/* Exception vectors (should be a branch to be detected as a valid code by the rom */
_exception_vectors:
	b 	reset_vector    /* reset */
	b 	reset_vector 	/* Undefined Instruction */
	b 	reset_vector   	/* Software Interrupt */
	b 	pabt_vector  	/* Prefetch Abort */
	b 	dabt_vector  	/* Data Abort */
.word		_edata		/* Size of the image for SAM-BA */
	b 	irq_vector	/* IRQ : read the AIC */
	b 	fiq_vector      /* FIQ */

undef_vector:
	b 	undef_vector
swi_vector:
	b 	swi_vector
pabt_vector:
	b 	pabt_vector
dabt_vector:
	b 	dabt_vector
rsvd_vector:
	b 	rsvd_vector
irq_vector:
	b 	irq_vector
fiq_vector:
	b 	fiq_vector
reset_vector:

/* Init the stack */
_init_stack:
	ldr     sp,=TOP_OF_MEM
	
	/* Clear all LED's */
	ldr		r1,=AT91C_PIOC_OER
	ldr		r0,=0x0EE00280
	str		r0,[r1]
	
	ldr		r1,=AT91C_PIOC_CODR
	ldr		r0,=0x0EE00280
	str		r0,[r1]
	
#ifdef CFG_NORFLASH
 
	/**************************************************************************
	 jump to 'unmapped' flash origin (@0x1000'0000+PC)
	 **************************************************************************/
	ldr r0,=0x10000000
	sub r0,r0,#4
	add pc,pc,r0
 
	/* relocation is slow, disable the watchdog or it will trigger */
	/* ldr	r1, =0xFFFFFD44
	   mov	r2, #0x00008000
	   str	r2, [r1] */
 
	/* signal red state LED */
	ldr		r1,=AT91C_PIOC_SODR
	ldr		r0,=0x00000080
	str		r0,[r1]
	
	/**************************************************************************
	 remap SRAM to address 0x0000'0000
	 **************************************************************************/
	ldr		r1,=AT91C_MATRIX_MRCR
	ldr		r0,=0x3
	str		r0,[r1]
	
	/**************************************************************************
	 Relocate 4K code from FLASH start + PC to SRAM and jump
	 **************************************************************************/
	ldr		r0,=0x10000000
	ldr		r1,=0x00000000
	ldr		r11,=0x00001000
	
CopyLoop:
	ldmia	r0!,{r2-r9}
	stmia	r1!,{r2-r9}
	cmp		r1,r11
	bne		CopyLoop

	ldr		r0,=ExecuteFromSRAM
	orr		r0, r0, #0
	mov		pc, r0

	/* -> we are now on internal SRAM */ 

ExecuteFromSRAM:
	
	/* signal red OUT1 LED */
	ldr		r1,=AT91C_PIOC_SODR
	ldr		r0,=0x00200000
	str		r0,[r1]
	 
/* Copy the data section in RAM at .data link address */
_init_data:
        ldr      r2, =_lp_data
        ldmia    r2, {r1, r3, r4}
1:
        cmp      r3, r4
        ldrcc    r2, [r1], #4
        strcc    r2, [r3], #4
        bcc      1b

/* Initialize the bss segment */
_init_bss:
	adr    r2, _lp_bss
	ldmia  r2, {r3, r4}
	mov    r2, #0
1:
	cmp    r3, r4
	strcc  r2, [r3], #4
	bcc    1b

#else
	"This version of AT91Bootstrap can only be used with CFG_NORFLASH defined ! ! !"
#endif
	
/* Branch on C code Main function (with interworking) */
_branch_main:

	/* signal red OUT2 LED */
	ldr		r1,=AT91C_PIOC_SODR
	ldr		r0,=0x00400000
	str		r0,[r1]

	ldr     r4, = main
	mov     lr, pc
	bx      r4

/* Branch to the application at the end of the bootstrap init */
_go:
	ldr 	r1, =MACH_TYPE
	mov     lr, pc
	bx      r0

	.align
_lp_data:
        .word _etext
        .word _sdata
        .word _edata

_lp_bss:
	.word _sbss
	.word _ebss
