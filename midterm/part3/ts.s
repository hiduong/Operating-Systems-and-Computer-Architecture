	.text
	.code 32
	
.global vectors_start, vectors_end
.global reset_handler, mkptable	
.global get_fault_status, get_fault_addr, get_spsr
.global lock, unlock
	
reset_handler:	
  LDR sp, =svc_stack_top
  BL fbuf_init
  BL copy_vectors_table

  BL mkptable

  mov r0, #0x4000
  mcr p15, 0, r0, c2, c0, 0
  mcr p15, 0, r0, c8, c7, 0	

  mov r0, #1
  mcr p15, 0, r0, c3, c0, 0

  mrc p15, 0, r0, c1, c0, 0
  orr r0, r0, #0x00000001
  mcr p15, 0, r0, c1, c0, 0
  nop
  nop
  nop
  mrc p15, 0, r2, c2, c0, 0
  mov r2, r2

  MSR cpsr, #0x97
  LDR sp, =abt_stack_top
  MSR cpsr, #0x92
  LDR sp, =irq_stack_top
  MSR cpsr, #0x13
  BL main
  B .
	
swi_handler:
data_handler:
  sub lr, lr, #4
  stmfd sp!, {r0-r12,lr}
  bl data_chandler
  ldmfd sp!, {r0-r12, pc}^
irq_handler:
  sub	lr, lr, #4
  stmfd	sp!, {r0-r12,lr}
  bl	irq_chandler  
  ldmfd	sp!, {r0-r12, pc}^

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

get_fault_status:
  MRC p15,0,r0,c5,c0,0
  mov pc, lr
get_fault_addr:
  MRC p15,0,r0,c6,c0,0
  mov pc, lr
get_spsr:
  mrs r0, spsr
  mov pc, lr

	
.end
