/********************************************************************
Copyright 2010-2017 K.C. Wang, <kwang@eecs.wsu.edu>
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
********************************************************************/
	.text
.code 32
.global reset_handler
.global vectors_start, vectors_end
.global proc, procsize
.global tswitch, scheduler, running, goUmode
.global int_on, int_off, lock, unlock, getcsr
.global switchPgdir
.global get_fault_status, get_fault_addr, get_spsr

/********************************************************
mode:	USER: 10000  0x10
	FIQ : 10001  0x11
	IRQ : 10010  0x12
	SVC : 10011  0x13
        ABT : 10111  0x17
	UND : 11011  0x1B
	SYS : 11111  0x1F
********************************************************/
reset_handler:
  /* set SVC stack to HIGH END of proc[0].kstack[] */
  LDR r0, =proc      // r0 points to proc's
  LDR r1, =procsize  // r1 -> procsize
  LDR r2, [r1,#0]    // r2 = procsize
  ADD r0, r0, r2     // r0 -> high end of proc[0]
  MOV sp, r0
	
  /* go in IRQ mode to set IRQ stack and enable IRQ interrupts */
  MSR cpsr, #0x12
  LDR sp, =irq_stack_top  // set IRQ stack poiner

  /* go in ABT mode to set ABT stack */
  MSR cpsr, #0x17
  LDR sp, =abt_stack_top

  /* go in UND mode to set UND stack */
  MSR cpsr, #0x1B
  LDR sp, =und_stack_top

  /* go back in SVC mode */
  MSR cpsr, #0x13      // write to cspr, so in SVC mode now

  // set previous mode of SVC mode to USER mode with interrupts ON
  MSR spsr, #0x10

  /* copy vector table to address 0 */
  BL copy_vectors

/*********L1 section entry ***********************************
 |3            2|1|1111111|11|0|0000|0|00|00
 |1            0|9|8765432|10|9|8765|4|32|10|
 |     addr     | |       |AP|0|DOM |1|CB|10|
 |              |000000000|01|0|0000|1|00|10|
                           KRW  dom0
                          0x 4    1      2
	FIRST TRY: AP=11 for both KU r/w ==> 0xC12
*************************************************************/
  BL mkPtable                 // build pagetable in C; much easier

  ldr r0, Mtable
  mcr p15, 0, r0, c2, c0, 0  // set TTBase
  mcr p15, 0, r0, c8, c7, 0  // flush TLB

  // set domain0: 01=client(check permission) 11=master(no check)
  mov r0, #0x5               // 01 for CLIENT
  mcr p15, 0, r0, c3, c0, 0
	
// enable MMU
  mrc p15, 0, r0, c1, c0, 0
  orr r0, r0, #0x00000001     // set bit0
  mcr p15, 0, r0, c1, c0, 0   // write to c1
  nop
  nop
  nop
	
  mrc p15, 0, r2, c2, c0
  mov r2, r2

  adr r0, mainstart
  ldr pc, [r0]
	
  B .
	
.align 4
Mtable:	    .word 0x400000  // at 4MB
mainstart:  .word main
	
.align 4

irq_handler:         // IRQ entry point
  sub	lr, lr, #4  
  stmfd	sp!, {r0-r12, lr}  // save all Umode regs in kstack

  bl	IRQ_handler  // call IRQ_handler() in C in svc.c file   

  ldmfd	sp!, {r0-r12, pc}^ // pop from kstack but restore Umode SR

data_handler:

  sub	lr, lr, #4
  stmfd	sp!, {r0-r12, lr}

  bl	data_chandler  

  ldmfd	sp!, {r0-r12, pc}^


tswitch: // tswitch() in Kmode
//       1  2  3  4  5  6  7  8  9  10  11  12  13  14
//       ---------------------------------------------
// stack=r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12,r14
//       ---------------------------------------------
  stmfd	sp!, {r0-r12, lr}

// mask out IRQ
  MRS r0, cpsr
  ORR r0, r0, #0x80
  MSR cpsr, r0

  LDR r0, =running // r0=&running
  LDR r1, [r0,#0]  // r1->runningPROC
  str sp, [r1,#4]  // running->ksp = sp

  bl	scheduler

  LDR r0, =running
  LDR r1, [r0,#0]     // r1->runningPROC
  lDR sp, [r1,#4]     // sp = running->ksp

// mask in IRQ
  MRS r0, cpsr
  BIC r0, r0, #0x80
  MSR cpsr, r0
	
  ldmfd	sp!, {r0-r12, pc}

klr:	.word 0

svc_entry: // SVC entry point

   stmfd sp!, {r0-r12, lr}
	
   ldr r5, =running   // r5=&running
   ldr r6, [r5, #0]   // r6 -> PROC of running
	
   mrs r7, spsr       // save spsr in PROC.cpsr at offset 16
   str r7, [r6, #16]
	
// ksp needed in fork()   	
   str sp, [r6, #4]   // save ksp into PROC.ksp at offset 4
	
// to SYS mode to access USER mode usp, ulr
   MSR cpsr, #0x1F
   str sp, [r6, #8]   // save usp into proc.usp at offset 8
   str lr, [r6, #12]  // save Umode PC into proc.upc at offset 12
	
// back to SVC mode with IRQ masked in
   msr cpsr, #0x13

   bl  svc_handler
	
 // replace saved r0 on stack with the return value r from svc_handler()
   add sp, sp, #4      // effectively pop saved r0 off stack
   stmfd sp!, {r0}     // push r as the saved r0 to Umode

goUmode:

// restore saved usp from the PROC.usp

   ldr r5, =running   // r5=&running
   ldr r6, [r5, #0]   // r6 -> PROC of running

   ldr r7, [r6, #16]  // load SPSR from proc.ucpsr
   msr spsr, r7

// set cpsr to SYS mode to access user mode sp	
   MSR cpsr, #0x1F
   ldr sp, [r6, #8]   // restore usp from PROC.usp

// back to SVC mode	
   msr cpsr, #0x13    // back to SVC mode

// ^: pop regs from kstack BUT also copy spsr into cpsr ==> back to Umode
   ldmfd sp!, {r0-r12, pc}^ // ^ : pop kstack AND to previous mode

unlock:
  MRS r0, cpsr
  BIC r0, r0, #0x80
  MSR cpsr, r0
  mov pc,lr	

lock:
  MRS r0, cpsr
  ORR r0, r0, #0x80
  MSR cpsr, r0
  mov pc,lr	

int_on:
  MSR cpsr, r0
  mov pc,lr	

int_off:
  MRS r0, cpsr
  MRS r1, cpsr
  ORR r1, r1, #0x80
  MSR cpsr, r1
  mov pc,lr	

getcsr:
   mrs r0, cpsr
   mov pc, lr
	
get_spsr:
	mrs r0, spsr
	mov pc, lr
	
get_fault_status:	// read and return MMU reg 5
  MRC p15,0,r0,c5,c0,0    // read DFSR
  mov pc, lr	

get_fault_addr:	         // read and return MMU reg 6
  MRC p15,0,r0,c6,c0,0    // read DFSR
  mov pc, lr	



vectors_start:
  LDR PC, reset_handler_addr
  LDR PC, undef_handler_addr
  LDR PC, svc_handler_addr
  LDR PC, prefetch_abort_handler_addr
  LDR PC, data_abort_handler_addr
  B .
  LDR PC, irq_handler_addr
  LDR PC, fiq_handler_addr

reset_handler_addr:          .word reset_handler
undef_handler_addr:          .word undef_handler
svc_handler_addr:            .word svc_entry
prefetch_abort_handler_addr: .word prefetch_abort_handler
//data_abort_handler_addr:     .word data_abort_handler
data_abort_handler_addr:     .word data_handler
irq_handler_addr:            .word irq_handler
fiq_handler_addr:            .word fiq_handler

vectors_end:

switchPgdir:	// switch pgdir to new PROC's pgdir; passed in r0
  // r0 contains address of PROC's pgdir address	
  mcr p15, 0, r0, c2, c0, 0  // set TTBase
  mov r1, #0
  mcr p15, 0, r1, c8, c7, 0  // flush TLB
  mcr p15, 0, r1, c7, c10, 0  // flush TLB
  mrc p15, 0, r2, c2, c0, 0

  // set domain: all 01=client(check permission) 11=master(no check)
  mov r0, #0x5                // 11 for MASER
  mcr p15, 0, r0, c3, c0, 0
	
  mov pc, lr                  // return

.end
