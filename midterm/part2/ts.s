	.text
	.code 32
	
.global reset_handler
.global vectors_start, vectors_end
.global proc, procsize
.global tswitch, scheduler, running
.global lock, unlock, int_off, int_on
	
reset_handler:
	
// set SVC mode sp to high end of proc[0]
  LDR r0, =proc
  LDR r1, =procsize
  LDR r2, [r1,#0]
  ADD r0, r0, r2
  MOV sp, r0
	
// copy vector table to address 0
  BL copy_vectors

// to IRQ mode and set IRQ mode sp
  MSR cpsr, #0x12
  LDR sp, =irq_stack

// back to SVC mode; call main()
  MSR cpsr, #0x13

  BL main
  B .

.align 4
	
irq_handler:
  sub	lr, lr, #4
  stmfd	sp!, {r0-r10, fp, ip, lr}
  bl	IRQ_handler  
  ldmfd	sp!, {r0-r10, fp, ip, pc}^

tswitch:
//       1  2  3  4  5  6  7  8  9  10  11  12  13  14
//       ---------------------------------------------
// stack=r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12,r14
//       ---------------------------------------------
  stmfd	sp!, {r0-r12, lr}

// mask out IRQ
  MRS r4, cpsr
  ORR r4, r4, #0x80   // set bit means MASK off IRQ interrupt 
  MSR cpsr, r4

  LDR r0, =running    // r0=&running
  LDR r1, [r0,#0]     // r1->runningPROC
  str sp, [r1,#4]     // running->ksp = sp

  bl	scheduler     // set new running

  LDR r0, =running
  LDR r1, [r0,#0]     // r1->runningPROC
  lDR sp, [r1,#4]     // restore running saved sp

// mask in IRQ
  MRS r4, cpsr
  BIC r4, r4, #0x80   // clear bit means UNMASK IRQ interrupt
  MSR cpsr, r4
 	
  ldmfd	sp!, {r0-r12, pc}  // NOTE: retrun to SAME mode

// unlock/lock: mask in/out IRQ interrupts
unlock:
  MRS r4, cpsr
  BIC r4, r4, #0x80   // clear bit means UNMASK IRQ interrupt
  MSR cpsr, r4
  mov pc, lr	

lock: 
  MRS r4, cpsr
  ORR r4, r4, #0x80   // set bit means MASK off IRQ interrupt 
  MSR cpsr, r4
  mov pc, lr
	
// int on(int cpsr): restore original cpsr from int_off()
int_on: // restore CPSR from r0
  MSR cpsr, r0
  mov pc, lr
	
// int sr = int_off(): mask out IRQ , return original CPSR
int_off: // return CPSR in r0
  MRS r0, cpsr
  MRS r1, cpsr
  ORR r1, r1, #0x80   // set bit means MASK off IRQ interrupt 
  MSR cpsr, r1
  mov pc, lr	

vectors_start:
  LDR PC, reset_handler_addr
  LDR PC, undef_handler_addr
  LDR PC, swi_handler_addr
  LDR PC, prefetch_abort_handler_addr
  LDR PC, data_abort_handler_addr
  B .
  LDR PC, irq_handler_addr
  LDR PC, fiq_handler_addr

reset_handler_addr:          .word reset_handler
undef_handler_addr:          .word undef_handler
swi_handler_addr:            .word swi_handler
prefetch_abort_handler_addr: .word prefetch_abort_handler
data_abort_handler_addr:     .word data_abort_handler
irq_handler_addr:            .word irq_handler
fiq_handler_addr:            .word fiq_handler

vectors_end:


	
.end
