# z80 disassembly
_chdir:
	LD HL,0x0002                   ; 0000: 21 02 00           !..   
	ADD HL,SP                      ;       39                 9     
	LD A,(HL)                      ;       7e                 ~     
	INC HL                         ;       23                 #     
	LD H,(HL)                      ;       66                 f     
	LD L,A                         ;       6f                 o     
	LD (0x001a),HL                 ;       22 1a 00           "..   
	SYS INDIR 0x0018               ;       cf 00 18 00        ....  
	LD BC,0x0000                   ;       01 00 00           ...   
	RET NC                         ;       d0                 .     
	DEC BC                         ;       0b                 .     
	LD (_errno),HL                 ;       22 00 00           "..   
	RET                            ;       c9                 .     
	DB 0xcf                        ; 0018: cf                 .     
	DB 0xc                         ;       0c                 .     
	DB 0x0                         ; 001a: 00                 .     
	DB 0x0                         ;       00                 .     
