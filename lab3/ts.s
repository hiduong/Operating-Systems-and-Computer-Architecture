	.text
	.code 32
	
.global reset_handler
.global vectors_start, vectors_end
.global lock, unlock
	
reset_handler:
	
// set SVC mode sp to high end of proc[0]
  LDR sp, =svc_stack_top
  BL copy_vectors
  MSR cpsr, #0x92
  LDR sp,=irq_stack_top
  MSR cpsr,#0x13
  BL main
  B .
	
.align 4
irq_handler:
  sub	lr, lr, #4
  stmfd	sp!, {r0-r12,lr}
  bl	IRQ_handler
  ldmfd	sp!, {r0-r12, pc}^
  	
// unlock/lock: mask in/out IRQ interrupts
unlock:
  MRS r0, cpsr
  BIC r0, r0, #0x80   // clear bit means UNMASK IRQ interrupt
  MSR cpsr, r0
  mov pc, lr	

lock: 
  MRS r0, cpsr
  ORR r0, r0, #0x80   // set bit means MASK off IRQ interrupt 
  MSR cpsr, r0
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
