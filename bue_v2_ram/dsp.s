.global dot3
.global mfilter
.global dq_to_abc
.global abc_to_dq
.global asm_test_loop

.section .data
flt_acm:	.word 0
flt_idx:	.word 0
flt_buf:	.skip 32*4, 0

.section .text

@; r0 - v[]
@; r1 - *ang
@; r2 - *mag
asm_test_loop:
	push {r4,r5,r6,r7,lr}

	ldr r3,[r0,#0]
	asr r4,r3,#31
	add r3,r3,r4
	eor r3,r3,r4	@; r3=abs(v[0])
	ldr r4,[r0,#4]	@; r4=v[1]
	
	eor r0,r0		@; r0 - SumAngle=0
	eor r7,r7		@; r7 - ns=0
	
	mov r5,r3
	movs r4,r4
	ble	ylesseq
	add r3,r3,r4
	sub r4,r4,r5
ylesseq:	
	sub r3,r3,r4
	add r4,r4,r5

	
	
/*loop:
	sub r0,r0,#1
	bpl loop
*/	

	
	pop {r4,r5,r6,r7,pc}

AngTable:	.word 128, 76, 40, 20, 10, 5, 3, 1					@; angles for those tg =0.5,0.25 etc
kc:			.word 724, 648, 628, 623, 623, 622, 622, 622 		@; mag correction cos(ang)

@; r0 - abc[]
@; r1 - dq[]
@; r2 - angle
abc_to_dq:
	push {r4,r5,r6,lr}

	ldr r3, =cos_tb 	
	lsl r2,#2

	@; calculate dq[0]
	ldr r4,[r3,r2]			@; r4 = cos_tb[angle]
	ldr r5,[r0]				@; r5 = abc[0]	
	mul r4,r4,r5			@; r4 = abc[0]*cos_tb[angle]
	
	ldr r6,P_4PI3
	add r6,r6,r2
	ldr r5,MASK1023
	and r5,r6
	ldr r6,[r3,r5]			@; r6 = cos_tb[angle+4*512/3]
	ldr r5,[r0,#4]			@; r5 = abc[1]
	mul r5,r5,r6			@; r5 = abc[1]*cos_tb[angle+4*512/3]	
	add r4,r4,r5			@; r4 = abc[0]*cos_tb[angle] + abc[1]*cos_tb[angle+4*512/3]
	
	ldr r6,P_2PI3
	add r6,r6,r2
	ldr r5,MASK1023
	and r5,r6
	ldr r6,[r3,r5]			@; r6 = cos_tb[angle+2*512/3]
	ldr r5,[r0,#8]			@; r5 = abc[2]
	mul r5,r5,r6			@; r5 = abc[2]*cos_tb[angle+2*512/3]	
	add r4,r4,r5			@; r4 = abc[0]*cos_tb[angle] + abc[1]*cos_tb[angle+4*512/3] + abc[2]*cos_tb[angle+2*512/3]
	
	asr r4,#10
	str r4,[r1]				@; dq[0] = r4
	
	@; calculate dq[1]
	ldr r6,P_3PI2
	add r6,r6,r2
	ldr r5,MASK1023
	and r5,r6
	ldr r6,[r3,r5]			@; r6 = sin[angle]
	ldr r4,[r0]				@; r4 = abc[0]
	mul r4,r4,r6			@; r4 = abc[0]*sin[angle]
	
	ldr r6,P_4PI33PI2
	add r6,r6,r2
	ldr r5,MASK1023
	and r5,r6
	ldr r6,[r3,r5]			@; r6 = sin[angle+4*512/3]
	ldr r5,[r0,#4]			@; r5 = abc[1]
	mul r5,r5,r6			@; r5 = abc[1]*sin[angle+4*512/3]	
	add r4,r4,r5			@; r4 = abc[0]*sin[angle] + abc[1]*sin[angle+4*512/3]
	
	ldr r6,P_2PI33PI2
	add r6,r6,r2
	ldr r5,MASK1023
	and r5,r6
	ldr r6,[r3,r5]			@; r6 = sin[angle+2*512/3]
	ldr r5,[r0,#8]			@; r5 = abc[2]
	mul r5,r5,r6			@; r5 = abc[2]*sin[angle+2*512/3]	
	add r4,r4,r5			@; r4 = abc[0]*sin[angle] + abc[1]*sin[angle+4*512/3] + abc[2]*sin[angle+2*512/3]
	
	asr r4,#10
	eor r6,r6
	sub r6,r4
	str r6,[r1,#4]			@; dq[1] = r4		
	
	pop {r4,r5,r6,pc}


@; r0 - abc[]
@; r1 - dq[]
@; r2 - angle
dq_to_abc:
	push {r4,r5,r6,lr}

	ldr r3, =cos_tb 	
	lsl r2,#2

	@ abc[0]
	ldr r4,[r3,r2]			@; r4 = cos_tb[angle]	
	ldr r5,[r1]				@; r5 = dq[0]	
	mul r4,r4,r5			@; r4 = dq[0]*cos_tb[angle]
		
	ldr r6,P_3PI2
	add r6,r6,r2
	ldr r5,MASK1023
	and r5,r6
	ldr r6,[r3,r5]			@; r6 = cos_tb[angle+3*512/2]
	ldr r5,[r1,#4]			@; r5 = dq[1]
	mul r5,r5,r6			@; r5 = dq[1]*cos_tb[angle+3*512/2]

	sub r4,r4,r5			@; r4 = dq[0]*cos_tb[angle] - dq[1]*cos_tb[angle+3*512/2]	
	asr r4,#20				
	str r4,[r0]				@; abc[0] = (dq[0]*cos_tb[angle] - dq[1]*cos_tb[angle+3*512/2]) >> 20
		
	@ abc[1]
	ldr r6,P_4PI3
	add r6,r6,r2
	ldr r5,MASK1023
	and r5,r6
	ldr r4,[r3,r5]			@; r4 = cos_tb[angle+4*512/3]
	ldr r5,[r1]				@; r5 = dq[0]
	mul r4,r4,r5			@; r4 = dq[0]*cos_tb[angle+4*512/3]
	
	ldr r6,P_4PI33PI2
	add r6,r6,r2
	ldr r5,MASK1023
	and r5,r6
	ldr r6,[r3,r5]			@; r6 = cos_tb[angle+4*512/3+3*512/2]
	ldr r5,[r1,#4]			@; r5 = dq[1]
	mul r5,r5,r6			@; r5 = dq[1]*cos_tb[angle+4*512/3+3*512/2]
	
	sub r4,r4,r5			@; r4 = dq[0]*cos_tb[angle+4*512/3] - dq[1]*cos_tb[angle+4*512/3+3*512/2]	
	asr r4,#20				
	str r4,[r0,#4]	
	
	@ abc[2]
	ldr r6,P_2PI3
	add r6,r6,r2
	ldr r5,MASK1023
	and r5,r6
	ldr r4,[r3,r5]	
	ldr r5,[r1]		
	mul r4,r4,r5	
	
	ldr r6,P_2PI33PI2
	add r6,r6,r2
	ldr r5,MASK1023
	and r5,r6
	ldr r6,[r3,r5]	
	ldr r5,[r1,#4]	
	mul r5,r5,r6	
	
	sub r4,r4,r5		
	asr r4,#20				
	str r4,[r0,#8]		
	
	pop {r4,r5,r6,pc}


.align 4
P_3PI2:		.word (4*3*512)/2
P_4PI3:		.word 4*4*512/3
P_2PI3:		.word 4*2*512/3
P_4PI33PI2:	.word 4*4*512/3+4*3*512/2
P_2PI33PI2:	.word 4*2*512/3+4*3*512/2
MASK1023:   .word 4*1023

mfilter:
						@; r0=x
	ldr r1, =flt_idx
	ldr r2, [r1] 
	add r2,r2,#1
	mov r3,#(32-1)
	and r2,r3	 		@; r2 = i
	str r2, [r1]		
		
	ldr r1, =flt_buf
	lsl r2,#2
	add r1, r1, r2		 @; r1=&b[i]	
	ldr r2,[r1]			 @; r2=b[i](old)
	str r0,[r1]			 @; b[i](new)=x		
	sub r0,r0,r2		 @; r0=x-b[i]				

	ldr r1, =flt_acm
	ldr r2,[r1]
	add r0,r0,r2		 @ r0 = a+x-b[i](old)	
	str r0,[r1]
	asr r0,r0,#5		 @ r0 = ( a+x-b[i](old) )/32
	
	mov pc,lr

dot3:
	push {r7,lr}
	
	ldr r2,[r0,#0]
	ldr r3,[r1,#0]
	mul r2,r2,r3
	mov r7,r2
	
	ldr r2,[r0,#4]
	ldr r3,[r1,#4]
	mul r2,r2,r3	
	add r7,r7,r2
	
	ldr r0,[r0,#8]
	ldr r3,[r1,#8]
	mul r0,r0,r3	
	add r0,r7,r0	
	
	pop {r7,pc}
