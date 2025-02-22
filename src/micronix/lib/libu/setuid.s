;
; assembly source for setuid system call
;
; /usr/src/lib/libu/setuid.s
;
; Changed: <2023-07-07 00:36:28 curt>
;
; vim: tabstop=8 shiftwidth=8 noexpandtab:
;

setuid.o:
    0    _errno: 0000 08 global 
    1   _setuid: 0000 0d global defined code 
0000: ld hl,0x2                      ; 21 02 00       !..  
0003: add hl,sp                      ; 39             9    
0004: ld a,(hl)                      ; 7e             ~    
0005: inc hl                         ; 23             #    
0006: ld h,(hl)                      ; 66             f    
0007: ld l,a                         ; 6f             o    
0008: sys setuid                     ; cf 17          ..   
000a: ld bc,0x0                      ; 01 00 00       ...  
000d: ret nc                         ; d0             .    
000e: dec bc                         ; 0b             .    
000f: ld (0x0),hl                    ; 22 00 00       "..  
0012: ret                            ; c9             .    
