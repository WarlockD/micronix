global	_main
global	ncsv, cret, indir
global	_printf
psect	text
_main:
ld	hl,19f
push	hl
call	_printf
pop	bc
ret	
global	_foobiebletch
global	_v
global	_k
_foobiebletch:
ld	hl,(_k)
push	hl
ld	hl,_v+4
push	hl
ld	hl,29f
push	hl
call	_printf
pop	bc
pop	bc
pop	bc
ret	
psect	data
19:
defb	104,101,108,108,111,44,32,119,111,114,108,100,10,0
29:
defb	37,115,32,37,100,10,0
psect	bss
_v:
defs	39
_k:
defs	2
psect	text
                                                                