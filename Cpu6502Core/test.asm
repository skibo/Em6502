; test.asm
;

	processor 6502

	org	$f000

	byte	$80, $4c
	byte	$80, $4c

	jmp	*
