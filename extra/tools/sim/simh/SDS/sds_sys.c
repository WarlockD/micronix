/* sds_sys.c: SDS 940 simulator interface

   Copyright (c) 2001-2012, Robert M Supnik

   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   ROBERT M SUPNIK BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
   IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

   Except as contained in this notice, the name of Robert M Supnik shall not be
   used in advertising or otherwise to promote the sale, use or other dealings
   in this Software without prior written authorization from Robert M Supnik.

   19-Mar-12    RMS     Fixed declarations of CCT arrays (Mark Pizzolato)
*/

#include "sds_defs.h"
#include <ctype.h>
#define FMTASC(x) ((x) < 040)? "<%03o>": "%c", (x)

extern DEVICE cpu_dev;
extern DEVICE chan_dev;
extern DEVICE ptr_dev;
extern DEVICE ptp_dev;
extern DEVICE tti_dev;
extern DEVICE tto_dev;
extern DEVICE lpt_dev;
extern DEVICE rtc_dev;
extern DEVICE drm_dev;
extern DEVICE rad_dev;
extern DEVICE dsk_dev;
extern DEVICE mt_dev;
extern DEVICE mux_dev, muxl_dev;
extern UNIT cpu_unit;
extern REG cpu_reg[];
extern uint32 M[MAXMEMSIZE];

/* SCP data structures and interface routines

   sim_name             simulator name string
   sim_PC               pointer to saved PC register descriptor
   sim_emax             number of words for examine
   sim_devices          array of pointers to simulated devices
   sim_stop_messages    array of pointers to stop messages
   sim_load             binary loader
*/

char sim_name[] = "SDS 940";

REG *sim_PC = &cpu_reg[0];

int32 sim_emax = 1;

DEVICE *sim_devices[] = {
    &cpu_dev,
    &chan_dev,
    &ptr_dev,
    &ptp_dev,
    &tti_dev,
    &tto_dev,
    &lpt_dev,
    &rtc_dev,
    &drm_dev,
    &rad_dev,
    &dsk_dev,
    &mt_dev,
    &mux_dev,
    &muxl_dev,
    NULL
    };

const char *sim_stop_messages[] = {
    "Unknown error",
    "IO device not ready",
    "HALT instruction",
    "Breakpoint",
    "Invalid IO device",
    "Invalid instruction",
    "Invalid I/O operation",
    "Nested indirects exceed limit",
    "Nested EXU's exceed limit",
    "Memory management trap during interrupt",
    "Memory management trap during trap",
    "Trap instruction not BRM",
    "RTC instruction not MIN or SKR",
    "Interrupt vector zero",
    "Runaway carriage control tape"
    };

/* Character conversion tables */

const char sds_to_ascii[64] = {
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', ' ', '=', '\'', ':', '>', '%',            /* 17 = check mark */
    '+', 'A', 'B', 'C', 'D', 'E', 'F', 'G',
    'H', 'I', '?', '.', ')', '[', '<', '@',             /* 37 = stop code */
    '-', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
    'Q', 'R', '!', '$', '*', ']', ';', '^',             /* 57 = triangle */
    '_', '/', 'S', 'T', 'U', 'V', 'W', 'X',
    'Y', 'Z', '?', ',', '(', '~', '\\', '#'             /* 72 = rec mark */
     };                                                 /* 75 = squiggle, 77 = del */

const char ascii_to_sds[128] = {
     -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,             /* 0 - 37 */
    032, 072,  -1,  -1,  -1, 052,  -1,  -1,
     -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
     -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
    012, 052,  -1, 077, 053, 017,  -1, 014,             /* 40 - 77 */
    074, 034, 054, 020, 073, 040, 033, 061,
    000, 001, 002, 003, 004, 005, 006, 007,
    010, 011, 015, 056, 036, 013, 016, 072,
    037, 021, 022, 023, 024, 025, 026, 027,             /* 100 - 137 */
    030, 031, 041, 042, 043, 044, 045, 046,
    047, 050, 051, 062, 063, 064, 065, 066,
    067, 070, 071, 035, 076, 055, 057, 060,
    000, 021, 022, 023, 024, 025, 026, 027,             /* 140 - 177 */
    030, 031, 041, 042, 043, 044, 045, 046,
    047, 050, 051, 062, 063, 064, 065, 066,
    067, 070, 071,  -1,  -1,  -1,  -1,  -1
    };

const char odd_par[64] = {
    0100, 0001, 0002, 0103, 0004, 0105, 0106, 0007,
    0010, 0111, 0112, 0013, 0114, 0015, 0016, 0117,
    0020, 0121, 0122, 0023, 0124, 0025, 0026, 0127,
    0130, 0031, 0032, 0133, 0034, 0135, 0136, 0037,
    0040, 0141, 0142, 0043, 0144, 0045, 0046, 0147,
    0150, 0051, 0052, 0153, 0054, 0155, 0156, 0057,
    0160, 0061, 0062, 0163, 0064, 0165, 0166, 0067,
    0070, 0171, 0172, 0073, 0174, 0075, 0076, 0177
    };

/* Load carriage control tape

   A carriage control tape consists of entries of the form

        (repeat count) column number,column number,column number,...

   The CCT entries are stored in lpt_cct[0:lnt-1], lpt_ccl contains the
   number of entries
*/

t_stat sim_load_cct (FILE *fileref)
{
int32 col, rpt, ptr, mask, cctbuf[CCT_LNT];
t_stat r;
extern int32 lpt_ccl, lpt_ccp;
extern uint8 lpt_cct[CCT_LNT];
char *cptr, cbuf[CBUFSIZE], gbuf[CBUFSIZE];

ptr = 0;
for ( ; (cptr = fgets (cbuf, CBUFSIZE, fileref)) != NULL; ) { /* until eof */
    mask = 0;
    if (*cptr == '(') {                                 /* repeat count? */
        cptr = get_glyph (cptr + 1, gbuf, ')');         /* get 1st field */
        rpt = get_uint (gbuf, 10, CCT_LNT, &r);         /* repeat count */
        if (r != SCPE_OK)
            return SCPE_FMT;
        }
    else rpt = 1;
    while (*cptr != 0) {                                /* get col no's */
        cptr = get_glyph (cptr, gbuf, ',');             /* get next field */
        col = get_uint (gbuf, 10, 7, &r);               /* column number */
        if (r != SCPE_OK)
            return SCPE_FMT;
        mask = mask | (1 << col);                       /* set bit */
        }
    for ( ; rpt > 0; rpt--) {                           /* store vals */
        if (ptr >= CCT_LNT)
            return SCPE_FMT;
        cctbuf[ptr++] = mask;
        }
    }
if (ptr == 0) return SCPE_FMT;
lpt_ccl = ptr;
lpt_ccp = 0;
for (rpt = 0; rpt < lpt_ccl; rpt++)
    lpt_cct[rpt] = cctbuf[rpt];
return SCPE_OK;
}

/* Load command.  -l means load a line printer tape.  Otherwise, load
   a bootstrap paper tape.
*/

int32 get_word (FILE *fileref, int32 *ldr)
{
int32 i, c, wd;

for (i = wd = 0; i < 4; ) {
    if ((c = fgetc (fileref)) == EOF)
        return -1;
    if ((c == 0) && (*ldr == 0))
        return -1;
    if (c == 0)
        continue;
    *ldr = 0;
    wd = (wd << 6) | (c & 077);
    i++;
    }
return wd;
}
    
t_stat sim_load (FILE *fileref, char *cptr, char *fnam, int flag)
{
int32 i, wd, buf[8];
int32 ldr = 1;
extern int32 sim_switches;
extern uint32 P;

if ((*cptr != 0) || (flag != 0))
    return SCPE_ARG;
if (sim_switches & SWMASK ('L'))
    return sim_load_cct (fileref);
for (i = 0; i < 8; i++) {                               /* read boot */
    if ((wd = get_word (fileref, &ldr)) < 0)
        return SCPE_FMT;
    buf[i] = wd;
	}
if ((buf[0] != 023200012) ||                            /* 2 = WIM 12,2 */
    (buf[1] != 004100002) ||                            /* 3 = BRX 2 */
    (buf[2] != 007100011) ||                            /* 4 = LDX 11 */
    (buf[3] != 023200000) ||                            /* 5 = WIM 0,2 */
    (buf[4] != 004021000) ||                            /* 6 = SKS 21000 */
    (buf[5] != 004100005))                              /* 7 = BRX 5 */
    return SCPE_FMT;
for (i = 0; i < 8; i++)                                 /* copy boot */
    M[i + 2] = buf[i];
if (I_GETOP (buf[6]) == BRU)
    P = buf[6] & VA_MASK;
for (i = buf[7] & VA_MASK; i <= VA_MASK; i++) {         /* load data */
    if ((wd = get_word (fileref, &ldr)) < 0)
        return SCPE_OK;
    M[i] = wd;
    }
return SCPE_NXM;
}

/* Symbol tables */

#define I_V_FL          24                              /* inst class */
#define I_M_FL          017                             /* class mask */
#define I_V_NPN         000                             /* no operand */
#define I_V_PPO         001                             /* POP */
#define I_V_IOI         002                             /* IO */
#define I_V_MRF         003                             /* memory reference */
#define I_V_REG         004                             /* register change */
#define I_V_SHF         005                             /* shift */
#define I_V_OPO         006                             /* opcode only */
#define I_V_CHC         007                             /* chan cmd */
#define I_V_CHT         010                             /* chan test */
#define I_NPN           (I_V_NPN << I_V_FL)     
#define I_PPO           (I_V_PPO << I_V_FL)     
#define I_IOI           (I_V_IOI << I_V_FL)     
#define I_MRF           (I_V_MRF << I_V_FL)     
#define I_REG           (I_V_REG << I_V_FL)     
#define I_SHF           (I_V_SHF << I_V_FL)     
#define I_OPO           (I_V_OPO << I_V_FL)
#define I_CHC           (I_V_CHC << I_V_FL)
#define I_CHT           (I_V_CHT << I_V_FL)     

static const int32 masks[] = {
 037777777, 010000000, 017700000,
 017740000, 017700000, 017774000,
 017700000, 017377677, 027737677
 };

static const char *opcode[] = {
 "POP", "EIR", "DIR",
 "ROV", "REO", "OTO", "OVT",
 "IDT", "IET",
 "BPT4", "BPT3", "BPT2", "BPT1",
 "CLAB", "ABC", "BAC", "XAB",
 "XXB", "STE", "LDE", "XEE",
 "CLEAR",

 "HLT", "BRU", "EOM", "EOD",
 "MIY", "BRI", "MIW", "POT",
 "ETR", "MRG", "EOR",
 "NOP", "EXU",
 "YIM", "WIM", "PIN",
 "STA", "STB", "STX",
 "SKS", "BRX", "BRM",
 "SKE", "BRR", "SKB", "SKN",
 "SUB", "ADD", "SUC", "ADC",
 "SKR", "MIN", "XMA", "ADM",
 "MUL", "DIV",
 "SKM", "LDX", "SKA", "SKG",
 "SKD", "LDB", "LDA", "EAX",

         "BRU*", 
 "MIY*", "BRI*", "MIW*", "POT*",
 "ETR*", "MRG*", "EOR*",
         "EXU*",
 "YIM*", "WIM*", "PIN*",
 "STA*", "STB*", "STX*",
         "BRX*", "BRM*",
 "SKE*", "BRR*", "SKB*", "SKN*",
 "SUB*", "ADD*", "SUC*", "ADC*",
 "SKR*", "MIN*", "XMA*", "ADM*",
 "MUL*", "DIV*",
 "SKM*", "LDX*", "SKA*", "SKG*",
 "SKD*", "LDB*", "LDA*", "EAX*",

 "RSH", "RCY", "LRSH",
 "LSH", "NOD", "LCY",
 "RSH*", "LSH*",

 "ALC", "DSC", "ASC", "TOP",
 "CAT", "CET", "CZT", "CIT",

 "CLA", "CLB", "CAB",                                   /* encode only */
 "CBA", "CBX", "CXB",
 "XPO", "CXA", "CAX",
 "CNA", "CLX", NULL,
 NULL
 };

static const int32 opc_val[] = {
 010000000+I_PPO, 000220002+I_NPN, 000220004+I_NPN,
 002200001+I_NPN, 002200010+I_NPN, 002200100+I_NPN, 002200101+I_NPN,
 004020002+I_NPN, 004020004+I_NPN,
 004020040+I_NPN, 004020100+I_NPN, 004020200+I_NPN, 004020400+I_NPN,
 004600003+I_NPN, 004600005+I_NPN, 004600012+I_NPN, 004600014+I_NPN,
 004600060+I_NPN, 004600122+I_NPN, 004600140+I_NPN, 004600160+I_NPN,
 024600003+I_NPN,

 000000000+I_NPN, 000100000+I_MRF, 000200000+I_IOI, 000600000+I_IOI,
 001000000+I_MRF, 001100000+I_MRF, 001200000+I_MRF, 001300000+I_MRF,
 001400000+I_MRF, 001600000+I_MRF, 001700000+I_MRF,
 002000000+I_OPO, 002300000+I_MRF,
 003000000+I_MRF, 003200000+I_MRF, 003300000+I_MRF,
 003500000+I_MRF, 003600000+I_MRF, 003700000+I_MRF,
 004000000+I_IOI, 004100000+I_MRF, 004300000+I_MRF,
 005000000+I_MRF, 005100000+I_MRF, 005200000+I_MRF, 005300000+I_MRF,
 005400000+I_MRF, 005500000+I_MRF, 005600000+I_MRF, 005700000+I_MRF,
 006000000+I_MRF, 006100000+I_MRF, 006200000+I_MRF, 006300000+I_MRF,
 006400000+I_MRF, 006500000+I_MRF,
 007000000+I_MRF, 007100000+I_MRF, 007200000+I_MRF, 007300000+I_MRF,
 007400000+I_MRF, 007500000+I_MRF, 007600000+I_MRF, 007700000+I_MRF,

                  000140000+I_MRF,
 001040000+I_MRF, 001140000+I_MRF, 001240000+I_MRF, 001340000+I_MRF,
 001440000+I_MRF, 001640000+I_MRF, 001740000+I_MRF,
                  002340000+I_MRF,
 003040000+I_MRF, 003240000+I_MRF, 003340000+I_MRF,
 003540000+I_MRF, 003640000+I_MRF, 003740000+I_MRF,
                  004140000+I_MRF, 004340000+I_MRF,
 005040000+I_MRF, 005140000+I_MRF, 005240000+I_MRF, 005340000+I_MRF,
 005440000+I_MRF, 005540000+I_MRF, 005640000+I_MRF, 005740000+I_MRF,
 006040000+I_MRF, 006140000+I_MRF, 006240000+I_MRF, 006340000+I_MRF,
 006440000+I_MRF, 006540000+I_MRF,
 007040000+I_MRF, 007140000+I_MRF, 007240000+I_MRF, 007340000+I_MRF,
 007440000+I_MRF, 007540000+I_MRF, 007640000+I_MRF, 007740000+I_MRF,

 006600000+I_SHF, 006620000+I_SHF, 006624000+I_SHF,
 006700000+I_SHF, 006710000+I_SHF, 006720000+I_SHF,
 006640000+I_MRF, 006740000+I_MRF,

 000250000+I_CHC, 000200000+I_CHC, 000212000+I_CHC, 000214000+I_CHC,
 004014000+I_CHT, 004011000+I_CHT, 004012000+I_CHT, 004010400+I_CHT,

 004600001+I_REG, 004600002+I_REG, 004600004+I_REG,
 004600010+I_REG, 004600020+I_REG, 004600040+I_REG,
 004600100+I_REG, 004600200+I_REG, 004600400+I_REG,
 004601000+I_REG, 024600000+I_REG, 004600000+I_REG,
 -1
 };

static const char *chname[] = {
 "W", "Y", "C", "D", "E", "F", "G", "H", NULL
 };

/* Register change decode

   Inputs:
        *of     =       output stream
        inst    =       mask bits
*/

void fprint_reg (FILE *of, int32 inst)
{
int32 i, j, sp;

inst = inst & ~(I_M_OP << I_V_OP);                      /* clear opcode */
for (i = sp = 0; opc_val[i] >= 0; i++) {                /* loop thru ops */
    j = (opc_val[i] >> I_V_FL) & I_M_FL;                /* get class */
    if ((j == I_V_REG) && (opc_val[i] & inst)) {        /* reg class? */
        inst = inst & ~opc_val[i];                      /* mask bit set? */
        fprintf (of, (sp? " %s": "%s"), opcode[i]);
        sp = 1;
        }
    }
return;
}

/* Symbolic decode

   Inputs:
        *of     =       output stream
        addr    =       current PC
        *val    =       pointer to values
        *uptr   =       pointer to unit
        sw      =       switches
   Outputs:
        return  =       status code
*/

t_stat fprint_sym (FILE *of, t_addr addr, t_value *val,
    UNIT *uptr, int32 sw)
{
int32 i, j, ch;
int32 inst, op, tag, va, shf, nonop;

inst = val[0];                                          /* get inst */
op = I_GETOP (inst);                                    /* get fields */
tag = (inst >> 21) & 06;
va = inst & VA_MASK;
shf = inst & I_SHFMSK;
nonop = inst & 077777;

if (sw & SWMASK ('A')) {                                /* ASCII? */
    if (inst > 0377)
        return SCPE_ARG;
    fprintf (of, FMTASC (inst & 0177));
    return SCPE_OK;
    }
if (sw & SWMASK ('C')) {                                /* character? */
    fprintf (of, "%c", sds_to_ascii[(inst >> 18) & 077]);
    fprintf (of, "%c", sds_to_ascii[(inst >> 12) & 077]);
    fprintf (of, "%c", sds_to_ascii[(inst >> 6) & 077]);
    fprintf (of, "%c", sds_to_ascii[inst & 077]);
    return SCPE_OK;
    }
if (!(sw & SWMASK ('M'))) return SCPE_ARG;

/* Instruction decode */

for (i = 0; opc_val[i] >= 0; i++) {                     /* loop thru ops */
    j = (opc_val[i] >> I_V_FL) & I_M_FL;                /* get class */
    if ((opc_val[i] & DMASK) == (inst & masks[j])) {    /* match? */

        switch (j) {                                    /* case on class */

        case I_V_NPN:                                   /* no operands */
        case I_V_OPO:                                   /* opcode only */
            fprintf (of, "%s", opcode[i]);              /* opcode */
            break;

        case I_V_SHF:                                   /* shift */
            fprintf (of, "%s %-o", opcode[i], shf);
            if (tag)
                fprintf (of, ",%-o", tag);
            break;

        case I_V_PPO:                                   /* pop */
            fprintf (of, "POP %-o,%-o", op, nonop);
            if (tag)
                fprintf (of, ",%-o", tag);
            break;

        case I_V_IOI:                                   /* I/O */
            fprintf (of, "%s %-o", opcode[i], nonop);
            if (tag)
                fprintf (of, ",%-o", tag);
            break;

        case I_V_MRF:                                   /* mem ref */
            fprintf (of, "%s %-o", opcode[i], va);
            if (tag)
                fprintf (of, ",%-o", tag);
            break;

        case I_V_REG:                                   /* reg change */
            fprint_reg (of, inst);                      /* decode */
            break;

        case I_V_CHC:                                   /* chan cmd */
            ch = I_GETEOCH (inst);                      /* get chan */
            fprintf (of, "%s %s", opcode[i], chname[ch]);
            break;

        case I_V_CHT:                                   /* chan test */
            ch = I_GETSKCH (inst);                      /* get chan */
            fprintf (of, "%s %s", opcode[i], chname[ch]);
            break;
            }                                           /* end case */

        return SCPE_OK;
        }                                               /* end if */
    }                                                   /* end for */
return SCPE_ARG;
}

/* Get (optional) tag

   Inputs:
        *cptr = pointer to input string
        *tag  = pointer to tag
   Outputs:
        cptr  = updated pointer to input string
*/

char *get_tag (char *cptr, t_value *tag)
{
char *tptr, gbuf[CBUFSIZE];
t_stat r;

tptr = get_glyph (cptr, gbuf, 0);                       /* get next field */
*tag = get_uint (gbuf, 8, 07, &r) << I_V_TAG;           /* parse */
if (r == SCPE_OK)                                       /* ok? advance */
    return tptr;
*tag = 0;
return cptr;                                            /* no change */
}

/* Symbolic input

   Inputs:
        *cptr   =       pointer to input string
        addr    =       current PC
        uptr    =       pointer to unit
        *val    =       pointer to output values
        sw      =       switches
   Outputs:
        status  =       error status
*/

t_stat parse_sym (char *cptr, t_addr addr, UNIT *uptr, t_value *val, int32 sw)
{
int32 i, j, k;
t_value d, tag;
t_stat r;
char gbuf[CBUFSIZE];

while (isspace (*cptr)) cptr++;
for (i = 1; (i < 4) && (cptr[i] != 0); i++) {
    if (cptr[i] == 0) {
        for (j = i + 1; j <= 4; j++)
            cptr[j] = 0;
        }
    }
if ((sw & SWMASK ('A')) || ((*cptr == '\'') && cptr++)) { /* ASCII char? */
    if (cptr[0] == 0)                                   /* must have 1 char */
        return SCPE_ARG;
    val[0] = (t_value) cptr[0] | 0200;
    return SCPE_OK;
    }
if ((sw & SWMASK ('C')) || ((*cptr == '"') && cptr++)) { /* string? */
    if (cptr[0] == 0)                                   /* must have 1 char */
        return SCPE_ARG;
    for (i = j = 0, val[0] = 0; i < 4; i++) {
        if (cptr[i] == 0)                               /* latch str end */
            j = 1;
        k = ascii_to_sds[cptr[i] & 0177];               /* cvt char */
        if (j || (k < 0))                               /* bad, end? spc */
            k = 0;
        val[0] = (val[0] << 6) | k;
        }
    return SCPE_OK;
    }

cptr = get_glyph (cptr, gbuf, 0);                       /* get opcode */
for (i = 0; (opcode[i] != NULL) && (strcmp (opcode[i], gbuf) != 0) ; i++) ;
if (opcode[i] == NULL)
    return SCPE_ARG;
val[0] = opc_val[i] & DMASK;                            /* get value */
j = (opc_val[i] >> I_V_FL) & I_M_FL;                    /* get class */

switch (j) {                                            /* case on class */

    case I_V_NPN: case I_V_OPO:                         /* opcode only */
        break;

    case I_V_SHF:                                       /* shift */
        cptr = get_glyph (cptr, gbuf, ',');             /* get next field */
        d = get_uint (gbuf, 8, I_SHFMSK, &r);           /* shift count */
        if (r != SCPE_OK)
            return SCPE_ARG;
        cptr = get_tag (cptr, &tag);                    /* get opt tag */
        val[0] = val[0] | d | tag;
        break;

    case I_V_PPO:                                       /* pop */
        cptr = get_glyph (cptr, gbuf, ',');             /* get next field */
        d = get_uint (gbuf, 8, 077, &r);                /* opcode */
        if (r != SCPE_OK)
            return SCPE_ARG;
        val[0] = val[0] | d;                            /* fall thru */

    case I_V_IOI:                                       /* I/O */
        cptr = get_glyph (cptr, gbuf, ',');             /* get next field */
        d = get_uint (gbuf, 8, 077777, &r);             /* 15b address */
        if (r != SCPE_OK)
            return SCPE_ARG;
        cptr = get_tag (cptr, &tag);                    /* get opt tag */
        val[0] = val[0] | d | tag;
        break;

    case I_V_MRF:                                       /* mem ref */
        cptr = get_glyph (cptr, gbuf, ',');             /* get next field */
        d = get_uint (gbuf, 8, VA_MASK, &r);            /* virt address */
        if (r != SCPE_OK)
            return SCPE_ARG;
        cptr = get_tag (cptr, &tag);                    /* get opt tag */
        val[0] = val[0] | d | tag;
        break;

    case I_V_REG:                                       /* register */
        for (cptr = get_glyph (cptr, gbuf, 0); gbuf[0] != 0;
             cptr = get_glyph (cptr, gbuf, 0)) {
            for (i = 0; (opcode[i] != NULL) &&
                (strcmp (opcode[i], gbuf) != 0); i++) ;
            if (opcode[i] != NULL) {
                k = opc_val[i] & DMASK;;
                if (I_GETOP (k) != RCH)
                    return SCPE_ARG;
                val[0] = val[0] | k;
                }
            else {
                d = get_uint (gbuf, 8, 077777, &r);
                if (r != SCPE_OK)
                    return SCPE_ARG;
                else val[0] = val[0] | d;
                }
            }
        break;

    case I_V_CHC: case I_V_CHT:                         /* channel */
        cptr = get_glyph (cptr, gbuf, ',');             /* get next field */
        for (i = 0; (chname[i] != NULL) && (strcmp (chname[i], gbuf) != 0);
            i++);
        if (chname[i] != NULL)                          /* named chan */
            d = i;
        else {
            d = get_uint (gbuf, 8, NUM_CHAN - 1, &r);
            if (r != SCPE_OK)                           /* numbered chan */
                return SCPE_ARG;
            }
        val[0] = val[0] | ((j == I_V_CHC)? I_SETEOCH (d): I_SETSKCH (d));
        break;
        }                                               /* end case */

if (*cptr != 0)                                         /* junk at end? */
    return SCPE_ARG;
return SCPE_OK;
}
