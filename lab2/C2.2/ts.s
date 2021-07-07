/************ ts.s file of C2.2 ************/

.text
.global start
	
start:	 ldr sp, =stack_top //set stack pointer

	bl sum //call sum

stop:	b stop //looping

sum:	//int sum(): computer the sum of an int array in Result
	stmfd sp!, {r0-r4, lr} //save r0-r4, lr on stack
	mov r0, #0 //r0 = 0
	ldr r1, =Array //r1 = &Array
	ldr r2, =N //r2 = &N
	ldr r2, [r2] //r2 = N
loop:
	ldr r3, [r1], #4 //r3 = *(r1++)
	add r0,r0,r3 //r0 += r3
	sub r2,r2,#1 //r2--
	cmp r2, #0 //if(r2 != 0)
	bne loop // goto loop;
	ldr r4, =Result //r4 = &Result
	str r0, [r4] // Result = r0
		
.data
	
N:	 .word 10 //number of array elements
Array:	 .word 1,2,3,4,5,6,7,8,9,10
Result:	 .word 0
