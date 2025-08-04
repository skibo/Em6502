	processor 6502

;;; illgl.asm - test illegal instructions.

WSYNC		equ	$02

RomStart	equ	$F000
RomEnd		equ	$FFFF
IntVectors	equ	$FFFA

	org	RomStart

Cart_Init
	sei
	cld
	ldx	#$FF
	txs

	;; Test NOPs
	byte	$1A		; implied
	byte	$3A
	byte	$5A
	byte	$7A
	byte	$DA
	byte	$FA

	byte	$80, $52	; immediate
	byte	$82, $52
	byte	$89, $52
	byte	$C2, $52
	byte	$E2, $52

	byte	$04, $80	; zero page
	byte	$44, $80
	byte	$64, $80
	byte	$14, $80	; zero page,X
	byte	$34, $80
	byte	$54, $80
	byte	$74, $80
	byte	$D4, $80
	byte	$F4, $80

	byte	$0C, $80, $00	; absolute
	byte	$1C, $80, $00	; absolute,X
	byte	$3C, $80, $00
	byte	$5C, $80, $00
	byte	$7C, $80, $00
	byte	$DC, $80, $00
	byte	$FC, $80, $00

;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; ANC, ASR ARR AXS SBC - all immedates
;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	lda	#$AA		; ANC
	anc	#$F0
	bcc	*
	cmp	#$A0
	bne	*

	lda	#$55		; ANC
	anc	#$F0
	bcs	*
	cmp	#$50
	bne	*

	lda	#$AA
	asr	#$F0		; ASR/ALR
	bcs	*
	cmp	#$50
	bne	*

	sec			; ARR
	lda	#$AA
	arr	#$F0
	bcc	*
	cmp	#$D0
	bne	*

	clc
	lda	#$f0
	ldx	#$55
	sbx	#$01		; AXS/SBX
	bcc	*
	cpx	#$4F
	bne	*

;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; SAX LAX - simple but more address modes
;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	lda	#$55		; SAX zp
	ldx	#$0f
	sax	$80
	ldy	$80
	cpy	#$05
	bne	*

	lax	$80		; LAX zp
	cmp	#$05
	bne	*
	cpx	#$05
	bne	*

	ldy	#$69		; SAX zp,y
	ldx	#$ee
	lda	#$f0
	sax	$20,y
	ldy	$89
	cpy	#$e0
	bne	*

	ldy	#$88		; LAX zp,y
	lax	$01,y
	cpx	#$e0
	bne	*
	cmp	#$e0
	bne	*

	lda	#$AA		; SAX absolute
	ldx	#$F0
	sax	$1234
	ldy	$1234
	cpy	#$A0
	bne	*

	lax	$1234		; LAX absolute
	cmp	#$A0
	bne	*
	cpx	#$A0
	bne	*

	ldy	#$32		; LAX absolute,Y
	sty	$1234
	lax	$1202,Y
	cmp	#$32
	bne	*
	cpx	#$32
	bne	*

	lda	#$23		; indirect pointer
	sta	$91
	lda	#$45
	sta	$90

	ldy	#$77		; LAX (ind,X)
	sty	$2345
	ldx	#$0F
	lax	($81,X)
	cmp	#$77
	bne	*
	cpx	#$77
	bne	*

	lda	#$99		; SAX (ind,X)
	ldx	#$70
	sax	($20,X)
	lda	$2345
	cmp	#$10
	bne	*

	ldy	#$55		; LAX (ind),Y
	sty	$239A
	lax	($90),Y
	cmp	#$55
	bne	*
	cpx	#$55
	bne	*

	ldy	#$f0		; LAX (ind),Y  (w pagedelay)
	sty	$2435
	lax	($90),Y
	cmp	#$f0
	bne	*
	cpx	#$f0
	bne	*

;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; SLO RLA SRE RRA DCP ISC - RMW with side efects
;;;    all address modes
;;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	lda	#$55		; SLO zp
	sta	$80
	lda	#$0f
	slo	$80
	bcs	*
	cmp	#$af
	bne	*
	ldy	$80
	cpy	#$AA
	bne	*

	sec
	lda	#$88		; RLA zp
	sta	$80
	lda	#$01
	rla	$80
	bcc	*
	cmp	#$01
	bne	*
	ldy	$80
	cpy	#$11
	bne	*

	lda	#$69		; SRE zp
	sta	$80
	lda	#$ff
	sre	$80
	bcc	*
	cmp	#$CB
	bne	*

	sec			; RRA zp
	lda	#$44
	sta	$80
	lda	#$55
	rra	$80
	bcs	*
	cmp	#$F7
	bne	*

	lda	#$6a		; DCP zp
	sta	$80
	lda	#$69
	dcp	$80
	bne	*
	bcc	*
	ldy	$80
	cpy	#$69
	bne	*

	clc
	lda	#$99		; ISC/ISB zp
	sta	$80
	lda	#$11
	isb	$80
	bcs	*
	cmp	#$76
	bne	*
	ldy	$80
	cpy	#$9A
	bne	*

	lda	#$AA		; SLO zp,X
	sta	$80
	ldx	#$30
	lda	#$F0
	slo	$50,X
	bcc	*
	cmp	#$F4
	bne	*
	ldy	$80
	cpy	#$54
	bne	*

	clc
	lda	#$44		; RLA zp,X
	sta	$80
	lda	#$90
	ldx	#$48
	rla	$38,X
	bcs	*
	cmp	#$80
	bne	*
	ldy	$80
	cpy	#$88
	bne	*

	lda	#$f4		; SRE zp,X
	sta	$80
	lda	#$92
	ldx	#$20
	sre	$60,X
	bcs	*
	cmp	#$E8
	bne	*
	ldy	$80
	cpy	#$7A
	bne	*

	clc			; RRA zp,X
	lda	#$a1
	sta	$80
	lda	#$14
	ldx	#$20
	rra	$60,X
	bcs	*
	cmp	#$65
	bne	*
	ldy	$80
	cpy	#$50
	bne	*

	lda	#$00		; DCP zp,X
	sta	$80
	lda	#$FF
	ldx	#$F0
	dcp	$90,X
	bne	*
	dcp	$90,X
	beq	*
	ldy	$80
	cpy	#$FE
	bne	*

	clc			; ISC/ISB zp,X
	lda	#$44
	sta	$80
	lda	#$50
	ldx	#$80
	isb	$00,X
	bcc	*
	cmp	#$0a
	bne	*
	ldy	$80
	cpy	#$45
	bne	*

	lda	#$99		; SLO absolute
	sta	$3333
	lda	#$0c
	slo	$3333
	bcc	*
	cmp	#$3e
	bne	*
	ldy	$3333
	cpy	#$32
	bne	*

	sec			; RLA absolute
	lda	#$f0
	sta	$3333
	lda	#$fe
	rla	$3333
	bcc	*
	cmp	#$e0
	bne	*
	ldy	$3333
	cpy	#$e1
	bne	*

	lda	#$11		; SRE absolute
	sta	$3333
	lda	#$88
	sre	$3333
	bcc	*
	cmp	#$80
	bne	*
	ldy	$3333
	cpy	#$08
	bne	*

	sec			; RRA absolute
	lda	#$e2
	sta	$3333
	lda	#$15
	rra	$3333
	bcc	*
	cmp	#$06
	bne	*
	ldy	$3333
	cpy	#$f1
	bne	*

	lda	#$55		; DCP absolute
	sta	$3333
	lda	#$54
	dcp	$3333
	bne	*
	dcp	$3333
	beq	*
	ldy	$3333
	cpy	#$53
	bne	*

	clc			; ISC/ISB absolute
	lda	#$66
	sta	$3333
	lda	#$22
	isb	$3333
	bcs	*
	cmp	#$ba
	bne	*
	ldy	$3333
	cpy	#$67
	bne	*

	lda	#$99		; SLO absolute,X
	sta	$3333
	ldx	#$33
	lda	#$0c
	slo	$3300,X
	bcc	*
	cmp	#$3e
	bne	*
	ldy	$3333
	cpy	#$32
	bne	*

	sec			; RLA absolute,X
	lda	#$f0
	sta	$3333
	ldx	#$88
	lda	#$fe
	rla	$32ab,X
	bcc	*
	cmp	#$e0
	bne	*
	ldy	$3333
	cpy	#$e1
	bne	*

	lda	#$11		; SRE absolute,X
	sta	$3333
	ldx	#$33
	lda	#$88
	sre	$3300,X
	bcc	*
	cmp	#$80
	bne	*
	ldy	$3333
	cpy	#$08
	bne	*

	sec			; RRA absolute,X
	lda	#$e2
	sta	$3333
	ldx	#$88
	lda	#$15
	rra	$32ab,X
	bcc	*
	cmp	#$06
	bne	*
	ldy	$3333
	cpy	#$f1
	bne	*

	lda	#$55		; DCP absolute,X
	sta	$3333
	ldx	#$33
	lda	#$54
	dcp	$3300,X
	bne	*
	ldx	#$88
	dcp	$32ab,X
	beq	*
	ldy	$3333
	cpy	#$53
	bne	*

	clc			; ISC/ISB absolute,X
	lda	#$66
	sta	$3333
	ldx	#$33
	lda	#$22
	isb	$3300,X
	bcs	*
	cmp	#$ba
	bne	*
	ldy	$3333
	cpy	#$67
	bne	*


	lda	#$99		; SLO absolute,Y
	sta	$3333
	ldy	#$33
	lda	#$0c
	slo	$3300,Y
	bcc	*
	cmp	#$3e
	bne	*
	ldy	$3333
	cpy	#$32
	bne	*

	sec			; RLA absolute,Y
	lda	#$f0
	sta	$3333
	ldy	#$88
	lda	#$fe
	rla	$32ab,Y
	bcc	*
	cmp	#$e0
	bne	*
	ldy	$3333
	cpy	#$e1
	bne	*

	lda	#$11		; SRE absolute,Y
	sta	$3333
	ldy	#$33
	lda	#$88
	sre	$3300,Y
	bcc	*
	cmp	#$80
	bne	*
	ldy	$3333
	cpy	#$08
	bne	*

	sec			; RRA absolute,Y
	lda	#$e2
	sta	$3333
	ldy	#$88
	lda	#$15
	rra	$32ab,Y
	bcc	*
	cmp	#$06
	bne	*
	ldy	$3333
	cpy	#$f1
	bne	*

	lda	#$55		; DCP absolute,Y
	sta	$3333
	ldy	#$33
	lda	#$54
	dcp	$3300,Y
	bne	*
	ldy	#$88
	dcp	$32ab,Y
	beq	*
	ldy	$3333
	cpy	#$53
	bne	*

	clc			; ISC/ISB absolute,Y
	lda	#$66
	sta	$3333
	ldy	#$33
	lda	#$22
	isb	$3300,Y
	bcs	*
	cmp	#$ba
	bne	*
	ldy	$3333
	cpy	#$67
	bne	*


	lda	#$24		; for indirects
	sta	$91
	lda	#$68
	sta	$90

	lda	#$F1		; SLO (ind,X)
	sta	$2468
	ldx	#8
	lda	#$0F
	slo	($88,X)
	bcc	*
	cmp	#$ef
	bne	*
	ldy	$2468
	cpy	#$e2
	bne	*

	sec
	lda	#$88		; RLA (ind,X)
	sta	$2468
	lda	#$01
	ldx	#$f8
	rla	($98,X)
	bcc	*
	cmp	#$01
	bne	*
	ldy	$2468
	cpy	#$11
	bne	*

	lda	#$69		; SRE (ind,X)
	sta	$2468
	lda	#$ff
	ldx	#8
	sre	($88,X)
	bcc	*
	cmp	#$cb
	bne	*

	sec			; RRA (ind,X)
	lda	#$44
	sta	$2468
	lda	#$55
	ldx	#$f8
	rra	($98,X)
	bcs	*
	cmp	#$f7
	bne	*
	ldy	$2468
	cpy	#$a2
	bne	*

	lda	#$6a		; DCP (ind,X)
	sta	$2468
	lda	#$69
	ldx	#8
	dcp	($88,X)
	bne	*
	bcc	*
	ldy	$2468
	cpy	#$69
	bne	*

	clc			; ISC/ISB (ind,x)
	lda	#$99
	sta	$2468
	lda	#$11
	ldx	#8
	isb	($88,X)
	bcs	*
	cmp	#$76
	bne	*
	ldy	$2468
	cpy	#$9a
	bne	*

	lda	#$f1		; SLO (ind),Y
	sta	$2499
	ldy	#$31
	lda	#$0f
	slo	($90),Y
	bcc	*
	cmp	#$ef
	bne	*
	ldy	$2499
	cpy	#$e2
	bne	*

	sec			; RLA (ind),Y
	lda	#$88
	sta	$2558
	ldy	#$f0
	lda	#$01
	rla	($90),Y
	bcc	*
	cmp	#$01
	bne	*
	ldy	$2558
	cpy	#$11
	bne	*

	lda	#$69		; SRE (ind),Y
	sta	$2499
	lda	#$ff
	ldy	#$31
	sre	($90),Y
	bcc	*
	cmp	#$cb
	bne	*

	sec			; RRA (ind),Y
	lda	#$44
	sta	$2558
	lda	#$55
	ldy	#$f0
	rra	($90),Y
	bcs	*
	cmp	#$f7
	bne	*
	ldy	$2558
	cpy	#$a2
	bne	*

	lda	#$6a		; DCP (ind),Y
	sta	$2499
	lda	#$69
	ldy	#$31
	dcp	($90),Y
	bne	*
	bcc	*
	ldy	$2499
	cpy	#$69
	bne	*

	clc			; ISC/ISB (ind),Y
	lda	#$99
	sta	$2558
	lda	#$11
	ldy	#$f0
	isb	($90),Y
	bcs	*
	cmp	#$76
	bne	*
	ldy	$2558
	cpy	#$9a
	bne	*

	jmp	done

	ORG     $F888
done:
	jmp	done

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
				;
				; Set up the 6502 interrupt vector table
				;
	ORG	IntVectors
NMI		dc.w	  Cart_Init
Reset		dc.w	  Cart_Init
IRQ		dc.w	  Cart_Init
