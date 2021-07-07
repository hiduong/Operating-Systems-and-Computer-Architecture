/************ ts.s file of C2.1 ************/

.text
.global start

start:
 mov r0, #1 //register r0 = 1
 MOV R1, #2 //register r1 = 2
 ADD R1, R1, R0 //add r0 and r1 r1 = r1 + r0
 ldr r2, =result //r2 = &result
 str r1, [r2] // result = r1 stores result of sum into memory location of the label result
	
stop: b stop
	
 .data
	
result:	.word 0 //a word location
