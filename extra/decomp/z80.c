/*
 * vim: set tabstop=4 shiftwidth=4 expandtab:
 */
#include "decomp.h"

char *reg8[] = { "b", "c", "d", "e", "h", "l", "(hl)", "a" };
char *rp[] = { "bc", "de", "hl", "sp" };
char *rp1[] = { "bc", "de", "hl", "af" };
char *op8[] = { "add", "adc", "sub", "sbc", "and", "xor ", "or", "cp" };
char *indreg[] = { "ix", "iy" };
char *condition[] = { "nz", "z", "nc", "c", "v", "nv", "p", "m" };
char *rot[] = { "rlc", "rrc", "rl", "rr", "sla", "sra", "sll", "srl" };
char *regname[] = {
    "b", "c", "d", "e", "h", "l", "a", 
    "bc", "de", "hl", "ix", "iy", "sp", "af",
    "zflag", "cflag", "vflag", "mflag"
};

/* translation table from register selector to register number */
char rt8[] = { breg, creg, dreg, ereg, hreg, lreg, -1, areg };
char rt16[] = { bcreg, dereg, hlreg, spreg };
char rtsp[] = { bcreg, dereg, hlreg, afreg };

/*
 * crack an instruction
 * returns: 1 if it is an unconditionitional jump
 * returns: 0 if the next instruction is part of a flow
 */
int
do_instr(struct inst *i)
{
    unsigned short pc;
    char *buf;

    unsigned char opcode;
    unsigned char arg8;
    unsigned short arg16;
    unsigned char index;

    unsigned char r2;           /* bc de hl sp,af */
    unsigned char sr;           /* bcdehl.a */
    unsigned char dr;           /* bcdehl.a */
    unsigned char ir;           /* 0 = ix, 1 = iy */
    struct expr *e;

    pc = i->addr;
    buf = i->dis;

    opcode = getmem(pc);
    arg8 = getmem(pc + 1);
    arg16 = getmem(pc + 1) + (getmem(pc + 2) << 8);

    sr = opcode & 0x7;
    dr = (opcode >> 3) & 0x7;
    r2 = (opcode >> 4) & 0x3;
    ir = (opcode >> 5) & 0x1;

    sprintf(buf, "-----------undef %x", opcode);
    i->len = 1;
    i->opcode = opcode;

    switch (opcode) {
    case 0x00:
        sprintf(buf, "nop");
        *buf= '\0';
        return 0;
    case 0x07:
        sprintf(buf, "rlca");
        i->mop[0] = 
            expr(assign, 0, 0, 
                expr(reg, E_BYTE, areg, 0, 0),
                expr(lshift, E_BYTE|E_FLAGS, 0,
                    expr(reg, E_BYTE, areg, 0, 0),
                    expr(constant, 0, 1, 0, 0)));
        return 0;
#ifdef notdef
    case 0x08:
        sprintf(buf, "ex af,af'");
        return 0;
#endif
    case 0x0F:
        sprintf(buf, "rrca");
        i->mop[0] = 
            expr(assign, 0, 0, 
                expr(reg, E_BYTE, areg, 0, 0),
                expr(rshift, E_BYTE|E_FLAGS, 0,
                    expr(reg, E_BYTE, areg, 0, 0),
                    expr(constant, 0, 1, 0, 0)));
        return 0;
    case 0x10:
        sprintf(buf, "djnz %", reladdr(pc, arg8));
        i->mop[0] = 
            expr(assign, 0, 0, 
                expr(reg, E_BYTE, breg, 0, 0),
                expr(sub, E_BYTE|E_FLAGS, 0,
                    expr(reg, E_BYTE, breg, 0, 0),
                    expr(constant, 0, 1, 0, 0)));
        i->mop[1] = 
            expr(cond, 0, 0,
                expr(reg, E_BIT, zflag, 0, 0),
                expr(jump, E_WORD, 0, 
                    expr(constant, 0, reloff(pc, arg8), 0, 0), 0));
        i->len = 2;
        return 0;
    case 0x17:
        sprintf(buf, "rla");
        i->mop[0] = 
            expr(assign, 0, 0, 
                expr(reg, E_BYTE, areg, 0, 0),
                expr(lshift, E_BYTE|E_FLAGS, 0,
                    expr(reg, E_BYTE, areg, 0, 0),
                    expr(constant, 0, 1, 0, 0)));
        return 0;
    case 0x18:
        sprintf(buf, "jr %s", reladdr(pc, arg8));
        i->mop[0] = expr(jump, E_WORD, 0, 
                        expr(constant, 0, reloff(pc, arg8), 0, 0), 0);
        i->len = 2;
        return 1;
    case 0x1F:
        sprintf(buf, "rra");
        i->mop[0] = 
            expr(assign, 0, 0, 
                expr(reg, E_BYTE, areg, 0, 0),
                expr(rshift, E_BYTE|E_FLAGS, 0,
                    expr(reg, E_BYTE, areg, 0, 0),
                    expr(constant, 0, 1, 0, 0)));
        return 0;
    case 0x20:
        sprintf(buf, "jr nz,%s", reladdr(pc, arg8));
        i->mop[0] = 
            expr(cond, 0, 0,
                expr(not, E_BIT, 0, expr(reg, E_BIT, zflag, 0, 0), 0),
                expr(jump, E_WORD, 0, 
                        expr(constant, 0, reloff(pc, arg8), 0, 0), 0));
        i->len = 2;
        return 0;
#ifdef notdef
    case 0x27:
        sprintf(buf, "daa;");
        return 0;
#endif notdef
    case 0x28:
        sprintf(buf, "jr z,%s", reladdr(pc, arg8));
        i->mop[0] = 
            expr(cond, 0, 0,
                expr(reg, E_BIT, zflag, 0, 0),
                expr(jump, E_WORD, 0, 
                        expr(constant, 0, reloff(pc, arg8), 0, 0),0));
        i->len = 2;
        return 0;
    case 0x2F:
        sprintf(buf, "cpl a");
        i->mop[0] = 
            expr(assign, 0, 0, 
                expr(reg, E_BYTE, areg, 0, 0),
                expr(not, E_BYTE|E_FLAGS, 0,
                    expr(reg, E_BYTE, areg, 0, 0), 0));
        return 0;
    case 0x30:
        sprintf(buf, "jr nc,%s", reladdr(pc, arg8));
        i->mop[0] = 
            expr(cond, 0, 0,
                expr(not, E_BIT, 0, expr(reg, E_BIT, cflag, 0, 0), 0),
                expr(jump, E_WORD, 0, 
                        expr(constant, 0, reloff(pc, arg8), 0, 0), 0));
        i->len = 2;
        return 0;
    case 0x37:
        sprintf(buf, "scf");
        i->mop[0] = 
            expr(assign, 0, 0, 
                expr(reg, E_BIT, cflag, 0, 0),
                expr(constant, E_BIT, 1, 0, 0));
        return 0;
    case 0x38:
        sprintf(buf, "jr c,%s", reladdr(pc, arg8));
        i->mop[0] = 
            expr(cond, 0, 0,
                expr(reg, E_BIT, cflag, 0, 0),
                expr(jump, E_WORD, 0, 
                        expr(constant, 0, reloff(pc, arg8), 0, 0), 0));
        i->len = 2;
        return 0;
    case 0x3F:
        sprintf(buf, "ccf");
        i->mop[0] = 
            expr(assign, 0, 0, 
                expr(reg, E_BIT, cflag, 0, 0),
                expr(not, E_BIT, 0, expr(reg, E_BIT, cflag, 0, 0), 0));
        return 0;
    case 0x22:
        sprintf(buf, "ld (%s),hl", addr(arg16));
        i->mop[0] = 
            expr(assign, 0, 0, 
                expr(memory, E_WORD, 0,
                    expr(constant, 0, arg16, 0, 0), 
                    0),
                expr(reg, E_WORD, hlreg, 0, 0));
        i->len = 3;
        return 0;
    case 0x2a:
        sprintf(buf, "ld hl,(%s)", addr(arg16));
        i->mop[0] = 
            expr(assign, 0, 0, 
                expr(reg, E_WORD, hlreg, 0, 0),
                expr(memory, E_WORD, 0,
                    expr(constant, 0, arg16, 0, 0), 
                    0));
        i->len = 3;
        return 0;
    case 0x32:
        sprintf(buf, "ld (%s),a", addr(arg16));
        i->mop[0] = 
            expr(assign, 0, 0, 
                expr(memory, E_BYTE, 0,
                    expr(constant, 0, arg16, 0, 0), 
                    0),
                expr(reg, E_BYTE, areg, 0, 0));
        i->len = 3;
        return 0;
    case 0x3a:
        sprintf(buf, "ld a,(%s)", addr(arg16));
        i->mop[0] = 
            expr(assign, 0, 0, 
                expr(reg, E_BYTE, areg, 0, 0),
                expr(memory, E_BYTE, 0,
                    expr(constant, 0, arg16, 0, 0), 
                    0));
        i->len = 3;
        return 0;
    case 0xc3:
        sprintf(buf, "jp %s;", addr(arg16));
        addlabel(arg16, 0);
        xref(arg16, i->addr);
        i->mop[0] = expr(jump, E_WORD, 0, 
                        expr(constant, 0, arg16, 0, 0), 0);
        i->len = 3;
        return 1;
    case 0xc9:
        sprintf(buf, "ret");
        i->mop[0] = expr(ret, E_WORD, 0, 0, 0);
        return 1;
    case 0xcd:
        sprintf(buf, "call %s", addr(arg16));
        i->len = 3;
        addlabel(arg16, 0);
        xref(arg16, i->addr);
        i->mop[0] = expr(call, E_WORD, 0, 
                        expr(constant, 0, arg16, 0, 0), 0);
        return 0;
    case 0x76:
        sprintf(buf, "halt");
        return 1;
#ifdef notdef
    case 0xd3:
        sprintf(buf, "ld io8(%s),a;", const8(arg8));
        i->len = 2;
        return 2;
    case 0xd9:
        sprintf(buf, "exx;");
        return 1;
    case 0xdb:
        sprintf(buf, "ld a,io8(%s);", const8(arg8));
        i->len = 2;
        return 2;
    case 0xe3:
        sprintf(buf, "ex (sp),hl;");
        return 1;
#endif
    case 0xe9:
        sprintf(buf, "jp (hl);");
        i->mop[0] = expr(jump, E_WORD, 0, 
                        expr(reg, E_WORD, hlreg, 0, 0), 0);
        return 1;
#ifdef notdef
    case 0xeb:
        sprintf(buf, "ex de,hl;");
        return 1;
    case 0xf3:
        sprintf(buf, "di;");
        return 1;
#endif
    case 0xf9:
        sprintf(buf, "ld sp, hl");
        i->mop[0] = 
            expr(assign, E_WORD, 0, 
                expr(reg, E_WORD, spreg, 0, 0),
                expr(reg, E_WORD, hlreg, 0, 0));
        return 0;
#ifdef notdef
    case 0xfb:
        sprintf(buf, "ei;");
        return 1;
#endif
    default:
        break;
    }

    if ((opcode & 0xcf) == 0x01) {
        sprintf(buf, "ld %s,%s", rp[r2], const16(arg16));
        i->mop[0] = 
            expr(assign, E_WORD, 0, 
                expr(reg, E_WORD, rt16[r2], 0, 0),
                expr(constant, 0, arg16, 0, 0));
        i->len = 3;
        return 0;
    }
    if ((opcode & 0xcf) == 0x02) {
        sprintf(buf, "ld (%s), a", rp[r2]);
        i->mop[0] = 
            expr(assign, 0, 0, 
                expr(memory, E_BYTE, 0,
                    expr(reg, E_WORD, rt16[r2], 0, 0), 
                    0),
                expr(reg, E_BYTE, areg, 0, 0));
        return 0;
    }
    if ((opcode & 0xcf) == 0x03) {
        sprintf(buf, "inc %s", rp[r2]);
        i->mop[0] = 
            expr(assign, 0, 0, 
                expr(reg, E_WORD, rt16[r2], 0, 0),
                expr(add, E_WORD, 0,
                    expr(reg, E_WORD, rt16[r2], 0, 0),
                    expr(constant, 0, 1, 0, 0)));
        return 0;
    }
    if ((opcode & 0xc7) == 0x04) {
        sprintf(buf, "inc %s", reg8[dr]);
        if (dr == 6) {
            i->mop[0] = 
                expr(assign, 0, 0, 
                    expr(memory, E_BYTE, 0,
                        expr(reg, E_WORD, hlreg, 0, 0), 0),
                    expr(add, E_BYTE|E_FLAGS, 0,
                        expr(memory, E_BYTE, 0,
                            expr(reg, E_WORD, hlreg, 0, 0), 0),
                        expr(constant, 0, 1, 0, 0)));
        } else {
            i->mop[0] = 
                expr(assign, 0, 0, 
                    expr(reg, E_BYTE, rt8[dr], 0, 0),
                    expr(add, E_BYTE|E_FLAGS, 0,
                        expr(reg, E_BYTE, rt8[dr], 0, 0),
                        expr(constant, 0, 1, 0, 0)));
        }
        return 0;
    }
    if ((opcode & 0xc7) == 0x05) {
        sprintf(buf, "dec %s", reg8[dr]);
        if (dr == 6) {
            i->mop[0] = 
                expr(assign, 0, 0, 
                    expr(memory, E_BYTE, 0,
                        expr(reg, E_WORD, hlreg, 0, 0), 0),
                    expr(sub, E_BYTE|E_FLAGS, 0,
                        expr(memory, E_BYTE, 0,
                            expr(reg, E_WORD, hlreg, 0, 0), 0),
                        expr(constant, 0, 1, 0, 0)));
        } else {
            i->mop[0] = 
                expr(assign, 0, 0, 
                    expr(reg, E_BYTE, rt8[dr], 0, 0),
                    expr(sub, E_BYTE|E_FLAGS, 0,
                        expr(reg, E_BYTE, rt8[dr], 0, 0),
                        expr(constant, 0, 1, 0, 0)));
        }
        return 0;
    }
    if ((opcode & 0xc7) == 0x06) {
        sprintf(buf, "ld %s,%s", reg8[dr], const8(arg8));
        if (dr == 6) {
            i->mop[0] = 
                expr(assign, 0, 0, 
                    expr(memory, E_BYTE, 0,
                        expr(reg, E_WORD, hlreg, 0, 0), 0),
                    expr(constant, 0, 1, 0, 0));
        } else {
            i->mop[0] = 
                expr(assign, 0, 0, 
                    expr(reg, E_BYTE, rt8[dr], 0, 0),
                expr(constant, E_BYTE, arg8, 0, 0));
        }
        i->len = 2;
        return 0;
    }
    if ((opcode & 0xcf) == 0x09) {
        sprintf(buf, "add hl,%s", rp[r2]);
        i->mop[0] = 
            expr(assign, 0, 0, 
                expr(reg, E_WORD, hlreg, 0, 0),
                expr(add, E_WORD, 0,
                    expr(reg, E_WORD, hlreg, 0, 0),
                    expr(reg, E_WORD, rt16[r2], 0, 0)));
        return 0;
    }
    if ((opcode & 0xcf) == 0x0a) {
        sprintf(buf, "ld a,(%s)", rp[r2]);
        i->mop[0] = 
            expr(assign, 0, 0, 
                expr(reg, E_BYTE, areg, 0, 0),
                expr(memory, E_BYTE, 0,
                    expr(reg, E_WORD, rt16[r2], 0, 0), 
                    0));
        return 0;
    }
    if ((opcode & 0xcf) == 0x0b) {
        sprintf(buf, "dec %s", rp[r2]);
        i->mop[0] = 
            expr(assign, 0, 0, 
                expr(reg, E_WORD, rt16[r2], 0, 0),
                expr(sub, E_WORD, 0,
                    expr(reg, E_WORD, rt16[r2], 0, 0),
                    expr(constant, 0, 1, 0, 0)));
        return 0;
    }
#ifdef notdef
    if ((opcode & 0xc7) == 0xc0) {
        sprintf(buf, "if %s { ret; };", condition[dr]);
        return 1;
    }
#endif
    if ((opcode & 0xcf) == 0xc1) {
        sprintf(buf, "pop %s;", rp1[r2]);
        i->mop[0] = 
            expr(pop, 0, 0, expr(reg, E_WORD, rtsp[r2], 0, 0), 0);
        return 0;
    }
#ifdef notdef
    if ((opcode & 0xc7) == 0xc2) {
        sprintf(buf, "if %s { jp %s; };", condition[dr], addr(arg16));
        i->len = 3;
        return 3;
    }
    if ((opcode & 0xc7) == 0xc4) {
        sprintf(buf, "if %s { call %s; };",
            condition[dr], addr(arg16));
        i->len = 3;
        return 3;
    }
#endif
    if ((opcode & 0xcf) == 0xc5) {
        sprintf(buf, "push %s;", rp1[r2]);
        i->mop[0] = 
            expr(push, 0, 0, expr(reg, E_WORD, rtsp[r2], 0, 0), 0);
    }
#ifdef notdef
    if ((opcode & 0xc7) == 0xc6) {
        sprintf(buf, "a %s %s;", op8[dr], const8(arg8));
        i->len = 2;
        return 2;
    }
    if ((opcode & 0xc7) == 0xc7) {
        sprintf(buf, "call %s;", addr(dr * 8));
        sp = special(arg16);
        if (sp->extra) {
            i->len = 3 + sp->extra;
        } else {
            i->len = 3;
        return 0;
    }
#endif
    if ((opcode & 0xc0) == 0x40) {
        sprintf(buf, "ld %s,%s;", reg8[dr], reg8[sr]);
        if (dr == 6) {
            i->mop[0] = 
                expr(assign, 0, 0, 
                    expr(memory, E_BYTE, 0,
                        expr(reg, E_WORD, hlreg, 0, 0), 0),
                    expr(reg, 0, rt8[sr], 0, 0));
        } else if (sr == 6) {
            i->mop[0] = 
                expr(assign, 0, 0, 
                    expr(reg, 0, rt8[dr], 0, 0),
                    expr(memory, E_BYTE, 0,
                        expr(reg, E_WORD, hlreg, 0, 0), 0));
        } else {
            i->mop[0] = 
                expr(assign, 0, 0, 
                    expr(reg, E_BYTE, rt8[dr], 0, 0),
                    expr(reg, E_BYTE, rt8[sr], 0, 0));
        }
        return 0;
    }
    if ((opcode & 0xc0) == 0x80) {
        sprintf(buf, "%s a,%s", op8[dr], reg8[sr]);
        if (sr == 6) {
            e = expr(memory, E_BYTE, 0,
                expr(reg, E_WORD, hlreg, 0, 0), 0);
        } else {
            e = expr(reg, 0, rt8[sr], 0, 0);
        }
        switch (dr) {
            case 0:
                 e = expr(add, E_BYTE|E_FLAGS, 0,
                    expr(reg, E_BYTE, areg, 0, 0), e);
                break;
            case 1:
                 e = expr(add, E_BYTE|E_FLAGS, 0,
                    expr(reg, E_BYTE, areg, 0, 0),
                    expr(add, E_BYTE, 0,
                        expr(reg, E_BIT, cflag, 0, 0),
                        e));
                break;
            case 2:
                 e = expr(sub, E_BYTE|E_FLAGS, 0,
                    expr(reg, E_BYTE, areg, 0, 0), e);
                break;
            case 3:
                 e = expr(sub, E_BYTE|E_FLAGS, 0,
                    expr(reg, E_BYTE, areg, 0, 0),
                    expr(add, E_BYTE, 0,
                        expr(reg, E_BIT, cflag, 0, 0),
                        e));
                break;
            default:
                break;
        }
        i->mop[0] = 
            expr(assign, 0, 0, expr(reg, E_BYTE, areg, 0, 0), e);
        return 0;
    }
    if (opcode == 0xed) {
        opcode = getmem(pc + 1);
        i->opcode |= (opcode << 8);
        r2 = (opcode >> 4) & 0x3;
        arg16 = getmem(pc + 2) + (getmem(pc + 3) << 8);
        i->len = 2;
        if ((opcode & 0xcf) == 0x42) {
            sprintf(buf, "sbc hl,%s", rp[r2]);
            i->mop[0] = 
                expr(assign, 0, 0, 
                    expr(reg, E_WORD, hlreg, 0, 0),
                expr(sub, E_WORD, 0,
                    expr(reg, E_WORD, hlreg, 0, 0),
                    expr(add, E_WORD, 0,
                        expr(reg, E_BIT, cflag, 0, 0),
                        expr(reg, E_WORD, rt16[r2], 0, 0))));
            return 0;
        }
        if ((opcode & 0xcf) == 0x43) {
            sprintf(buf, "ld (%s),%s", addr(arg16), rp[r2]);
            i->mop[0] = 
                expr(assign, 0, 0, 
                    expr(memory, E_WORD, 0,
                        expr(constant, 0, arg16, 0, 0), 
                        0),
                    expr(reg, E_WORD, rt16[r2], 0, 0));
            return 4;
        }
        if ((opcode & 0xcf) == 0x4a) {
            sprintf(buf, "adc hl,%s", rp[r2]);
            i->mop[0] = 
                expr(assign, 0, 0, 
                    expr(reg, E_WORD, hlreg, 0, 0),
                expr(add, E_WORD, 0,
                    expr(reg, E_WORD, hlreg, 0, 0),
                    expr(add, E_WORD, 0,
                        expr(reg, E_BIT, cflag, 0, 0),
                        expr(reg, E_WORD, rt16[r2], 0, 0))));
            return 0;
        }
        if ((opcode & 0xcf) == 0x4b) {
            sprintf(buf, "ld %s,(%s)", rp[r2], addr(arg16));
            i->mop[0] = 
                expr(assign, 0, 0, 
                    expr(reg, E_WORD, rt16[r2], 0, 0),
                    expr(memory, E_WORD, 0,
                        expr(constant, 0, arg16, 0, 0), 
                        0));
            i->len = 4;
            return 0;
        }
        switch (opcode) {
        case 0xb0:
            sprintf(buf, "ldir", addr(pc));
            i->mop[0] = 
                expr(assign, 0, 0, 
                    expr(memory, E_WORD, 0, expr(reg, E_WORD, dereg, 0, 0), 0),
                    expr(memory, E_WORD, 0, expr(reg, E_WORD, hlreg, 0, 0), 0));
            i->mop[1] = 
                expr(assign, 0, 0, 
                    expr(reg, E_WORD, dereg, 0, 0),
                    expr(add, E_WORD, 0,
                        expr(reg, E_WORD, dereg, 0, 0),
                        expr(constant, 0, 1, 0, 0)));
            i->mop[2] = 
                expr(assign, 0, 0, 
                    expr(reg, E_WORD, hlreg, 0, 0),
                    expr(add, E_WORD, 0,
                        expr(reg, E_WORD, hlreg, 0, 0),
                        expr(constant, 0, 1, 0, 0)));
            i->mop[3] = 
                expr(assign, 0, 0, 
                    expr(reg, E_WORD, bcreg, 0, 0),
                    expr(sub, E_WORD|E_FLAGS, 0,
                        expr(reg, E_WORD, bcreg, 0, 0),
                        expr(constant, 0, 1, 0, 0)));
            i->mop[4] = 
                expr(cond, 0, 0,
                    expr(reg, E_BIT, zflag, 0, 0),
                    expr(jump, E_WORD, 0, 
                        expr(constant, 0, reloff(pc, -2), 0, 0), 0));
            return 0;
        case 0x44:
            sprintf(buf, "neg");
            i->mop[0] = 
                expr(assign, 0, 0, 
                    expr(reg, E_BYTE, areg, 0, 0),
                    expr(sub, E_BYTE|E_FLAGS, 0,
                        expr(constant, 0, 0, 0, 0),
                        expr(reg, E_BYTE, areg, 0, 0)));
            return 0;
        }
        sprintf(buf, "----------undef ed %x", opcode);
        i->len = 2;
        return 0;
    }
#ifdef notdef
    if (opcode == 0xcb) {
        opcode = getmem(pc + 1);
        i->opcode |= (opcode << 8);
        sr = opcode & 0x7;
        dr = (opcode >> 3) & 0x7;
        arg8 = 1 << dr;

        switch (opcode & 0xc0) {
        case 0x00:
            break;
        case 0x80:
            sprintf(buf, "%s &= %s;", reg8[sr], const8(~arg8));
        i->len = 2;
            return 2;
        case 0xc0:
            sprintf(buf, "%s |= %s;", reg8[sr], const8(arg8));
        i->len = 2;
            return 2;
        case 0x40:
            sprintf(buf, "Z = %s & %s;", reg8[sr], const8(arg8));
        i->len = 2;
            return 2;
        }
        if (dr != 6) {
            sprintf(buf, "%s %s;", rot[dr], reg8[sr]);
        i->len = 2;
            return 2;
        }
    }
    if ((opcode & 0xdf) == 0xdd) {
        opcode = getmem(pc + 1);
        i->opcode |= (opcode << 8);
        sr = opcode & 0x7;
        dr = (opcode >> 3) & 0x7;
        r2 = (opcode >> 4) & 0x3;
        index = getmem(pc + 2);
        arg16 = getmem(pc + 2) + (getmem(pc + 3) << 8);

        switch (opcode) {
        case 0x21:
            sprintf(buf, "%s = %s;", indreg[ir], addr(arg16));
            return 4;
        case 0x22:
            sprintf(buf, "mem16(%s) = %s;", addr(arg16), indreg[ir]);
            return 4;
        case 0x2a:
            sprintf(buf, "%s = mem16(%s);", indreg[ir], addr(arg16));
            return 4;
        case 0x23:
            sprintf(buf, "%s = %s + 1;", indreg[ir], indreg[ir]);
        i->len = 2;
            return 2;
        case 0x2b:
            sprintf(buf, "%s = %s + 1;", indreg[ir], indreg[ir]);
        i->len = 2;
            return 2;
        case 0x34:
            sprintf(buf, "mem8(%s%s) = mem8(%s%s) + 1;",
                indreg[ir], signedoff(index),
                indreg[ir], signedoff(index));
        i->len = 3;
            return 3;
        case 0x35:
            sprintf(buf, "mem8(%s%s) = mem8(%s%s) - 1;",
                indreg[ir], signedoff(index),
                indreg[ir], signedoff(index));
        i->len = 3;
            return 3;
        case 0x36:
            sprintf(buf, "mem8(%s%s) = %s;",
                indreg[ir], signedoff(index),
                const8(getmem(pc + 3)));
            return 4;
        case 0xe1:
            sprintf(buf, "pop %s;", indreg[ir]);
        i->len = 2;
            return 2;
        case 0xe3:
            sprintf(buf, "ex (sp),%s;", indreg[ir]);
        i->len = 2;
            return 2;
        case 0xe5:
            sprintf(buf, "push %s;", indreg[ir]);
        i->len = 2;
            return 2;
        case 0xe9:
            sprintf(buf, "jp iy;", indreg[ir]);
        i->len = 2;
            return 2;
        case 0xf9:
            sprintf(buf, "sp = iy;", indreg[ir]);
        i->len = 2;
            return 2;
        default:
            break;
        }
        if (opcode == 0xcb) {
            opcode = getmem(pc + 3);
            i->opcode |=  (opcode << 16);
            sr = opcode & 0x7;
            dr = (opcode >> 3) & 0x7;
            index = getmem(pc + 2);
            arg8 = 1 << dr;

            switch (opcode & 0xc0) {
            case 0x00:
                break;
            case 0x80:
                sprintf(buf, "mem8(%s%s) &= %s;",
                    indreg[ir], signedoff(index),
                    const8(~arg8));
                return 4;
            case 0xc0:
                sprintf(buf, "mem8(%s%s) |= %s;",
                    indreg[ir], signedoff(index),
                    const8(arg8));
                return 4;
            case 0x40:
                sprintf(buf, "Z = mem8(%s%s) & %s;",
                    indreg[ir], signedoff(index),
                    const8(arg8));
                return 4;
            }
            if (dr != 6) {
                sprintf(buf, "%s mem8(%s%s);",
                    rot[dr], indreg[ir], signedoff(index));
                return 4;
            }
        }
        if ((opcode & 0xc7) == 0x46) {
            sprintf(buf, "%s = mem8(%s%s);",
                reg8[dr], indreg[ir], signedoff(index));
        i->len = 3;
            return 3;
        }
        if ((opcode & 0xcf) == 0x09) {
            sprintf(buf, "%s += %s;", indreg[ir], rp[r2]);
        i->len = 2;
            return 2;
        }
        if ((opcode & 0xf8) == 0x70) {
            if (sr != 6) {
                sprintf(buf, "mem8(%s%s) = %s;",
                    indreg[ir], signedoff(index), reg8[sr]);
        i->len = 3;
                return 3;
            }
        }
        if ((opcode & 0xc7) == 0x86) {
            sprintf(buf, "a %s mem8(%s%s);",
                op8[dr], indreg[ir], signedoff(index));
        i->len = 3;
            return 3;
        }
        sprintf(buf, "-----------undef %s %x",
            ir ? "iy" : "ix", opcode);
        i->len = 2;
        return 2;
    }
#endif
    return 0;
}
