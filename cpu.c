/****************************************************************************
*                                                                           *
*                            Third Year Project                             *
*                                                                           *
*                            An IBM PC Emulator                             *
*                          For Unix and X Windows                           *
*                                                                           *
*                             By David Hedley                               *
*                                                                           *
*                                                                           *
* This program is Copyrighted.  Consult the file COPYRIGHT for more details *
*                                                                           *
****************************************************************************/

/* Statistics:
   At 12/12/00	MIPS:	5.12/5.89/8.20/5.86/7.80 (mlh)
*/
  
/* This is CPU.C  It emulates the 8086 processor */

#include "global.h"

#include <stdio.h>

#include "mytypes.h"
#include "cpu.h"
#include "bios.h"
#include "instr.h"
#include "hardware.h"

#ifdef DEBUGGER
#   include "debugger.h"
#endif

#ifdef DEBUG
#	include <stdlib.h>
#endif

WORD wregs[8];	     /* Always little-endian */
BYTE *bregs[16];     /* Points to bytes within wregs[] */
unsigned sregs[4];   /* Always native machine word order */

unsigned ip;	     /* Always native machine word order */


    /* All the byte flags will either be 1 or 0 */

BYTE CF, PF, ZF, TF, IF, DF;

    /* All the word flags may be either none-zero (true) or zero (false) */

unsigned AF, OF, SF;


static UINT8 parity_table[256];
static BYTE *ModRMRegB[256];
static WORD *ModRMRegW[256];

    /* 'Shadows' of the segment registers. Point directly into the start of the
       segment within memory[] */
BYTE *c_cs,*c_ds,*c_es,*c_ss,*c_stack;

volatile int int_pending;   /* Interrupt pending */
volatile int int_blocked;   /* Blocked pending */

static struct
{
    BYTE **segment;
    WORD *reg1;
    WORD *reg2;
    int offset;
    int offset16;
    int pad[3];	/* Make structure power of 2 bytes wide for speed */
} ModRMRM[256];

static WORD zero_word = 0;

static WORD *ModRMRMWRegs[256];
static BYTE *ModRMRMBRegs[256];

void trap(void);

#define PushSeg(Seg) \
{ \
      register unsigned tmp = (WORD)(ReadWord(&wregs[SP])-2); \
      WriteWord(&wregs[SP],tmp); \
      PutMemW(c_stack,tmp,sregs[Seg]); \
}


#define PopSeg(seg, Seg) \
{ \
      register unsigned tmp = ReadWord(&wregs[SP]); \
      sregs[Seg] = GetMemW(c_stack,tmp); \
      seg = SegToMemPtr(Seg); \
      tmp += 2; \
      WriteWord(&wregs[SP],tmp); \
}


#define PushWordReg(Reg) \
{ \
      register unsigned tmp1 = (WORD)(ReadWord(&wregs[SP])-2); \
      WORD tmp2; \
      WriteWord(&wregs[SP],tmp1); \
      tmp2 = ReadWord(&wregs[Reg]); \
      PutMemW(c_stack,tmp1,tmp2); \
}


#define PopWordReg(Reg) \
{ \
      register unsigned tmp = ReadWord(&wregs[SP]); \
      WORD tmp2 = GetMemW(c_stack,tmp); \
      tmp += 2; \
      WriteWord(&wregs[SP],tmp); \
      WriteWord(&wregs[Reg],tmp2); \
}


#define XchgAXReg(Reg) \
{ \
      register unsigned tmp; \
      tmp = wregs[Reg]; \
      wregs[Reg] = wregs[AX]; \
      wregs[AX] = tmp; \
}


#define IncWordReg(Reg) \
{ \
      register unsigned tmp = (unsigned)ReadWord(&wregs[Reg]); \
      register unsigned tmp1 = tmp+1; \
      SetOFW_Add(tmp1,tmp,1); \
      SetAF(tmp1,tmp,1); \
      SetZFW(tmp1); \
      SetSFW(tmp1); \
      SetPF(tmp1); \
      WriteWord(&wregs[Reg],tmp1); \
}


#define DecWordReg(Reg) \
{ \
      register unsigned tmp = (unsigned)ReadWord(&wregs[Reg]); \
      register unsigned tmp1 = tmp-1; \
      SetOFW_Sub(tmp1,1,tmp); \
      SetAF(tmp1,tmp,1); \
      SetZFW(tmp1); \
      SetSFW(tmp1); \
      SetPF(tmp1); \
      WriteWord(&wregs[Reg],tmp1); \
}


#define Logical_br8(op) \
{ \
      unsigned ModRM = (unsigned)GetMemInc(c_cs,ip); \
      register unsigned src = (unsigned)*GetModRMRegB(ModRM); \
      BYTE *dest = GetModRMRMB(ModRM); \
      register unsigned tmp = (unsigned) *dest; \
      tmp op ## = src; \
      CF = OF = AF = 0; \
      SetZFB(tmp); \
      SetSFB(tmp); \
      SetPF(tmp); \
      *dest = (BYTE)tmp; \
}


#define Logical_r8b(op) \
{ \
      unsigned ModRM = (unsigned)GetMemInc(c_cs,ip); \
      register unsigned src = (unsigned)*GetModRMRMB(ModRM); \
      BYTE *dest = GetModRMRegB(ModRM); \
      register unsigned tmp = (unsigned) *dest; \
      tmp op ## = src; \
      CF = OF = AF = 0; \
      SetZFB(tmp); \
      SetSFB(tmp); \
      SetPF(tmp); \
      *dest = (BYTE)tmp; \
}


#define Logical_wr16(op) \
{ \
      unsigned ModRM = GetMemInc(c_cs,ip); \
      WORD *src = GetModRMRegW(ModRM); \
      WORD *dest = GetModRMRMW(ModRM); \
      register unsigned tmp1 = (unsigned)ReadWord(src); \
      register unsigned tmp2 = (unsigned)ReadWord(dest); \
      register unsigned tmp3 = tmp1 op tmp2; \
      CF = OF = AF = 0; \
      SetZFW(tmp3); \
      SetSFW(tmp3); \
      SetPF(tmp3); \
      WriteWord(dest,tmp3); \
}


#define Logical_r16w(op) \
{ \
      unsigned ModRM = GetMemInc(c_cs,ip); \
      WORD *dest = GetModRMRegW(ModRM); \
      WORD *src = GetModRMRMW(ModRM); \
      register unsigned tmp1 = (unsigned)ReadWord(src); \
      register unsigned tmp2 = (unsigned)ReadWord(dest); \
      register unsigned tmp3 = tmp1 op tmp2; \
      CF = OF = AF = 0; \
      SetZFW(tmp3); \
      SetSFW(tmp3); \
      SetPF(tmp3); \
      WriteWord(dest,tmp3); \
}


#define Logical_ald8(op) \
{ \
      register unsigned tmp = *bregs[AL]; \
      tmp op ## = (unsigned)GetMemInc(c_cs,ip); \
      CF = OF = AF = 0; \
      SetZFB(tmp); \
      SetSFB(tmp); \
      SetPF(tmp); \
      *bregs[AL] = (BYTE)tmp; \
}


#define Logical_axd16(op) \
{ \
      register unsigned src; \
      register unsigned tmp = ReadWord(&wregs[AX]); \
      src = GetMemInc(c_cs,ip); \
      src += GetMemInc(c_cs,ip) << 8; \
      tmp op ## = src; \
      CF = OF = AF = 0; \
      SetZFW(tmp); \
      SetSFW(tmp); \
      SetPF(tmp); \
      WriteWord(&wregs[AX],tmp); \
}


#define LogicalOp(name,symbol) \
static INLINE2 void i_ ## name ## _br8(void) \
{ Logical_br8(symbol); } \
static INLINE2 void i_ ## name ## _wr16(void) \
{ Logical_wr16(symbol); } \
static INLINE2 void i_ ## name ## _r8b(void) \
{ Logical_r8b(symbol); } \
static INLINE2 void i_ ## name ## _r16w(void) \
{ Logical_r16w(symbol); } \
static INLINE2 void i_ ## name ## _ald8(void) \
{ Logical_ald8(symbol); } \
static INLINE2 void i_ ## name ## _axd16(void) \
{ Logical_axd16(symbol); }


#define JumpCond(name, cond) \
static INLINE2 void i_j ## name ## (void) \
{ \
      register int tmp = (int)((INT8)GetMemInc(c_cs,ip)); \
      if (cond) ip = (WORD)(ip+tmp); \
}


void init_cpu(void)
{
    unsigned int i,j,c;
    
    for (i = 0; i < 4; i++)
    {
        wregs[i] = 0;
        sregs[i] = 0x70;
    }
    for (; i < 8; i++)
        wregs[i] = 0;
    
    wregs[SP] = 0;
    ip = 0x100;
    
    for (i = 0;i < 256; i++)
    {
        for (j = i, c = 0; j > 0; j >>= 1)
            if (j & 1) c++;
        
        parity_table[i] = !(c & 1);
    }
    
    CF = PF = AF = ZF = SF = TF = IF = DF = OF = 0;
    
    bregs[AL] = (BYTE *)&wregs[AX];
    bregs[AH] = (BYTE *)&wregs[AX]+1;
    bregs[CL] = (BYTE *)&wregs[CX];
    bregs[CH] = (BYTE *)&wregs[CX]+1;
    bregs[DL] = (BYTE *)&wregs[DX];
    bregs[DH] = (BYTE *)&wregs[DX]+1;
    bregs[BL] = (BYTE *)&wregs[BX];
    bregs[BH] = (BYTE *)&wregs[BX]+1;
    
    bregs[SPL] = (BYTE *)&wregs[SP];
    bregs[SPH] = (BYTE *)&wregs[SP]+1;
    bregs[BPL] = (BYTE *)&wregs[BP];
    bregs[BPH] = (BYTE *)&wregs[BP]+1;
    bregs[SIL] = (BYTE *)&wregs[SI];
    bregs[SIH] = (BYTE *)&wregs[SI]+1;
    bregs[DIL] = (BYTE *)&wregs[DI];
    bregs[DIH] = (BYTE *)&wregs[DI]+1;
    
    for (i = 0; i < 256; i++)
    {
        ModRMRegB[i] = bregs[(i & 0x38) >> 3];
        ModRMRegW[i] = &wregs[(i & 0x38) >> 3];
    }
    
    for (i = 0; i < 0x40; i++)
    {
        switch (i & 7)
        {
        case 0:
            ModRMRM[i].segment = &c_ds;
            ModRMRM[i].reg1 = &wregs[BX];
            ModRMRM[i].reg2 = &wregs[SI];
            ModRMRM[i].offset = ModRMRM[i].offset16 = 0;
            break;
        case 1:
            ModRMRM[i].segment = &c_ds;
            ModRMRM[i].reg1 = &wregs[BX];
            ModRMRM[i].reg2 = &wregs[DI];
            ModRMRM[i].offset = ModRMRM[i].offset16 = 0;
            break;
        case 2:
            ModRMRM[i].segment = &c_ss;
            ModRMRM[i].reg1 = &wregs[BP];
            ModRMRM[i].reg2 = &wregs[SI];
            ModRMRM[i].offset = ModRMRM[i].offset16 = 0;
            break;
        case 3:
            ModRMRM[i].segment = &c_ss;
            ModRMRM[i].reg1 = &wregs[BP];
            ModRMRM[i].reg2 = &wregs[DI];
            ModRMRM[i].offset = ModRMRM[i].offset16 = 0;
            break;
        case 4:
            ModRMRM[i].segment = &c_ds;
            ModRMRM[i].reg1 = &zero_word;
            ModRMRM[i].reg2 = &wregs[SI];
            ModRMRM[i].offset = ModRMRM[i].offset16 = 0;
            break;
        case 5:
            ModRMRM[i].segment = &c_ds;
            ModRMRM[i].reg1 = &zero_word;
            ModRMRM[i].reg2 = &wregs[DI];
            ModRMRM[i].offset = ModRMRM[i].offset16 = 0;
            break;
        case 6:
            ModRMRM[i].segment = &c_ds;
            ModRMRM[i].reg1 = &zero_word;
            ModRMRM[i].reg2 = &zero_word;
            ModRMRM[i].offset = 1;
            ModRMRM[i].offset16 = 1;
            break;
        default:
            ModRMRM[i].segment = &c_ds;
            ModRMRM[i].reg1 = &wregs[BX];
            ModRMRM[i].reg2 = &zero_word;
            ModRMRM[i].offset = ModRMRM[i].offset16 = 0;
            break;
        }
    }
    
    for (i = 0x40; i < 0x80; i++)
    {
        switch (i & 7)
        {
        case 0:
            ModRMRM[i].segment = &c_ds;
            ModRMRM[i].reg1 = &wregs[BX];
            ModRMRM[i].reg2 = &wregs[SI];
            ModRMRM[i].offset = 1;
            ModRMRM[i].offset16 = 0;
            break;
        case 1:
            ModRMRM[i].segment = &c_ds;
            ModRMRM[i].reg1 = &wregs[BX];
            ModRMRM[i].reg2 = &wregs[DI];
            ModRMRM[i].offset = 1;
            ModRMRM[i].offset16 = 0;
            break;
        case 2:
            ModRMRM[i].segment = &c_ss;
            ModRMRM[i].reg1 = &wregs[BP];
            ModRMRM[i].reg2 = &wregs[SI];
            ModRMRM[i].offset = 1;
            ModRMRM[i].offset16 = 0;
            break;
        case 3:
            ModRMRM[i].segment = &c_ss;
            ModRMRM[i].reg1 = &wregs[BP];
            ModRMRM[i].reg2 = &wregs[DI];
            ModRMRM[i].offset = 1;
            ModRMRM[i].offset16 = 0;
            break;
        case 4:
            ModRMRM[i].segment = &c_ds;
            ModRMRM[i].reg1 = &zero_word;
            ModRMRM[i].reg2 = &wregs[SI];
            ModRMRM[i].offset = 1;
            ModRMRM[i].offset16 = 0;
            break;
        case 5:
            ModRMRM[i].segment = &c_ds;
            ModRMRM[i].reg1 = &zero_word;
            ModRMRM[i].reg2 = &wregs[DI];
            ModRMRM[i].offset = 1;
            ModRMRM[i].offset16 = 0;
            break;
        case 6:
            ModRMRM[i].segment = &c_ss;
            ModRMRM[i].reg1 = &wregs[BP];
            ModRMRM[i].reg2 = &zero_word;
            ModRMRM[i].offset = 1;
            ModRMRM[i].offset16 = 0;
            break;
        default:
            ModRMRM[i].segment = &c_ds;
            ModRMRM[i].reg1 = &wregs[BX];
            ModRMRM[i].reg2 = &zero_word;
            ModRMRM[i].offset = 1;
            ModRMRM[i].offset16 = 0;
            break;
        }
    }
    
    for (i = 0x80; i < 0xc0; i++)
    {
        switch (i & 7)
        {
        case 0:
            ModRMRM[i].segment = &c_ds;
            ModRMRM[i].reg1 = &wregs[BX];
            ModRMRM[i].reg2 = &wregs[SI];
            ModRMRM[i].offset = 1;
            ModRMRM[i].offset16 = 1;
            break;
        case 1:
            ModRMRM[i].segment = &c_ds;
            ModRMRM[i].reg1 = &wregs[BX];
            ModRMRM[i].reg2 = &wregs[DI];
            ModRMRM[i].offset = 1;
            ModRMRM[i].offset16 = 1;
            break;
        case 2:
            ModRMRM[i].segment = &c_ss;
            ModRMRM[i].reg1 = &wregs[BP];
            ModRMRM[i].reg2 = &wregs[SI];
            ModRMRM[i].offset = 1;
            ModRMRM[i].offset16 = 1;
            break;
        case 3:
            ModRMRM[i].segment = &c_ss;
            ModRMRM[i].reg1 = &wregs[BP];
            ModRMRM[i].reg2 = &wregs[DI];
            ModRMRM[i].offset = 1;
            ModRMRM[i].offset16 = 1;
            break;
        case 4:
            ModRMRM[i].segment = &c_ds;
            ModRMRM[i].reg1 = &zero_word;
            ModRMRM[i].reg2 = &wregs[SI];
            ModRMRM[i].offset = 1;
            ModRMRM[i].offset16 = 1;
            break;
        case 5:
            ModRMRM[i].segment = &c_ds;
            ModRMRM[i].reg1 = &zero_word;
            ModRMRM[i].reg2 = &wregs[DI];
            ModRMRM[i].offset = 1;
            ModRMRM[i].offset16 = 1;
            break;
        case 6:
            ModRMRM[i].segment = &c_ss;
            ModRMRM[i].reg1 = &wregs[BP];
            ModRMRM[i].reg2 = &zero_word;
            ModRMRM[i].offset = 1;
            ModRMRM[i].offset16 = 1;
            break;
        default:
            ModRMRM[i].segment = &c_ds;
            ModRMRM[i].reg1 = &wregs[BX];
            ModRMRM[i].reg2 = &zero_word;
            ModRMRM[i].offset = 1;
            ModRMRM[i].offset16 = 1;
            break;
        }
    }
    
    for (i = 0xc0; i < 0x100; i++)
    {
        ModRMRMWRegs[i] = &wregs[i & 7];
        ModRMRMBRegs[i] = bregs[i & 7];
    }
}


#ifdef DEBUG2
void loc(void)
{
    printf("%04X:%04X ", sregs[CS], ip);
}
#endif

static void interrupt(unsigned int_num)
{
    unsigned dest_seg, dest_off,tmp1;

    i_pushf();
    dest_off = GetMemW(memory,int_num*4);
    dest_seg = GetMemW(memory,int_num*4+2);

    tmp1 = (WORD)(ReadWord(&wregs[SP])-2);

    PutMemW(c_stack,tmp1,sregs[CS]);
    tmp1 = (WORD)(tmp1-2);
    PutMemW(c_stack,tmp1,ip);
    WriteWord(&wregs[SP],tmp1);

    ip = (WORD)dest_off;
    sregs[CS] = (WORD)dest_seg;
    c_cs = SegToMemPtr(CS);

    TF = IF = 0;    /* Turn of trap and interrupts... */

}


static void external_int(void)
{
    disable();

#ifdef DEBUGGER
    call_debugger(D_INT);
#endif
    D2(printf("Interrupt 0x%02X\n", int_pending););
    interrupt(int_pending);
    int_pending = 0;

    enable();
}


#define GetModRMRegW(ModRM) (ModRMRegW[ModRM])
#define GetModRMRegB(ModRM) (ModRMRegB[ModRM])

static INLINE WORD *GetModRMRMW(unsigned ModRM)
{
    register unsigned dest; 

    if (ModRM >= 0xc0)
        return ModRMRMWRegs[ModRM];

    dest = 0;

    if (ModRMRM[ModRM].offset)
    {
        dest = (WORD)((INT16)((INT8)GetMemInc(c_cs,ip)));
        if (ModRMRM[ModRM].offset16)
            dest = (GetMemInc(c_cs,ip) << 8) + (BYTE)dest;
    }
    
    return (WORD *)(BYTE *)(*ModRMRM[ModRM].segment +
                    (WORD)(ReadWord(ModRMRM[ModRM].reg1) +
                           ReadWord(ModRMRM[ModRM].reg2) + dest));
}


static INLINE BYTE *GetModRMRMB(unsigned ModRM)
{
    register unsigned dest;
    
    if (ModRM >= 0xc0)
        return ModRMRMBRegs[ModRM];

    dest = 0;

    if (ModRMRM[ModRM].offset)
    {
        dest = (WORD)((INT16)((INT8)GetMemInc(c_cs,ip)));
        if (ModRMRM[ModRM].offset16)
            dest = (GetMemInc(c_cs,ip) << 8) + (BYTE)dest;
    }
    
    return (*ModRMRM[ModRM].segment +
            (WORD)(ReadWord(ModRMRM[ModRM].reg1) +
                   ReadWord(ModRMRM[ModRM].reg2) + dest));
}


static INLINE2 void i_add_br8(void)
{
    /* Opcode 0x00 */

    unsigned ModRM = (unsigned)GetMemInc(c_cs,ip);
    register unsigned src = (unsigned)*GetModRMRegB(ModRM);
    BYTE *dest = GetModRMRMB(ModRM);
    register unsigned tmp = (unsigned) *dest;
    register unsigned tmp2 = tmp;
    
    tmp += src;
    
    SetCFB_Add(tmp,tmp2);
    SetOFB_Add(tmp,src,tmp2);
    SetAF(tmp,src,tmp2);
    SetZFB(tmp);
    SetSFB(tmp);
    SetPF(tmp);

    *dest = (BYTE)tmp;
}


static INLINE2 void i_add_wr16(void)
{
    /* Opcode 0x01 */

    unsigned ModRM = GetMemInc(c_cs,ip);
    WORD *src = GetModRMRegW(ModRM);
    WORD *dest = GetModRMRMW(ModRM);
    register unsigned tmp1 = (unsigned)ReadWord(dest);
    register unsigned tmp2 = (unsigned)ReadWord(src);
    register unsigned tmp3;

    tmp3 = tmp1+tmp2;

    SetCFW_Add(tmp3,tmp1);
    SetOFW_Add(tmp3,tmp2,tmp1);
    SetAF(tmp3,tmp2,tmp1);
    SetZFW(tmp3);
    SetSFW(tmp3);
    SetPF(tmp3);

    WriteWord(dest, tmp3);
}


static INLINE2 void i_add_r8b(void)
{
    /* Opcode 0x02 */

    unsigned ModRM = (unsigned)GetMemInc(c_cs,ip);
    register unsigned src = (unsigned)*GetModRMRMB(ModRM);
    BYTE *dest = GetModRMRegB(ModRM);
    register unsigned tmp = (unsigned) *dest;
    register unsigned tmp2 = tmp;

    tmp += src;

    SetCFB_Add(tmp,tmp2);
    SetOFB_Add(tmp,src,tmp2);
    SetAF(tmp,src,tmp2);
    SetZFB(tmp);
    SetSFB(tmp);
    SetPF(tmp);

    *dest = (BYTE)tmp;

}


static INLINE2 void i_add_r16w(void)
{
    /* Opcode 0x03 */

    unsigned ModRM = GetMemInc(c_cs,ip);
    WORD *dest = GetModRMRegW(ModRM);
    WORD *src = GetModRMRMW(ModRM);
    register unsigned tmp1 = (unsigned)ReadWord(dest);
    register unsigned tmp2 = (unsigned)ReadWord(src);
    register unsigned tmp3;

    tmp3 = tmp1 + tmp2;

    SetCFW_Add(tmp3,tmp1);
    SetOFW_Add(tmp3,tmp2,tmp1);
    SetAF(tmp3,tmp2,tmp1);
    SetZFW(tmp3);
    SetSFW(tmp3);
    SetPF(tmp3);

    WriteWord(dest, tmp3);
}


static INLINE2 void i_add_ald8(void)
{
    /* Opcode 0x04 */

    register unsigned src = (unsigned)GetMemInc(c_cs,ip);
    register unsigned tmp = (unsigned)*bregs[AL];
    register unsigned tmp2 = tmp;

    tmp2 += src;

    SetCFB_Add(tmp2,tmp);
    SetOFB_Add(tmp2,src,tmp);
    SetAF(tmp2,src,tmp);
    SetZFB(tmp2);
    SetSFB(tmp2);
    SetPF(tmp2);

    *bregs[AL] = (BYTE)tmp2;
}


static INLINE2 void i_add_axd16(void)
{
    /* Opcode 0x05 */

    register unsigned src;
    register unsigned tmp = ReadWord(&wregs[AX]);
    register unsigned tmp2 = tmp;

    src = GetMemInc(c_cs,ip);
    src += GetMemInc(c_cs,ip) << 8;

    tmp2 += src;

    SetCFW_Add(tmp2,tmp);
    SetOFW_Add(tmp2,tmp,src);
    SetAF(tmp2,tmp,src);
    SetZFW(tmp2);
    SetSFW(tmp2);
    SetPF(tmp2);

    WriteWord(&wregs[AX],tmp2);
}


static INLINE2 void i_push_es(void)
{
    /* Opcode 0x06 */

    PushSeg(ES);
}


static INLINE2 void i_pop_es(void)
{
    /* Opcode 0x07 */
    PopSeg(c_es,ES);
}

    /* most OR instructions go here... */

LogicalOp(or,|)


static INLINE2 void i_push_cs(void)
{
    /* Opcode 0x0e */

    PushSeg(CS);
}


static INLINE2 void i_adc_br8(void)
{
    /* Opcode 0x10 */

    unsigned ModRM = (unsigned)GetMemInc(c_cs,ip);
    register unsigned src = (unsigned)*GetModRMRegB(ModRM)+CF;
    BYTE *dest = GetModRMRMB(ModRM);
    register unsigned tmp = (unsigned) *dest;
    register unsigned tmp2 = tmp;

    tmp += src;

    CF = tmp >> 8;

/*    SetCFB_Add(tmp,tmp2); */
    SetOFB_Add(tmp,src,tmp2);
    SetAF(tmp,src,tmp2);
    SetZFB(tmp);
    SetSFB(tmp);
    SetPF(tmp);

    *dest = (BYTE)tmp;
}


static INLINE2 void i_adc_wr16(void)
{
    /* Opcode 0x11 */

    unsigned ModRM = GetMemInc(c_cs,ip);
    WORD *src = GetModRMRegW(ModRM);
    WORD *dest = GetModRMRMW(ModRM);
    register unsigned tmp1 = (unsigned)ReadWord(dest);
    register unsigned tmp2 = (unsigned)ReadWord(src)+CF;
    register unsigned tmp3;

    tmp3 = tmp1+tmp2;

    CF = tmp3 >> 16;
/*    SetCFW_Add(tmp3,tmp1); */
    SetOFW_Add(tmp3,tmp2,tmp1);
    SetAF(tmp3,tmp2,tmp1);
    SetZFW(tmp3);
    SetSFW(tmp3);
    SetPF(tmp3);

    WriteWord(dest, tmp3);

}


static INLINE2 void i_adc_r8b(void)
{
    /* Opcode 0x12 */

    unsigned ModRM = (unsigned)GetMemInc(c_cs,ip);
    register unsigned src = (unsigned)*GetModRMRMB(ModRM)+CF;
    BYTE *dest = GetModRMRegB(ModRM);
    register unsigned tmp = (unsigned) *dest;
    register unsigned tmp2 = tmp;

    tmp += src;

    CF = tmp >> 8;

/*    SetCFB_Add(tmp,tmp2); */
    SetOFB_Add(tmp,src,tmp2);
    SetAF(tmp,src,tmp2);
    SetZFB(tmp);
    SetSFB(tmp);
    SetPF(tmp);

    *dest = (BYTE)tmp;
}


static INLINE2 void i_adc_r16w(void)
{
    /* Opcode 0x13 */

    unsigned ModRM = GetMemInc(c_cs,ip);
    WORD *dest = GetModRMRegW(ModRM);
    WORD *src = GetModRMRMW(ModRM);
    register unsigned tmp1 = (unsigned)ReadWord(dest);
    register unsigned tmp2 = (unsigned)ReadWord(src)+CF;
    register unsigned tmp3;

    tmp3 = tmp1+tmp2;

    CF = tmp3 >> 16;

/*    SetCFW_Add(tmp3,tmp1); */
    SetOFW_Add(tmp3,tmp2,tmp1);
    SetAF(tmp3,tmp2,tmp1);
    SetZFW(tmp3);
    SetSFW(tmp3);
    SetPF(tmp3);

    WriteWord(dest, tmp3);

}


static INLINE2 void i_adc_ald8(void)
{
    /* Opcode 0x14 */

    register unsigned src = (unsigned)GetMemInc(c_cs,ip)+CF;
    register unsigned tmp = (unsigned)*bregs[AL];
    register unsigned tmp2 = tmp;

    tmp2 += src;

    CF = tmp2 >> 8;

/*    SetCFB_Add(tmp2,tmp); */
    SetOFB_Add(tmp2,src,tmp);
    SetAF(tmp2,src,tmp);
    SetZFB(tmp2);
    SetSFB(tmp2);
    SetPF(tmp2);

    *bregs[AL] = (BYTE)tmp2;
}


static INLINE2 void i_adc_axd16(void)
{
    /* Opcode 0x15 */

    register unsigned src;
    register unsigned tmp = ReadWord(&wregs[AX]);
    register unsigned tmp2 = tmp;

    src = GetMemInc(c_cs,ip);
    src += (GetMemInc(c_cs,ip) << 8)+CF;

    tmp2 += src;

    CF = tmp2 >> 16;

/*    SetCFW_Add(tmp2,tmp); */
    SetOFW_Add(tmp2,tmp,src);
    SetAF(tmp2,tmp,src);
    SetZFW(tmp2);
    SetSFW(tmp2);
    SetPF(tmp2);

    WriteWord(&wregs[AX],tmp2);
}


static INLINE2 void i_push_ss(void)
{
    /* Opcode 0x16 */

    PushSeg(SS);
}


static INLINE2 void i_pop_ss(void)
{
    /* Opcode 0x17 */

    static int multiple = 0;

    PopSeg(c_ss,SS);
    c_stack = c_ss;
    
    if (multiple == 0)	/* prevent unlimited recursion */
    {
        multiple = 1;
/*
#ifdef DEBUGGER
        call_debugger(D_TRACE);
#endif
*/
        instruction[GetMemInc(c_cs,ip)]();
        multiple = 0;
    }
}


static INLINE2 void i_sbb_br8(void)
{
    /* Opcode 0x18 */

    unsigned ModRM = (unsigned)GetMemInc(c_cs,ip);
    register unsigned src = (unsigned)*GetModRMRegB(ModRM)+CF;
    BYTE *dest = GetModRMRMB(ModRM);
    register unsigned tmp = (unsigned) *dest;
    register unsigned tmp2 = tmp;

    tmp -= src;

    CF = (tmp & 0x100) == 0x100;

/*    SetCFB_Sub(tmp,tmp2); */
    SetOFB_Sub(tmp,src,tmp2);
    SetAF(tmp,src,tmp2);
    SetZFB(tmp);
    SetSFB(tmp);
    SetPF(tmp);

    *dest = (BYTE)tmp;
}


static INLINE2 void i_sbb_wr16(void)
{
    /* Opcode 0x19 */

    unsigned ModRM = GetMemInc(c_cs,ip);
    WORD *src = GetModRMRegW(ModRM);
    WORD *dest = GetModRMRMW(ModRM);
    register unsigned tmp1 = ReadWord(dest);
    register unsigned tmp2 = ReadWord(src)+CF;
    register unsigned tmp3;

    tmp3 = tmp1-tmp2;

    CF = (tmp3 & 0x10000) == 0x10000;

/*    SetCFW_Sub(tmp2,tmp1); */
    SetOFW_Sub(tmp3,tmp2,tmp1);
    SetAF(tmp3,tmp2,tmp1);
    SetZFW(tmp3);
    SetSFW(tmp3);
    SetPF(tmp3);

    WriteWord(dest, tmp3);
}


static INLINE2 void i_sbb_r8b(void)
{
    /* Opcode 0x1a */

    unsigned ModRM = (unsigned)GetMemInc(c_cs,ip);
    register unsigned src = (unsigned)*GetModRMRMB(ModRM)+CF;
    BYTE *dest = GetModRMRegB(ModRM);
    register unsigned tmp = (unsigned) *dest;
    register unsigned tmp2 = tmp;

    tmp -= src;

    CF = (tmp & 0x100) == 0x100;

/*    SetCFB_Sub(tmp,tmp2); */
    SetOFB_Sub(tmp,src,tmp2);
    SetAF(tmp,src,tmp2);
    SetZFB(tmp);
    SetSFB(tmp);
    SetPF(tmp);

    *dest = (BYTE)tmp;

}


static INLINE2 void i_sbb_r16w(void)
{
    /* Opcode 0x1b */

    unsigned ModRM = GetMemInc(c_cs,ip);
    WORD *dest = GetModRMRegW(ModRM);
    WORD *src = GetModRMRMW(ModRM);
    register unsigned tmp1 = ReadWord(dest);
    register unsigned tmp2 = ReadWord(src)+CF;
    register unsigned tmp3;

    tmp3 = tmp1-tmp2;

    CF = (tmp3 & 0x10000) == 0x10000;

/*    SetCFW_Sub(tmp2,tmp1); */
    SetOFW_Sub(tmp3,tmp2,tmp1);
    SetAF(tmp3,tmp2,tmp1);
    SetZFW(tmp3);
    SetSFW(tmp3);
    SetPF(tmp3);

    WriteWord(dest, tmp3);
}


static INLINE2 void i_sbb_ald8(void)
{
    /* Opcode 0x1c */

    register unsigned src = GetMemInc(c_cs,ip)+CF;
    register unsigned tmp = *bregs[AL];
    register unsigned tmp1 = tmp;

    tmp1 -= src;

    CF = (tmp1 & 0x100) == 0x100;

/*    SetCFB_Sub(src,tmp); */
    SetOFB_Sub(tmp1,src,tmp);
    SetAF(tmp1,src,tmp);
    SetZFB(tmp1);
    SetSFB(tmp1);
    SetPF(tmp1);

    *bregs[AL] = (BYTE)tmp1;
}


static INLINE2 void i_sbb_axd16(void)
{
    /* Opcode 0x1d */

    register unsigned src;
    register unsigned tmp = ReadWord(&wregs[AX]);
    register unsigned tmp2 = tmp;

    src = GetMemInc(c_cs,ip);
    src += (GetMemInc(c_cs,ip) << 8)+CF;

    tmp2 -= src;

    CF = (tmp2 & 0x10000) == 0x10000;

/*    SetCFW_Sub(src,tmp); */
    SetOFW_Sub(tmp2,src,tmp);
    SetAF(tmp2,tmp,src);
    SetZFW(tmp2);
    SetSFW(tmp2);
    SetPF(tmp2);

    WriteWord(&wregs[AX],tmp2);
}


static INLINE2 void i_push_ds(void)
{
    /* Opcode 0x1e */

    PushSeg(DS);
}


static INLINE2 void i_pop_ds(void)
{
    /* Opcode 0x1f */
    PopSeg(c_ds,DS);
}

    /* most AND instructions go here... */

LogicalOp(and,&)


static INLINE2 void i_es(void)
{
    /* Opcode 0x26 */

    c_ds = c_ss = c_es;

    instruction[GetMemInc(c_cs,ip)]();

    c_ds = SegToMemPtr(DS);
    c_ss = SegToMemPtr(SS);
}

static INLINE2 void i_daa(void)
{
    if (AF || ((*bregs[AL] & 0xf) > 9))
    {
        *bregs[AL] += 6;
        AF = 1;
    }
    else
        AF = 0;

    if (CF || (*bregs[AL] > 0x9f))
    {
        *bregs[AL] += 0x60;
        CF = 1;
    }
    else
        CF = 0;

    SetPF(*bregs[AL]);
    SetSFB(*bregs[AL]);
    SetZFB(*bregs[AL]);
}

static INLINE2 void i_sub_br8(void)
{
    /* Opcode 0x28 */

    unsigned ModRM = (unsigned)GetMemInc(c_cs,ip);
    register unsigned src = (unsigned)*GetModRMRegB(ModRM);
    BYTE *dest = GetModRMRMB(ModRM);
    register unsigned tmp = (unsigned) *dest;
    register unsigned tmp2 = tmp;

    tmp -= src;

    SetCFB_Sub(tmp,tmp2);
    SetOFB_Sub(tmp,src,tmp2);
    SetAF(tmp,src,tmp2);
    SetZFB(tmp);
    SetSFB(tmp);
    SetPF(tmp);

    *dest = (BYTE)tmp;

}


static INLINE2 void i_sub_wr16(void)
{
    /* Opcode 0x29 */

    unsigned ModRM = GetMemInc(c_cs,ip);
    WORD *src = GetModRMRegW(ModRM);
    WORD *dest = GetModRMRMW(ModRM);
    register unsigned tmp1 = ReadWord(dest);
    register unsigned tmp2 = ReadWord(src);
    register unsigned tmp3;

    tmp3 = tmp1-tmp2;

    SetCFW_Sub(tmp2,tmp1);
    SetOFW_Sub(tmp3,tmp2,tmp1);
    SetAF(tmp3,tmp2,tmp1);
    SetZFW(tmp3);
    SetSFW(tmp3);
    SetPF(tmp3);

    WriteWord(dest, tmp3);
}


static INLINE2 void i_sub_r8b(void)
{
    /* Opcode 0x2a */

    unsigned ModRM = (unsigned)GetMemInc(c_cs,ip);
    BYTE *dest = GetModRMRegB(ModRM);
    register unsigned src = *GetModRMRMB(ModRM);
    register unsigned tmp = *dest;
    register unsigned tmp2 = tmp;

    tmp -= src;

    SetCFB_Sub(tmp,tmp2);
    SetOFB_Sub(tmp,src,tmp2);
    SetAF(tmp,src,tmp2);
    SetZFB(tmp);
    SetSFB(tmp);
    SetPF(tmp);

    *dest = (BYTE)tmp;
}


static INLINE2 void i_sub_r16w(void)
{
    /* Opcode 0x2b */

    unsigned ModRM = GetMemInc(c_cs,ip);
    WORD *dest = GetModRMRegW(ModRM);
    WORD *src = GetModRMRMW(ModRM);
    register unsigned tmp1 = ReadWord(dest);
    register unsigned tmp2 = ReadWord(src);
    register unsigned tmp3;

    tmp3 = tmp1-tmp2;

    SetCFW_Sub(tmp2,tmp1);
    SetOFW_Sub(tmp3,tmp2,tmp1);
    SetAF(tmp3,tmp2,tmp1);
    SetZFW(tmp3);
    SetSFW(tmp3);
    SetPF(tmp3);

    WriteWord(dest, tmp3);
}


static INLINE2 void i_sub_ald8(void)
{
    /* Opcode 0x2c */

    register unsigned src = GetMemInc(c_cs,ip);
    register unsigned tmp = *bregs[AL];
    register unsigned tmp1 = tmp;

    tmp1 -= src;

    SetCFB_Sub(src,tmp);
    SetOFB_Sub(tmp1,src,tmp);
    SetAF(tmp1,src,tmp);
    SetZFB(tmp1);
    SetSFB(tmp1);
    SetPF(tmp1);

    *bregs[AL] = (unsigned) tmp1;
}


static INLINE2 void i_sub_axd16(void)
{
    /* Opcode 0x2d */

    register unsigned src;
    register unsigned tmp = ReadWord(&wregs[AX]);
    register unsigned tmp2 = tmp;

    src = GetMemInc(c_cs,ip);
    src += (GetMemInc(c_cs,ip) << 8);

    tmp2 -= src;

    SetCFW_Sub(src,tmp);
    SetOFW_Sub(tmp2,src,tmp);
    SetAF(tmp2,tmp,src);
    SetZFW(tmp2);
    SetSFW(tmp2);
    SetPF(tmp2);

    WriteWord(&wregs[AX],tmp2);
}


static INLINE2 void i_cs(void)
{
    /* Opcode 0x2e */

    c_ds = c_ss = c_cs;

    instruction[GetMemInc(c_cs,ip)]();

    c_ds = SegToMemPtr(DS);
    c_ss = SegToMemPtr(SS);
}


    /* most XOR instructions go here */

LogicalOp(xor,^)


static INLINE2 void i_ss(void)
{
    /* Opcode 0x36 */

    c_ds = c_ss;

    instruction[GetMemInc(c_cs,ip)]();

    c_ds = SegToMemPtr(DS);
}

static INLINE2 void i_aaa(void)
{
    /* Opcode 0x37 */
    if ((*bregs[AL] & 0x0f) > 9 || AF) {
	*bregs[AL] += 6;
	*bregs[AL] &= 0x0f;
	(*bregs[AH])++;
	AF = 1;
	CF = 1;
    }
    else {
	AF = 0;
	CF = 0;
    }
}

static INLINE2 void i_cmp_br8(void)
{
    /* Opcode 0x38 */

    unsigned ModRM = (unsigned)GetMemInc(c_cs,ip);
    register unsigned src = (unsigned)*GetModRMRegB(ModRM);
    register unsigned tmp = *GetModRMRMB(ModRM);
    register unsigned tmp2 = tmp;

    tmp -= src;

    SetCFB_Sub(tmp,tmp2);
    SetOFB_Sub(tmp,src,tmp2);
    SetAF(tmp,src,tmp2);
    SetZFB(tmp);
    SetSFB(tmp);
    SetPF(tmp);
}


static INLINE2 void i_cmp_wr16(void)
{
    /* Opcode 0x39 */

    unsigned ModRM = GetMemInc(c_cs,ip);
    WORD *src = GetModRMRegW(ModRM);
    WORD *dest = GetModRMRMW(ModRM);
    register unsigned tmp1 = ReadWord(dest);
    register unsigned tmp2 = ReadWord(src);
    register unsigned tmp3;

    tmp3 = tmp1-tmp2;

    SetCFW_Sub(tmp2,tmp1);
    SetOFW_Sub(tmp3,tmp2,tmp1);
    SetAF(tmp3,tmp2,tmp1);
    SetZFW(tmp3);
    SetSFW(tmp3);
    SetPF(tmp3);
}


static INLINE2 void i_cmp_r8b(void)
{
    /* Opcode 0x3a */

    unsigned ModRM = (unsigned)GetMemInc(c_cs,ip);
    register unsigned tmp = (unsigned)*GetModRMRegB(ModRM);
    register unsigned src = *GetModRMRMB(ModRM);
    register unsigned tmp2 = tmp;

    tmp -= src;

    SetCFB_Sub(tmp,tmp2);
    SetOFB_Sub(tmp,src,tmp2);
    SetAF(tmp,src,tmp2);
    SetZFB(tmp);
    SetSFB(tmp);
    SetPF(tmp);
}

static INLINE2 void i_cmp_r16w(void)
{
    /* Opcode 0x3b */

    unsigned ModRM = GetMemInc(c_cs,ip);
    WORD *dest = GetModRMRegW(ModRM);
    WORD *src = GetModRMRMW(ModRM);
    register unsigned tmp1 = ReadWord(dest);
    register unsigned tmp2 = ReadWord(src);
    register unsigned tmp3;

    tmp3 = tmp1-tmp2;

    SetCFW_Sub(tmp2,tmp1);
    SetOFW_Sub(tmp3,tmp2,tmp1);
    SetAF(tmp3,tmp2,tmp1);
    SetZFW(tmp3);
    SetSFW(tmp3);
    SetPF(tmp3);
}


static INLINE2 void i_cmp_ald8(void)
{
    /* Opcode 0x3c */

    register unsigned src = GetMemInc(c_cs,ip);
    register unsigned tmp = *bregs[AL];
    register unsigned tmp1 = tmp;

    tmp1 -= src;

    SetCFB_Sub(src,tmp);
    SetOFB_Sub(tmp1,src,tmp);
    SetAF(tmp1,src,tmp);
    SetZFB(tmp1);
    SetSFB(tmp1);
    SetPF(tmp1);
}


static INLINE2 void i_cmp_axd16(void)
{
    /* Opcode 0x3d */

    register unsigned src;
    register unsigned tmp = ReadWord(&wregs[AX]);
    register unsigned tmp2 = tmp;

    src = GetMemInc(c_cs,ip);
    src += (GetMemInc(c_cs,ip) << 8);

    tmp2 -= src;

    SetCFW_Sub(src,tmp);
    SetOFW_Sub(tmp2,src,tmp);
    SetAF(tmp2,tmp,src);
    SetZFW(tmp2);
    SetSFW(tmp2);
    SetPF(tmp2);
}


static INLINE2 void i_ds(void)
{
    /* Opcode 0x3e */

    c_ss = c_ds;

    instruction[GetMemInc(c_cs,ip)]();

    c_ss = SegToMemPtr(SS);
}

static INLINE2 void i_aas(void)
{
    /* Opcode 0x3f */
    if ((*bregs[AL] & 0x0f) > 9 || AF) {
	*bregs[AL] -= 6;
	*bregs[AL] &= 0x0f;
	(*bregs[AH])--;
	AF = 1;
	CF = 1;
    }
    else {
	AF = 0;
	CF = 0;
    }
}

static INLINE2 void i_inc_ax(void)
{
    /* Opcode 0x40 */
    IncWordReg(AX);
}


static INLINE2 void i_inc_cx(void)
{
    /* Opcode 0x41 */
    IncWordReg(CX);
}


static INLINE2 void i_inc_dx(void)
{
    /* Opcode 0x42 */
    IncWordReg(DX);
}


static INLINE2 void i_inc_bx(void)
{
    /* Opcode 0x43 */
    IncWordReg(BX);
}


static INLINE2 void i_inc_sp(void)
{
    /* Opcode 0x44 */
    IncWordReg(SP);
}


static INLINE2 void i_inc_bp(void)
{
    /* Opcode 0x45 */
    IncWordReg(BP);
}


static INLINE2 void i_inc_si(void)
{
    /* Opcode 0x46 */
    IncWordReg(SI);
}


static INLINE2 void i_inc_di(void)
{
    /* Opcode 0x47 */
    IncWordReg(DI);
}


static INLINE2 void i_dec_ax(void)
{
    /* Opcode 0x48 */
    DecWordReg(AX);
}


static INLINE2 void i_dec_cx(void)
{
    /* Opcode 0x49 */
    DecWordReg(CX);
}


static INLINE2 void i_dec_dx(void)
{
    /* Opcode 0x4a */
    DecWordReg(DX);
}


static INLINE2 void i_dec_bx(void)
{
    /* Opcode 0x4b */
    DecWordReg(BX);
}


static INLINE2 void i_dec_sp(void)
{
    /* Opcode 0x4c */
    DecWordReg(SP);
}


static INLINE2 void i_dec_bp(void)
{
    /* Opcode 0x4d */
    DecWordReg(BP);
}


static INLINE2 void i_dec_si(void)
{
    /* Opcode 0x4e */
    DecWordReg(SI);
}


static INLINE2 void i_dec_di(void)
{
    /* Opcode 0x4f */
    DecWordReg(DI);
}


static INLINE2 void i_push_ax(void)
{
    /* Opcode 0x50 */
    PushWordReg(AX);
}


static INLINE2 void i_push_cx(void)
{
    /* Opcode 0x51 */
    PushWordReg(CX);
}


static INLINE2 void i_push_dx(void)
{
    /* Opcode 0x52 */
    PushWordReg(DX);
}


static INLINE2 void i_push_bx(void)
{
    /* Opcode 0x53 */
    PushWordReg(BX);
}


static INLINE2 void i_push_sp(void)
{
    /* Opcode 0x54 */
    PushWordReg(SP);
}


static INLINE2 void i_push_bp(void)
{
    /* Opcode 0x55 */
    PushWordReg(BP);
}



static INLINE2 void i_push_si(void)
{
    /* Opcode 0x56 */
    PushWordReg(SI);
}


static INLINE2 void i_push_di(void)
{
    /* Opcode 0x57 */
    PushWordReg(DI);
}


static INLINE2 void i_pop_ax(void)
{
    /* Opcode 0x58 */
    PopWordReg(AX);
}


static INLINE2 void i_pop_cx(void)
{
    /* Opcode 0x59 */
    PopWordReg(CX);
}


static INLINE2 void i_pop_dx(void)
{
    /* Opcode 0x5a */
    PopWordReg(DX);
}


static INLINE2 void i_pop_bx(void)
{
    /* Opcode 0x5b */
    PopWordReg(BX);
}


static INLINE2 void i_pop_sp(void)
{
    /* Opcode 0x5c */
    PopWordReg(SP);
}


static INLINE2 void i_pop_bp(void)
{
    /* Opcode 0x5d */
    PopWordReg(BP);
}


static INLINE2 void i_pop_si(void)
{
    /* Opcode 0x5e */
    PopWordReg(SI);
}


static INLINE2 void i_pop_di(void)
{
    /* Opcode 0x5f */
    PopWordReg(DI);
}

static INLINE2 void i_push_immed16(void)
{
    /* 0x68 */
    register unsigned tmp1 = (WORD)(ReadWord(&wregs[SP])-2);
    unsigned val = GetMemInc(c_cs, ip);
    val |= GetMemInc(c_cs, ip) << 8;

    WriteWord(&wregs[SP],tmp1);
    PutMemW(c_stack,tmp1,val);
}

static INLINE2 void i_imul_r16_immed16(void)
{
    /* PENDING */
    /* 0x69 */
}

static INLINE2 void i_outs_dx_rm8(void)
{
    /* PENDING */
    /* 0x6E */
}

static INLINE2 void i_outs_dx_rm16(void)
{
    /* PENDING */
    /* 0x6F */
}

    /* Conditional jumps from 0x70 to 0x7f */

JumpCond(o, OF)                 /* 0x70 = Jump if overflow */
JumpCond(no, !OF)               /* 0x71 = Jump if no overflow */
JumpCond(b, CF)                 /* 0x72 = Jump if below */
JumpCond(nb, !CF)               /* 0x73 = Jump if not below */
JumpCond(z, ZF)                 /* 0x74 = Jump if zero */
JumpCond(nz, !ZF)               /* 0x75 = Jump if not zero */
JumpCond(be, CF || ZF)          /* 0x76 = Jump if below or equal */
JumpCond(nbe, !(CF || ZF))      /* 0x77 = Jump if not below or equal */
JumpCond(s, SF)                 /* 0x78 = Jump if sign */
JumpCond(ns, !SF)               /* 0x79 = Jump if no sign */
JumpCond(p, PF)                 /* 0x7a = Jump if parity */
JumpCond(np, !PF)               /* 0x7b = Jump if no parity */
JumpCond(l,(!(!SF)!=!(!OF))&&!ZF)    /* 0x7c = Jump if less */
JumpCond(nl, ZF||(!(!SF) == !(!OF))) /* 0x7d = Jump if not less */
JumpCond(le, ZF||(!(!SF) != !(!OF))) /* 0x7e = Jump if less than or equal */
JumpCond(nle,(!(!SF)==!(!OF))&&!ZF)  /* 0x7f = Jump if not less than or equal*/


static INLINE2 void i_80pre(void)
{
    /* Opcode 0x80 */
    unsigned ModRM = GetMemInc(c_cs,ip);
    BYTE *dest = GetModRMRMB(ModRM);
    register unsigned src = GetMemInc(c_cs,ip);
    register unsigned tmp = *dest;
    register unsigned tmp2;
    
    
    switch (ModRM & 0x38)
    {
    case 0x00:  /* ADD eb,d8 */
        tmp2 = src + tmp;
        
        SetCFB_Add(tmp2,tmp);
        SetOFB_Add(tmp2,src,tmp);
        SetAF(tmp2,src,tmp);
        SetZFB(tmp2);
        SetSFB(tmp2);
        SetPF(tmp2);
        
        *dest = (BYTE)tmp2;
        break;
    case 0x08:  /* OR eb,d8 */
        tmp |= src;
        
        CF = OF = AF = 0;
        
        SetZFB(tmp);
        SetSFB(tmp);
        SetPF(tmp);
        
        *dest = (BYTE)tmp;
        break;
    case 0x10:  /* ADC eb,d8 */
        src += CF;
        tmp2 = src + tmp;
        
        CF = tmp2 >> 8;
/*        SetCFB_Add(tmp2,tmp); */
        SetOFB_Add(tmp2,src,tmp);
        SetAF(tmp2,src,tmp);
        SetZFB(tmp2);
        SetSFB(tmp2);
        SetPF(tmp2);
        
        *dest = (BYTE)tmp2;
        break;
    case 0x18:  /* SBB eb,b8 */
        src += CF;
        tmp2 = tmp;
        tmp -= src;
        
        CF = (tmp & 0x100) == 0x100;

/*        SetCFB_Sub(tmp,tmp2); */
        SetOFB_Sub(tmp,src,tmp2);
        SetAF(tmp,src,tmp2);
        SetZFB(tmp);
        SetSFB(tmp);
        SetPF(tmp);
        
        *dest = (BYTE)tmp;
        break;
    case 0x20:  /* AND eb,d8 */
        tmp &= src;
        
        CF = OF = AF = 0;
        
        SetZFB(tmp);
        SetSFB(tmp);
        SetPF(tmp);
        
        *dest = (BYTE)tmp;
        break;
    case 0x28:  /* SUB eb,d8 */
        tmp2 = tmp;
        tmp -= src;
        
        SetCFB_Sub(tmp,tmp2);
        SetOFB_Sub(tmp,src,tmp2);
        SetAF(tmp,src,tmp2);
        SetZFB(tmp);
        SetSFB(tmp);
        SetPF(tmp);
        
        *dest = (BYTE)tmp;
        break;
    case 0x30:  /* XOR eb,d8 */
        tmp ^= src;
        
        CF = OF = AF = 0;
        
        SetZFB(tmp);
        SetSFB(tmp);
        SetPF(tmp);
        
        *dest = (BYTE)tmp;
        break;
    case 0x38:  /* CMP eb,d8 */
        tmp2 = tmp;
        tmp -= src;
        
        SetCFB_Sub(tmp,tmp2);
        SetOFB_Sub(tmp,src,tmp2);
        SetAF(tmp,src,tmp2);
        SetZFB(tmp);
        SetSFB(tmp);
        SetPF(tmp);
        
        break;
    }
}


static INLINE2 void i_81pre(void)
{
    /* Opcode 0x81 */

    unsigned ModRM = GetMemInc(c_cs,ip);
    WORD *dest = GetModRMRMW(ModRM);
    register unsigned src = GetMemInc(c_cs,ip);
    register unsigned tmp = ReadWord(dest);
    register unsigned tmp2;

    src += GetMemInc(c_cs,ip) << 8;

    switch (ModRM & 0x38)
    {
    case 0x00:  /* ADD ew,d16 */
        tmp2 = src + tmp;
        
        SetCFW_Add(tmp2,tmp);
        SetOFW_Add(tmp2,src,tmp);
        SetAF(tmp2,src,tmp);
        SetZFW(tmp2);
        SetSFW(tmp2);
        SetPF(tmp2);
        
        WriteWord(dest,tmp2);
        break;
    case 0x08:  /* OR ew,d16 */
        tmp |= src;
        
        CF = OF = AF = 0;
        
        SetZFW(tmp);
        SetSFW(tmp);
        SetPF(tmp);
        
        WriteWord(dest,tmp);
        break;
    case 0x10:  /* ADC ew,d16 */
        src += CF;
        tmp2 = src + tmp;
        
        CF = tmp2 >> 16;
/*        SetCFW_Add(tmp2,tmp); */
        SetOFW_Add(tmp2,src,tmp);
        SetAF(tmp2,src,tmp);
        SetZFW(tmp2);
        SetSFW(tmp2);
        SetPF(tmp2);
        
        WriteWord(dest,tmp2);
        break;
    case 0x18:  /* SBB ew,d16 */
        src += CF;
        tmp2 = tmp;
        tmp -= src;
        
        CF = (tmp & 0x10000) == 0x10000;

/*        SetCFW_Sub(tmp,tmp2); */
        SetOFW_Sub(tmp,src,tmp2);
        SetAF(tmp,src,tmp2);
        SetZFW(tmp);
        SetSFW(tmp);
        SetPF(tmp);
        
        WriteWord(dest,tmp);
        break;
    case 0x20:  /* AND ew,d16 */
        tmp &= src;
        
        CF = OF = AF = 0;
        
        SetZFW(tmp);
        SetSFW(tmp);
        SetPF(tmp);
        
        WriteWord(dest,tmp);
        break;
    case 0x28:  /* SUB ew,d16 */
        tmp2 = tmp;
        tmp -= src;
        
        SetCFW_Sub(tmp,tmp2);
        SetOFW_Sub(tmp,src,tmp2);
        SetAF(tmp,src,tmp2);
        SetZFW(tmp);
        SetSFW(tmp);
        SetPF(tmp);
        
        WriteWord(dest,tmp);
        break;
    case 0x30:  /* XOR ew,d16 */
        tmp ^= src;
        
        CF = OF = AF = 0;
        
        SetZFW(tmp);
        SetSFW(tmp);
        SetPF(tmp);
        
        WriteWord(dest,tmp);
        break;
    case 0x38:  /* CMP ew,d16 */
        tmp2 = tmp;
        tmp -= src;
        
        SetCFW_Sub(tmp,tmp2);
        SetOFW_Sub(tmp,src,tmp2);
        SetAF(tmp,src,tmp2);
        SetZFW(tmp);
        SetSFW(tmp);
        SetPF(tmp);
        
        break;
    }
}


static INLINE2 void i_83pre(void)
{
    /* Opcode 0x83 */

    unsigned ModRM = GetMemInc(c_cs,ip);
    WORD *dest = GetModRMRMW(ModRM);
    register unsigned src = (WORD)((INT16)((INT8)GetMemInc(c_cs,ip)));
    register unsigned tmp = ReadWord(dest);
    register unsigned tmp2;
    
    switch (ModRM & 0x38)
    {
    case 0x00:  /* ADD ew,d8 */
        tmp2 = src + tmp;
        
        SetCFW_Add(tmp2,tmp);
        SetOFW_Add(tmp2,src,tmp);
        SetAF(tmp2,src,tmp);
        SetZFW(tmp2);
        SetSFW(tmp2);
        SetPF(tmp2);
        
        WriteWord(dest,tmp2);
        break;
    case 0x08:  /* OR ew,d8 */
        tmp |= src;
        
        CF = OF = AF = 0;
        
        SetZFW(tmp);
        SetSFW(tmp);
        SetPF(tmp);
        
        WriteWord(dest,tmp);
        break;
    case 0x10:  /* ADC ew,d8 */
        src += CF;
        tmp2 = src + tmp;
        
        CF = tmp2 >> 16;

/*        SetCFW_Add(tmp2,tmp); */
        SetOFW_Add(tmp2,src,tmp);
        SetAF(tmp2,src,tmp);
        SetZFW(tmp2);
        SetSFW(tmp2);
        SetPF(tmp2);
        
        WriteWord(dest,tmp2);
        break;
    case 0x18:  /* SBB ew,d8 */
        src += CF;
        tmp2 = tmp;
        tmp -= src;
        
        CF = (tmp & 0x10000) == 0x10000;

/*        SetCFW_Sub(tmp,tmp2); */
        SetOFW_Sub(tmp,src,tmp2);
        SetAF(tmp,src,tmp2);
        SetZFW(tmp);
        SetSFW(tmp);
        SetPF(tmp);
        
        WriteWord(dest,tmp);
        break;
    case 0x20:  /* AND ew,d8 */
        tmp &= src;
        
        CF = OF = AF = 0;
        
        SetZFW(tmp);
        SetSFW(tmp);
        SetPF(tmp);
        
        WriteWord(dest,tmp);
        break;
    case 0x28:  /* SUB ew,d8 */
        tmp2 = tmp;
        tmp -= src;
        
        SetCFW_Sub(tmp,tmp2);
        SetOFW_Sub(tmp,src,tmp2);
        SetAF(tmp,src,tmp2);
        SetZFW(tmp);
        SetSFW(tmp);
        SetPF(tmp);
        
        WriteWord(dest,tmp);
        break;
    case 0x30:  /* XOR ew,d8 */
        tmp ^= src;
        
        CF = OF = AF = 0;
        
        SetZFW(tmp);
        SetSFW(tmp);
        SetPF(tmp);
        
        WriteWord(dest,tmp);
        break;
        
    case 0x38:  /* CMP ew,d8 */
        tmp2 = tmp;
        tmp -= src;
        
        SetCFW_Sub(tmp,tmp2);
        SetOFW_Sub(tmp,src,tmp2);
        SetAF(tmp,src,tmp2);
        SetZFW(tmp);
        SetSFW(tmp);
        SetPF(tmp);
        
        break;
    }
}


static INLINE2 void i_test_br8(void)
{
    /* Opcode 0x84 */

    unsigned ModRM = (unsigned)GetMemInc(c_cs,ip);
    register unsigned src = (unsigned)*GetModRMRegB(ModRM);
    BYTE *dest = GetModRMRMB(ModRM);
    register unsigned tmp = (unsigned) *dest;
    tmp &= src;
    CF = OF = AF = 0;
    SetZFB(tmp);
    SetSFB(tmp);
    SetPF(tmp);
}


static INLINE2 void i_test_wr16(void)
{
    /* Opcode 0x85 */

    unsigned ModRM = GetMemInc(c_cs,ip);
    WORD *src = GetModRMRegW(ModRM);
    WORD *dest = GetModRMRMW(ModRM);
    register unsigned tmp1 = (unsigned)ReadWord(src);
    register unsigned tmp2 = (unsigned)ReadWord(dest);
    register unsigned tmp3 = tmp1 & tmp2;
    CF = OF = AF = 0;
    SetZFW(tmp3);
    SetSFW(tmp3);
    SetPF(tmp3);
}


static INLINE2 void i_xchg_br8(void)
{
    /* Opcode 0x86 */

    unsigned ModRM = (unsigned)GetMemInc(c_cs,ip);
    register BYTE *src = GetModRMRegB(ModRM);
    register BYTE *dest = GetModRMRMB(ModRM);
    BYTE tmp;

    tmp = *src;
    *src = *dest;
    *dest = tmp;
}


static INLINE2 void i_xchg_wr16(void)
{
    /* Opcode 0x87 */

    unsigned ModRM = GetMemInc(c_cs,ip);
    register BYTE *src = (BYTE *)GetModRMRegW(ModRM);
    register BYTE *dest = (BYTE *)GetModRMRMW(ModRM);
    BYTE tmp1,tmp2;

    tmp1 = src[0];
    tmp2 = src[1];
    src[0] = dest[0];
    src[1] = dest[1];
    dest[0] = tmp1;
    dest[1] = tmp2;

}


static INLINE2 void i_mov_br8(void)
{
    /* Opcode 0x88 */

    register unsigned ModRM = GetMemInc(c_cs,ip);
    register BYTE src = *GetModRMRegB(ModRM);
    register BYTE *dest = GetModRMRMB(ModRM);

    *dest = src;
}


static INLINE2 void i_mov_wr16(void)
{
    /* Opcode 0x89 */

    register unsigned ModRM = GetMemInc(c_cs,ip);
    register WORD *src = GetModRMRegW(ModRM);
    register WORD *dest = GetModRMRMW(ModRM);

    CopyWord(dest,src);
}


static INLINE2 void i_mov_r8b(void)
{
    /* Opcode 0x8a */

    register unsigned ModRM = GetMemInc(c_cs,ip);
    register BYTE *dest = GetModRMRegB(ModRM);
    register BYTE src = *GetModRMRMB(ModRM);

    *dest = src;
}


static INLINE2 void i_mov_r16w(void)
{
    /* Opcode 0x8b */

    register unsigned ModRM = GetMemInc(c_cs,ip);
    register WORD *dest = GetModRMRegW(ModRM);
    register WORD *src = GetModRMRMW(ModRM);

    CopyWord(dest,src);
}


static INLINE2 void i_mov_wsreg(void)
{
    /* Opcode 0x8c */

    register unsigned ModRM = GetMemInc(c_cs,ip);
    register WORD *dest = GetModRMRMW(ModRM);

    WriteWord(dest,sregs[(ModRM & 0x38) >> 3]);
}


static INLINE2 void i_lea(void)
{
    /* Opcode 0x8d */

    register unsigned ModRM = GetMemInc(c_cs,ip);
    register unsigned src = 0;
    register WORD *dest = GetModRMRegW(ModRM);

    if (ModRMRM[ModRM].offset)
    {
        src = (WORD)((INT16)((INT8)GetMemInc(c_cs,ip)));
        if (ModRMRM[ModRM].offset16)
            src = (GetMemInc(c_cs,ip) << 8) + (BYTE)(src);
    }

    src += ReadWord(ModRMRM[ModRM].reg1)+ReadWord(ModRMRM[ModRM].reg2);        
    WriteWord(dest,src);
}


static INLINE2 void i_mov_sregw(void)
{
    /* Opcode 0x8e */
    
    static int multiple = 0;
    register unsigned ModRM = GetMemInc(c_cs,ip);
    register WORD *src = GetModRMRMW(ModRM);
    
    switch (ModRM & 0x38)
    {
    case 0x00:	/* mov es,... */
        sregs[ES] = ReadWord(src);
        c_es = SegToMemPtr(ES);
        break;
    case 0x18:	/* mov ds,... */
        sregs[DS] = ReadWord(src);
        c_ds = SegToMemPtr(DS);
        break;
    case 0x10:	/* mov ss,... */
        sregs[SS] = ReadWord(src);
        c_stack = c_ss = SegToMemPtr(SS);
        
        if (multiple == 0) /* Prevent unlimited recursion.. */
        {
            multiple = 1;
/*
#ifdef DEBUGGER
            call_debugger(D_TRACE);
#endif
*/
            instruction[GetMemInc(c_cs,ip)]();
            multiple = 0;
        }
        
        break;
    case 0x08:	/* mov cs,... (hangs 486, but we'll let it through) */
        break;
        
    }
}


static INLINE2 void i_popw(void)
{
    /* Opcode 0x8f */
    
    unsigned ModRM = GetMemInc(c_cs,ip);
    register WORD *dest = GetModRMRMW(ModRM);
    register unsigned tmp = ReadWord(&wregs[SP]);
    WORD tmp2 = GetMemW(c_stack,tmp);
    tmp += 2;
    WriteWord(&wregs[SP],tmp);
    WriteWord(dest,tmp2);
}


static INLINE2 void i_nop(void)
{
    /* Opcode 0x90 */
}


static INLINE2 void i_xchg_axcx(void)
{
    /* Opcode 0x91 */
    XchgAXReg(CX);
}


static INLINE2 void i_xchg_axdx(void)
{
    /* Opcode 0x92 */
    XchgAXReg(DX);
}


static INLINE2 void i_xchg_axbx(void)
{
    /* Opcode 0x93 */
    XchgAXReg(BX);
}


static INLINE2 void i_xchg_axsp(void)
{
    /* Opcode 0x94 */
    XchgAXReg(SP);
}


static INLINE2 void i_xchg_axbp(void)
{
    /* Opcode 0x95 */
    XchgAXReg(BP);
}


static INLINE2 void i_xchg_axsi(void)
{
    /* Opcode 0x96 */
    XchgAXReg(SI);
}


static INLINE2 void i_xchg_axdi(void)
{
    /* Opcode 0x97 */
    XchgAXReg(DI);
}

static INLINE2 void i_cbw(void)
{
    /* Opcode 0x98 */

    *bregs[AH] = (*bregs[AL] & 0x80) ? 0xff : 0;
}
        
static INLINE2 void i_cwd(void)
{
    /* Opcode 0x99 */

    wregs[DX] = (*bregs[AH] & 0x80) ? ChangeE(0xffff) : ChangeE(0);
}


static INLINE2 void i_call_far(void)
{
    register unsigned tmp, tmp1, tmp2;

    tmp = GetMemInc(c_cs,ip);
    tmp += GetMemInc(c_cs,ip) << 8;

    tmp2 = GetMemInc(c_cs,ip);
    tmp2 += GetMemInc(c_cs,ip) << 8;

    tmp1 = (WORD)(ReadWord(&wregs[SP])-2);

    PutMemW(c_stack,tmp1,sregs[CS]);
    tmp1 = (WORD)(tmp1-2);
    PutMemW(c_stack,tmp1,ip);

    WriteWord(&wregs[SP],tmp1);

    ip = (WORD)tmp;
    sregs[CS] = (WORD)tmp2;
    c_cs = SegToMemPtr(CS);
}


static INLINE2 void i_wait(void)
{
    /* Opcode 0x9b */
    
    return;
}


static INLINE2 void i_pushf(void)
{
    /* Opcode 0x9c */

    register unsigned tmp1 = (ReadWord(&wregs[SP])-2);
    WORD tmp2 = CompressFlags() | 0xf000;

    PutMemW(c_stack,tmp1,tmp2);
    WriteWord(&wregs[SP],tmp1);
}


static INLINE2 void i_popf(void)
{
    /* Opcode 0x9d */

    register unsigned tmp = ReadWord(&wregs[SP]);
    unsigned tmp2 = (unsigned)GetMemW(c_stack,tmp);

    ExpandFlags(tmp2);
    tmp += 2;
    WriteWord(&wregs[SP],tmp);
    
    if (IF && int_blocked)
    {
        int_pending = int_blocked;
        int_blocked = 0;
        D2(printf("Unblocking interrupt\n"););
    }
    if (TF) trap();
}


static INLINE2 void i_sahf(void)
{
    /* Opcode 0x9e */

    unsigned tmp = (CompressFlags() & 0xff00) | (*bregs[AH] & 0xd5);

    ExpandFlags(tmp);
}


static INLINE2 void i_lahf(void)
{
    /* Opcode 0x9f */

    *bregs[AH] = CompressFlags() & 0xff;
}

static INLINE2 void i_mov_aldisp(void)
{
    /* Opcode 0xa0 */

    register unsigned addr;

    addr = GetMemInc(c_cs,ip);
    addr += GetMemInc(c_cs, ip) << 8;

    *bregs[AL] = GetMemB(c_ds, addr);
}


static INLINE2 void i_mov_axdisp(void)
{
    /* Opcode 0xa1 */

    register unsigned addr;

    addr = GetMemInc(c_cs, ip);
    addr += GetMemInc(c_cs, ip) << 8;

    *bregs[AL] = GetMemB(c_ds, addr);
    *bregs[AH] = GetMemB(c_ds, addr+1);
}


static INLINE2 void i_mov_dispal(void)
{
    /* Opcode 0xa2 */

    register unsigned addr;

    addr = GetMemInc(c_cs,ip);
    addr += GetMemInc(c_cs, ip) << 8;

    PutMemB(c_ds, addr, *bregs[AL]);
}


static INLINE2 void i_mov_dispax(void)
{
    /* Opcode 0xa3 */

    register unsigned addr;

    addr = GetMemInc(c_cs, ip);
    addr += GetMemInc(c_cs, ip) << 8;

    PutMemB(c_ds, addr, *bregs[AL]);
    PutMemB(c_ds, addr+1, *bregs[AH]);
}


static INLINE2 void i_movsb(void)
{
    /* Opcode 0xa4 */

    register unsigned di = ReadWord(&wregs[DI]);
    register unsigned si = ReadWord(&wregs[SI]);

    BYTE tmp = GetMemB(c_ds,si);

    PutMemB(c_es,di, tmp);

    di += -2*DF +1;
    si += -2*DF +1;

    WriteWord(&wregs[DI],di);
    WriteWord(&wregs[SI],si);
}


static INLINE2 void i_movsw(void)
{
    /* Opcode 0xa5 */

    register unsigned di = ReadWord(&wregs[DI]);
    register unsigned si = ReadWord(&wregs[SI]);
    
    WORD tmp = GetMemW(c_ds,si);

    PutMemW(c_es,di, tmp);

    di += -4*DF +2;
    si += -4*DF +2;

    WriteWord(&wregs[DI],di);
    WriteWord(&wregs[SI],si);
}


static INLINE2 void i_cmpsb(void)
{
    /* Opcode 0xa6 */

    unsigned di = ReadWord(&wregs[DI]);
    unsigned si = ReadWord(&wregs[SI]);
    unsigned src = GetMemB(c_es, di);
    unsigned tmp = GetMemB(c_ds, si);
    unsigned tmp2 = tmp;
    
    tmp -= src;

    SetCFB_Sub(tmp,tmp2);
    SetOFB_Sub(tmp,src,tmp2);
    SetAF(tmp,src,tmp2);
    SetZFB(tmp);
    SetSFB(tmp);
    SetPF(tmp);

    di += -2*DF +1;
    si += -2*DF +1;

    WriteWord(&wregs[DI],di);
    WriteWord(&wregs[SI],si);
}


static INLINE2 void i_cmpsw(void)
{
    /* Opcode 0xa7 */

    unsigned di = ReadWord(&wregs[DI]);
    unsigned si = ReadWord(&wregs[SI]);
    unsigned src = GetMemW(c_es, di);
    unsigned tmp = GetMemW(c_ds, si);
    unsigned tmp2 = tmp;
    
    tmp -= src;

    SetCFW_Sub(tmp,tmp2);
    SetOFW_Sub(tmp,src,tmp2);
    SetAF(tmp,src,tmp2);
    SetZFW(tmp);
    SetSFW(tmp);
    SetPF(tmp);

    di += -4*DF +2;
    si += -4*DF +2;

    WriteWord(&wregs[DI],di);
    WriteWord(&wregs[SI],si);
}


static INLINE2 void i_test_ald8(void)
{
    /* Opcode 0xa8 */

    register unsigned tmp1 = (unsigned)*bregs[AL];
    register unsigned tmp2 = (unsigned) GetMemInc(c_cs,ip);

    tmp1 &= tmp2;
    CF = OF = AF = 0;
    SetZFB(tmp1);
    SetSFB(tmp1);
    SetPF(tmp1);
}


static INLINE2 void i_test_axd16(void)
{
    /* Opcode 0xa9 */

    register unsigned tmp1 = (unsigned)ReadWord(&wregs[AX]);
    register unsigned tmp2;
    
    tmp2 = (unsigned) GetMemInc(c_cs,ip);
    tmp2 += GetMemInc(c_cs,ip) << 8;

    tmp1 &= tmp2;
    CF = OF = AF = 0;
    SetZFW(tmp1);
    SetSFW(tmp1);
    SetPF(tmp1);
}

static INLINE2 void i_stosb(void)
{
    /* Opcode 0xaa */

    register unsigned di = ReadWord(&wregs[DI]);

    PutMemB(c_es,di,*bregs[AL]);
    di += -2*DF +1;
    WriteWord(&wregs[DI],di);
}

static INLINE2 void i_stosw(void)
{
    /* Opcode 0xab */

    register unsigned di = ReadWord(&wregs[DI]);

    PutMemB(c_es,di,*bregs[AL]);
    PutMemB(c_es,di+1,*bregs[AH]);
    di += -4*DF +2;;
    WriteWord(&wregs[DI],di);
}

static INLINE2 void i_lodsb(void)
{
    /* Opcode 0xac */

    register unsigned si = ReadWord(&wregs[SI]);

    *bregs[AL] = GetMemB(c_ds,si);
    si += -2*DF +1;
    WriteWord(&wregs[SI],si);
}

static INLINE2 void i_lodsw(void)
{
    /* Opcode 0xad */

    register unsigned si = ReadWord(&wregs[SI]);
    register unsigned tmp = GetMemW(c_ds,si);

    si +=  -4*DF+2;
    WriteWord(&wregs[SI],si);
    WriteWord(&wregs[AX],tmp);
}
    
static INLINE2 void i_scasb(void)
{
    /* Opcode 0xae */

    unsigned di = ReadWord(&wregs[DI]);
    unsigned src = GetMemB(c_es, di);
    unsigned tmp = *bregs[AL];
    unsigned tmp2 = tmp;
    
    tmp -= src;

    SetCFB_Sub(tmp,tmp2);
    SetOFB_Sub(tmp,src,tmp2);
    SetAF(tmp,src,tmp2);
    SetZFB(tmp);
    SetSFB(tmp);
    SetPF(tmp);

    di += -2*DF +1;

    WriteWord(&wregs[DI],di);
}


static INLINE2 void i_scasw(void)
{
    /* Opcode 0xaf */

    unsigned di = ReadWord(&wregs[DI]);
    unsigned src = GetMemW(c_es, di);
    unsigned tmp = ReadWord(&wregs[AX]);
    unsigned tmp2 = tmp;
    
    tmp -= src;

    SetCFW_Sub(tmp,tmp2);
    SetOFW_Sub(tmp,src,tmp2);
    SetAF(tmp,src,tmp2);
    SetZFW(tmp);
    SetSFW(tmp);
    SetPF(tmp);

    di += -4*DF +2;

    WriteWord(&wregs[DI],di);
}

static INLINE2 void i_mov_ald8(void)
{
    /* Opcode 0xb0 */

    *bregs[AL] = GetMemInc(c_cs,ip);
}


static INLINE2 void i_mov_cld8(void)
{
    /* Opcode 0xb1 */

    *bregs[CL] = GetMemInc(c_cs,ip);
}


static INLINE2 void i_mov_dld8(void)
{
    /* Opcode 0xb2 */

    *bregs[DL] = GetMemInc(c_cs,ip);
}


static INLINE2 void i_mov_bld8(void)
{
    /* Opcode 0xb3 */

    *bregs[BL] = GetMemInc(c_cs,ip);
}


static INLINE2 void i_mov_ahd8(void)
{
    /* Opcode 0xb4 */

    *bregs[AH] = GetMemInc(c_cs,ip);
}


static INLINE2 void i_mov_chd8(void)
{
    /* Opcode 0xb5 */

    *bregs[CH] = GetMemInc(c_cs,ip);
}


static INLINE2 void i_mov_dhd8(void)
{
    /* Opcode 0xb6 */

    *bregs[DH] = GetMemInc(c_cs,ip);
}


static INLINE2 void i_mov_bhd8(void)
{
    /* Opcode 0xb7 */

    *bregs[BH] = GetMemInc(c_cs,ip);
}


static INLINE2 void i_mov_axd16(void)
{
    /* Opcode 0xb8 */

    *bregs[AL] = GetMemInc(c_cs,ip);
    *bregs[AH] = GetMemInc(c_cs,ip);
}


static INLINE2 void i_mov_cxd16(void)
{
    /* Opcode 0xb9 */

    *bregs[CL] = GetMemInc(c_cs,ip);
    *bregs[CH] = GetMemInc(c_cs,ip);
}


static INLINE2 void i_mov_dxd16(void)
{
    /* Opcode 0xba */

    *bregs[DL] = GetMemInc(c_cs,ip);
    *bregs[DH] = GetMemInc(c_cs,ip);
}


static INLINE2 void i_mov_bxd16(void)
{
    /* Opcode 0xbb */

    *bregs[BL] = GetMemInc(c_cs,ip);
    *bregs[BH] = GetMemInc(c_cs,ip);
}


static INLINE2 void i_mov_spd16(void)
{
    /* Opcode 0xbc */

    *bregs[SPL] = GetMemInc(c_cs,ip);
    *bregs[SPH] = GetMemInc(c_cs,ip);
}


static INLINE2 void i_mov_bpd16(void)
{
    /* Opcode 0xbd */

    *bregs[BPL] = GetMemInc(c_cs,ip);
    *bregs[BPH] = GetMemInc(c_cs,ip);
}


static INLINE2 void i_mov_sid16(void)
{
    /* Opcode 0xbe */

    *bregs[SIL] = GetMemInc(c_cs,ip);
    *bregs[SIH] = GetMemInc(c_cs,ip);
}


static INLINE2 void i_mov_did16(void)
{
    /* Opcode 0xbf */

    *bregs[DIL] = GetMemInc(c_cs,ip);
    *bregs[DIH] = GetMemInc(c_cs,ip);
}


static INLINE2 void i_ret_d16(void)
{
    /* Opcode 0xc2 */

    register unsigned tmp = ReadWord(&wregs[SP]);
    register unsigned count;

    count = GetMemInc(c_cs,ip);
    count += GetMemInc(c_cs,ip) << 8;

    ip = GetMemW(c_stack,tmp);
    tmp += count+2;
    WriteWord(&wregs[SP],tmp);
}


static INLINE2 void i_ret(void)
{
    /* Opcode 0xc3 */

    register unsigned tmp = ReadWord(&wregs[SP]);
    ip = GetMemW(c_stack,tmp);
    tmp += 2;
    WriteWord(&wregs[SP],tmp);
}


static INLINE2 void i_les_dw(void)
{
    /* Opcode 0xc4 */

    unsigned ModRM = GetMemInc(c_cs,ip);
    register WORD *dest = GetModRMRegW(ModRM);
    register WORD *src = GetModRMRMW(ModRM);
    WORD tmp = ReadWord(src);

    WriteWord(dest, tmp);
    src += 1;
    sregs[ES] = ReadWord(src);
    c_es = SegToMemPtr(ES);
}


static INLINE2 void i_lds_dw(void)
{
    /* Opcode 0xc5 */

    unsigned ModRM = GetMemInc(c_cs,ip);
    register WORD *dest = GetModRMRegW(ModRM);
    register WORD *src = GetModRMRMW(ModRM);
    WORD tmp = ReadWord(src);

    WriteWord(dest,tmp);
    src += 1;
    sregs[DS] = ReadWord(src);
    c_ds = SegToMemPtr(DS);
}

static INLINE2 void i_mov_bd8(void)
{
    /* Opcode 0xc6 */

    unsigned ModRM = GetMemInc(c_cs,ip);
    register BYTE *dest = GetModRMRMB(ModRM);

    *dest = GetMemInc(c_cs,ip);
}


static INLINE2 void i_mov_wd16(void)
{
    /* Opcode 0xc7 */

    unsigned ModRM = GetMemInc(c_cs,ip);
    register BYTE *dest = (BYTE *)GetModRMRMW(ModRM);

    *dest++ = GetMemInc(c_cs,ip);
    *dest = GetMemInc(c_cs,ip);
}


static INLINE2 void i_retf_d16(void)
{
    /* Opcode 0xca */

    register unsigned tmp = ReadWord(&wregs[SP]);
    register unsigned count;

    count = GetMemInc(c_cs,ip);
    count += GetMemInc(c_cs,ip) << 8;

    ip = GetMemW(c_stack,tmp);
    tmp = (WORD)(tmp+2);
    sregs[CS] = GetMemW(c_stack,tmp);
    c_cs = SegToMemPtr(CS);
    tmp += count+2;
    WriteWord(&wregs[SP],tmp);
}


static INLINE2 void i_retf(void)
{
    /* Opcode 0xcb */

    register unsigned tmp = ReadWord(&wregs[SP]);
    ip = GetMemW(c_stack,tmp);
    tmp = (WORD)(tmp+2);
    sregs[CS] = GetMemW(c_stack,tmp);
    c_cs = SegToMemPtr(CS);
    tmp += 2;
    WriteWord(&wregs[SP],tmp);
}


static INLINE2 void i_int3(void)
{
    /* Opcode 0xcc */

    interrupt(3);
}


static INLINE2 void i_int(void)
{
    /* Opcode 0xcd */

    unsigned int_num = GetMemInc(c_cs,ip);

    interrupt(int_num);
}


static INLINE2 void i_into(void)
{
    /* Opcode 0xce */

    if (OF)
        interrupt(4);
}


static INLINE2 void i_iret(void)
{
    /* Opcode 0xcf */

    register unsigned tmp = ReadWord(&wregs[SP]);
    ip = GetMemW(c_stack,tmp);
    tmp = (WORD)(tmp+2);
    sregs[CS] = GetMemW(c_stack,tmp);
    c_cs = SegToMemPtr(CS);
    tmp += 2;
    WriteWord(&wregs[SP],tmp);

    i_popf();
#ifdef DEBUGGER
    call_debugger(D_TRACE);
#endif
}    


static INLINE2 void i_d0pre(void)
{
    /* Opcode 0xd0 */

    unsigned ModRM = GetMemInc(c_cs,ip);
    register BYTE *dest = GetModRMRMB(ModRM);
    register unsigned tmp = *dest;
    register unsigned tmp2 = tmp;

    switch (ModRM & 0x38)
    {
    case 0x00:  /* ROL eb,1 */
        CF = (tmp & 0x80) != 0;
        *dest = (tmp << 1) + CF;
        OF = !(!(tmp & 0x40)) != CF;
        break;
    case 0x08:  /* ROR eb,1 */
        CF = (tmp & 0x01) != 0;
        *dest = (tmp >> 1) + (CF << 7);
        OF = !(!(tmp & 0x80)) != CF;
        break;
    case 0x10:  /* RCL eb,1 */
        OF = (tmp ^ (tmp << 1)) & 0x80;
        *dest = (tmp << 1) + CF;
        CF = (tmp & 0x80) != 0;
        break;
    case 0x18:  /* RCR eb,1 */
        *dest = (tmp >> 1) + (CF << 7);
        OF = !(!(tmp & 0x80)) != CF;
        CF = (tmp & 0x01) != 0;
        break;
    case 0x20:  /* SHL eb,1 */
    case 0x30:
        tmp += tmp;
        
        SetCFB_Add(tmp,tmp2);
        SetOFB_Add(tmp,tmp2,tmp2);
        AF = 1;
        SetZFB(tmp);
        SetSFB(tmp);
        SetPF(tmp);
        
        *dest = (BYTE)tmp;
        break;
    case 0x28:  /* SHR eb,1 */
        CF = (tmp & 0x01) != 0;
        OF = tmp & 0x80;
        
        tmp2 = tmp >> 1;
        
        SetSFB(tmp2);
        SetPF(tmp2);
        SetZFB(tmp2);
        AF = 1;
        *dest = (BYTE)tmp2;
        break;
    case 0x38:  /* SAR eb,1 */
        CF = (tmp & 0x01) != 0;
        OF = 0;
        
        tmp2 = (tmp >> 1) | (tmp & 0x80);
        
        SetSFB(tmp2);
        SetPF(tmp2);
        SetZFB(tmp2);
        AF = 1;
        *dest = (BYTE)tmp2;
        break;
    }
}


static INLINE2 void i_d1pre(void)
{
    /* Opcode 0xd1 */

    unsigned ModRM = GetMemInc(c_cs,ip);
    register WORD *dest = GetModRMRMW(ModRM);
    register unsigned tmp = ReadWord(dest);
    register unsigned tmp2 = tmp;

    switch (ModRM & 0x38)
    {
    case 0x00:  /* ROL ew,1 */
        CF = (tmp & 0x8000) != 0;
        tmp2 = (tmp << 1) + CF;
        OF = !(!(tmp & 0x4000)) != CF;
        WriteWord(dest,tmp2);
        break;
    case 0x08:  /* ROR ew,1 */
        CF = (tmp & 0x01) != 0;
        tmp2 = (tmp >> 1) + ((unsigned)CF << 15);
        OF = !(!(tmp & 0x8000)) != CF;
        WriteWord(dest,tmp2);
        break;
    case 0x10:  /* RCL ew,1 */
        tmp2 = (tmp << 1) + CF;
        OF = (tmp ^ (tmp << 1)) & 0x8000;
        CF = (tmp & 0x8000) != 0;
        WriteWord(dest,tmp2);
        break;
    case 0x18:  /* RCR ew,1 */
        tmp2 = (tmp >> 1) + ((unsigned)CF << 15);
        OF = !(!(tmp & 0x8000)) != CF;
        CF = (tmp & 0x01) != 0;
        WriteWord(dest,tmp2);
        break;
    case 0x20:  /* SHL ew,1 */
    case 0x30:
        tmp += tmp;
        
        SetCFW_Add(tmp,tmp2);
        SetOFW_Add(tmp,tmp2,tmp2);
        AF = 1;
        SetZFW(tmp);
        SetSFW(tmp);
        SetPF(tmp);
        
        WriteWord(dest,tmp);
        break;
    case 0x28:  /* SHR ew,1 */
        CF = (tmp & 0x01) != 0;
        OF = tmp & 0x8000;
        
        tmp2 = tmp >> 1;
        
        SetSFW(tmp2);
        SetPF(tmp2);
        SetZFW(tmp2);
        AF = 1;
        WriteWord(dest,tmp2);
        break;
    case 0x38:  /* SAR ew,1 */
        CF = (tmp & 0x01) != 0;
        OF = 0;
        
        tmp2 = (tmp >> 1) | (tmp & 0x8000);
        
        SetSFW(tmp2);
        SetPF(tmp2);
        SetZFW(tmp2);
        AF = 1;
        WriteWord(dest,tmp2);
        break;
    }
}


static INLINE2 void i_d2pre(void)
{
    /* Opcode 0xd2 */

    unsigned ModRM;
    register BYTE *dest;
    register unsigned tmp;
    register unsigned tmp2;
    unsigned count;

    if (*bregs[CL] == 1)
    {
        i_d0pre();
        D(printf("Skipping CL processing\n"););
        return;
    }

    ModRM = GetMemInc(c_cs,ip);
    dest = GetModRMRMB(ModRM);
    tmp = (unsigned)*dest;
    count = (unsigned)*bregs[CL];

    switch (ModRM & 0x38)
    {
    case 0x00:  /* ROL eb,CL */
        for (; count > 0; count--)
        {
            CF = (tmp & 0x80) != 0;
            tmp = (tmp << 1) + CF;
        }
        *dest = (BYTE)tmp;
        break;
    case 0x08:  /* ROR eb,CL */
        for (; count > 0; count--)
        {
            CF = (tmp & 0x01) != 0;
            tmp = (tmp >> 1) + (CF << 7);
        }
        *dest = (BYTE)tmp;
        break;
    case 0x10:  /* RCL eb,CL */
        for (; count > 0; count--)
        {
            tmp2 = CF;
            CF = (tmp & 0x80) != 0;
            tmp = (tmp << 1) + tmp2;
        }
        *dest = (BYTE)tmp;
        break;
    case 0x18:  /* RCR eb,CL */
        for (; count > 0; count--)
        {
            tmp2 = (tmp >> 1) + (CF << 7);
            CF = (tmp & 0x01) != 0;
            tmp = tmp2;
        }
        *dest = (BYTE)tmp;
        break;
    case 0x20:
    case 0x30:  /* SHL eb,CL */
        if (count >= 9)
        {
            CF = 0;             /* Not sure about this... */
            tmp = 0;
        }
        else
        {
            CF = ((tmp << (count-1)) & 0x80) != 0;
            tmp <<= count;
        }
        
        AF = 1;
        SetZFB(tmp);
        SetSFB(tmp);
        SetPF(tmp);
        
        *dest = (BYTE)tmp;
        break;
    case 0x28:  /* SHR eb,CL */
        if (count >= 9)
        {
            CF = 0;             /* Not sure about this... */
            tmp = 0;
        }
        else
        {
            CF = ((tmp >> (count-1)) & 0x1) != 0;
            tmp >>= count;
        }
        
        SetSFB(tmp);
        SetPF(tmp);
        SetZFB(tmp);
        AF = 1;
        *dest = (BYTE)tmp;
        break;
    case 0x38:  /* SAR eb,CL */
        tmp2 = tmp & 0x80;
        CF = (((INT8)tmp >> (count-1)) & 0x01) != 0;        
        for (; count > 0; count--)
            tmp = (tmp >> 1) | tmp2;
        
        SetSFB(tmp);
        SetPF(tmp);
        SetZFB(tmp);
        AF = 1;
        *dest = (BYTE)tmp;
        break;
    }
}


static INLINE2 void i_d3pre(void)
{
    /* Opcode 0xd3 */

    unsigned ModRM;
    register WORD *dest;
    register unsigned tmp;
    register unsigned tmp2;
    unsigned count;

    if (*bregs[CL] == 1)
    {
        i_d1pre();
        return;
    }

    ModRM = GetMemInc(c_cs,ip);
    dest = GetModRMRMW(ModRM);
    tmp = ReadWord(dest);
    count = (unsigned)*bregs[CL];

    switch (ModRM & 0x38)
    {
    case 0x00:  /* ROL ew,CL */
        for (; count > 0; count--)
        {
            CF = (tmp & 0x8000) != 0;
            tmp = (tmp << 1) + CF;
        }
        WriteWord(dest,tmp);
        break;
    case 0x08:  /* ROR ew,CL */
        for (; count > 0; count--)
        {
            CF = (tmp & 0x01) != 0;
            tmp = (tmp >> 1) + (CF << 15);
        }
        WriteWord(dest, tmp);
        break;
    case 0x10:  /* RCL ew,CL */
        for (; count > 0; count--)
        {
            tmp2 = CF;
            CF = (tmp & 0x8000) != 0;
            tmp = (tmp << 1) + tmp2;
        }
        WriteWord(dest, tmp);
        break;
    case 0x18:  /* RCR ew,CL */
        for (; count > 0; count--)
        {
            tmp2 = (tmp >> 1) + (CF << 15);
            CF = (tmp & 0x01) != 0;
            tmp = tmp2;
        }
        WriteWord(dest, tmp);
        break;
    case 0x20:
    case 0x30:  /* SHL ew,CL */
        if (count >= 17)
        {
            CF = 0;             /* Not sure about this... */
            tmp = 0;
        }
        else
        {
            CF = ((tmp << (count-1)) & 0x8000) != 0;
            tmp <<= count;
        }
        
        AF = 1;
        SetZFW(tmp);
        SetSFW(tmp);
        SetPF(tmp);
        
        WriteWord(dest, tmp);
        break;
    case 0x28:  /* SHR ew,CL */
        if (count >= 17)
        {
            CF = 0;             /* Not sure about this... */
            tmp = 0;
        }
        else
        {
            CF = ((tmp >> (count-1)) & 0x1) != 0;
            tmp >>= count;
        }
        
        SetSFW(tmp);
        SetPF(tmp);
        SetZFW(tmp);
        AF = 1;
        WriteWord(dest, tmp);
        break;
    case 0x38:  /* SAR ew,CL */
        tmp2 = tmp & 0x8000;
        CF = (((INT16)tmp >> (count-1)) & 0x01) != 0;        
        for (; count > 0; count--)
            tmp = (tmp >> 1) | tmp2;
        
        SetSFW(tmp);
        SetPF(tmp);
        SetZFW(tmp);
        AF = 1;
        WriteWord(dest, tmp);
        break;
    }
}

static INLINE2 void i_aam(void)
{
    /* Opcode 0xd4 */
    unsigned mult = GetMemInc(c_cs,ip);

	if (mult == 0)
        interrupt(0);
    else
    {
        *bregs[AH] = *bregs[AL] / mult;
        *bregs[AL] %= mult;

        SetPF(*bregs[AL]);
        SetZFW(wregs[AX]);
        SetSFB(*bregs[AH]);
    }
}


static INLINE2 void i_aad(void)
{
    /* Opcode 0xd5 */
    unsigned mult = GetMemInc(c_cs,ip);

    *bregs[AL] = *bregs[AH] * mult + *bregs[AL];
    *bregs[AH] = 0;

    SetPF(*bregs[AL]);
    SetZFB(*bregs[AL]);
    SF = 0;
}

static INLINE2 void i_xlat(void)
{
    /* Opcode 0xd7 */

    unsigned dest = ReadWord(&wregs[BX])+*bregs[AL];
    
    *bregs[AL] = GetMemB(c_ds, dest);
}

static INLINE2 void i_escape(void)
{
    /* Opcodes 0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde and 0xdf */

    unsigned ModRM = GetMemInc(c_cs,ip);
    GetModRMRMB(ModRM);
}

static INLINE2 void i_loopne(void)
{
    /* Opcode 0xe0 */

    register int disp = (int)((INT8)GetMemInc(c_cs,ip));
    register unsigned tmp = ReadWord(&wregs[CX])-1;

    WriteWord(&wregs[CX],tmp);

    if (!ZF && tmp) ip = (WORD)(ip+disp);
}

static INLINE2 void i_loope(void)
{
    /* Opcode 0xe1 */

    register int disp = (int)((INT8)GetMemInc(c_cs,ip));
    register unsigned tmp = ReadWord(&wregs[CX])-1;

    WriteWord(&wregs[CX],tmp);

    if (ZF && tmp) ip = (WORD)(ip+disp);
}

static INLINE2 void i_loop(void)
{
    /* Opcode 0xe2 */

    register int disp = (int)((INT8)GetMemInc(c_cs,ip));
    register unsigned tmp = ReadWord(&wregs[CX])-1;

    WriteWord(&wregs[CX],tmp);

    if (tmp) ip = (WORD)(ip+disp);
}

static INLINE2 void i_jcxz(void)
{
    /* Opcode 0xe3 */

    register int disp = (int)((INT8)GetMemInc(c_cs,ip));
    
    if (wregs[CX] == 0)
        ip = (WORD)(ip+disp);
}

static INLINE2 void i_inal(void)
{
    /* Opcode 0xe4 */

    unsigned port = GetMemInc(c_cs,ip);

    *bregs[AL] = read_port(port);
}

static INLINE2 void i_inax(void)
{
    /* Opcode 0xe5 */

    unsigned port = GetMemInc(c_cs,ip);

    *bregs[AL] = read_port(port);
    *bregs[AH] = read_port(port+1);
}
    
static INLINE2 void i_outal(void)
{
    /* Opcode 0xe6 */

    unsigned port = GetMemInc(c_cs,ip);

    write_port(port, *bregs[AL]);
}

static INLINE2 void i_outax(void)
{
    /* Opcode 0xe7 */

    unsigned port = GetMemInc(c_cs,ip);

    write_port(port, *bregs[AL]);
    write_port(port+1, *bregs[AH]);
}

static INLINE2 void i_call_d16(void)
{
    /* Opcode 0xe8 */

    register unsigned tmp;
    register unsigned tmp1 = (WORD)(ReadWord(&wregs[SP])-2);

    tmp = GetMemInc(c_cs,ip);
    tmp += GetMemInc(c_cs,ip) << 8;

    PutMemW(c_stack,tmp1,ip);
    WriteWord(&wregs[SP],tmp1);

    ip = (WORD)(ip+(INT16)tmp);
}


static INLINE2 void i_jmp_d16(void)
{
    /* Opcode 0xe9 */

    register int tmp = GetMemInc(c_cs,ip);
    tmp += GetMemInc(c_cs,ip) << 8;

    ip = (WORD)(ip+(INT16)tmp);
}


static INLINE2 void i_jmp_far(void)
{
    /* Opcode 0xea */

    register unsigned tmp,tmp1;

    tmp = GetMemInc(c_cs,ip);
    tmp += GetMemInc(c_cs,ip) << 8;

    tmp1 = GetMemInc(c_cs,ip);
    tmp1 += GetMemInc(c_cs,ip) << 8;

    sregs[CS] = (WORD)tmp1;
    c_cs = SegToMemPtr(CS);
    ip = (WORD)tmp;
}


static INLINE2 void i_jmp_d8(void)
{
    /* Opcode 0xeb */
    register int tmp = (int)((INT8)GetMemInc(c_cs,ip));
    ip = (WORD)(ip+tmp);
}


static INLINE2 void i_inaldx(void)
{
    /* Opcode 0xec */

    *bregs[AL] = read_port(ReadWord(&wregs[DX]));
}

static INLINE2 void i_inaxdx(void)
{
    /* Opcode 0xed */

    unsigned port = ReadWord(&wregs[DX]);

    *bregs[AL] = read_port(port);
    *bregs[AH] = read_port(port+1);
}

static INLINE2 void i_outdxal(void)
{
    /* Opcode 0xee */

    write_port(ReadWord(&wregs[DX]), *bregs[AL]);
}

static INLINE2 void i_outdxax(void)
{
    /* Opcode 0xef */
    unsigned port = ReadWord(&wregs[DX]);

    write_port(port, *bregs[AL]);
    write_port(port+1, *bregs[AH]);
}

static INLINE2 void i_lock(void)
{
    /* Opcode 0xf0 */
}

static INLINE2 void i_gobios(void)
{
    /* Opcode 0xf1 */
    unsigned intno;
    
    if (GetMemInc(c_cs,ip) != 0xf1)
        i_notdone();

    intno = GetMemInc(c_cs,ip);
    ip += 3; /* Skip next three bytes */

    bios_routine[intno]();
}


static void rep(int flagval)
{
    /* Handles rep- and repnz- prefixes. flagval is the value of ZF for the
       loop  to continue for CMPS and SCAS instructions. */

    unsigned next = GetMemInc(c_cs,ip);
    unsigned count = ReadWord(&wregs[CX]);

    switch(next)
    {
    case 0x26:  /* ES: */
        c_ss = c_ds = c_es;
        rep(flagval);
        c_ds = SegToMemPtr(DS);
        c_ss = SegToMemPtr(SS);
        break;
    case 0x2e:  /* CS: */
        c_ss = c_ds = c_cs;
        rep(flagval);
        c_ds = SegToMemPtr(DS);
        c_ss = SegToMemPtr(SS);
        break;
    case 0x36:  /* SS: */
        c_ds = c_ss;
        rep(flagval);
        c_ds = SegToMemPtr(DS);
        break;
    case 0x3e:  /* DS: */
        c_ss = c_ds;
        rep(flagval);
        c_ss = SegToMemPtr(SS);
        break;
    case 0xa4:  /* REP MOVSB */
        for (; count > 0; count--)
            i_movsb();
        WriteWord(&wregs[CX],count);
        break;
    case 0xa5:  /* REP MOVSW */
        for (; count > 0; count--)
            i_movsw();
        WriteWord(&wregs[CX],count);
        break;
    case 0xa6:  /* REP(N)E CMPSB */
        for (ZF = flagval; (ZF == flagval) && (count > 0); count--)
            i_cmpsb();
        WriteWord(&wregs[CX],count);
        break;
    case 0xa7:  /* REP(N)E CMPSW */
        for (ZF = flagval; (ZF == flagval) && (count > 0); count--)
            i_cmpsw();
        WriteWord(&wregs[CX],count);
        break;
    case 0xaa:  /* REP STOSB */
        for (; count > 0; count--)
            i_stosb();
        WriteWord(&wregs[CX],count);
        break;
    case 0xab:  /* REP LODSW */
        for (; count > 0; count--)
            i_stosw();
        WriteWord(&wregs[CX],count);
        break;
    case 0xac:  /* REP LODSB */
        for (; count > 0; count--)
            i_lodsb();
        WriteWord(&wregs[CX],count);
        break;
    case 0xad:  /* REP LODSW */
        for (; count > 0; count--)
            i_lodsw();
        WriteWord(&wregs[CX],count);
        break;
    case 0xae:  /* REP(N)E SCASB */
        for (ZF = flagval; (ZF == flagval) && (count > 0); count--)
            i_scasb();
        WriteWord(&wregs[CX],count);
        break;
    case 0xaf:  /* REP(N)E SCASW */
        for (ZF = flagval; (ZF == flagval) && (count > 0); count--)
            i_scasw();
        WriteWord(&wregs[CX],count);
        break;
    default:
        instruction[next]();
    }
}
            

static INLINE2 void i_repne(void)
{
    /* Opcode 0xf2 */

    rep(0);
}


static INLINE2 void i_repe(void)
{
    /* Opcode 0xf3 */

    rep(1);
}


static INLINE2 void i_cmc(void)
{
    /* Opcode 0xf5 */

    CF = !CF;
}


static INLINE2 void i_f6pre(void)
{
	/* Opcode 0xf6 */
    unsigned ModRM = GetMemInc(c_cs,ip);
    register BYTE *byte = GetModRMRMB(ModRM);
    register unsigned tmp = (unsigned)*byte;
    register unsigned tmp2;
    
    
    switch (ModRM & 0x38)
    {
    case 0x00:	/* TEST Eb, data8 */
    case 0x08:  /* ??? */
        tmp &= GetMemInc(c_cs,ip);
        
        CF = OF = AF = 0;
        SetZFB(tmp);
        SetSFB(tmp);
        SetPF(tmp);
        break;
        
    case 0x10:	/* NOT Eb */
        *byte = ~tmp;
        break;
        
    case 0x18:	/* NEG Eb */
        tmp2 = tmp;
        tmp = -tmp;
        
        CF = (int)tmp2 > 0;
        
        SetAF(tmp,0,tmp2);
        SetZFB(tmp);
        SetSFB(tmp);
        SetPF(tmp);
        
        *byte = tmp;
        break;
    case 0x20:	/* MUL AL, Eb */
	{
	    UINT16 result;
	    tmp2 = *bregs[AL];
        
	    SetSFB(tmp2);
	    SetPF(tmp2);
        
	    result = (UINT16)tmp2*tmp;
	    WriteWord(&wregs[AX],(WORD)result);

	    SetZFW(wregs[AX]);
	    CF = OF = (*bregs[AH] != 0);
	}
        break;
    case 0x28:	/* IMUL AL, Eb */
	{
	    INT16 result;
        
	    tmp2 = (unsigned)*bregs[AL];
        
	    SetSFB(tmp2);
	    SetPF(tmp2);
        
	    result = (INT16)((INT8)tmp2)*(INT16)((INT8)tmp);
	    WriteWord(&wregs[AX],(WORD)result);
        
	    SetZFW(wregs[AX]);
        
	    OF = (result >> 7 != 0) && (result >> 7 != -1);
	    CF = !(!OF);
	}
        break;
    case 0x30:	/* DIV AL, Ew */
	{
	    UINT16 result;
        
	    result = ReadWord(&wregs[AX]);
        
	    if (tmp)
	    {
            if ((result / tmp) > 0xff)
            {
                interrupt(0);
                break;
            }
            else
            {
                *bregs[AH] = result % tmp;
                *bregs[AL] = result / tmp;
            }
            
	    }
	    else
	    {
            interrupt(0);
            break;
	    }
	}
        break;
    case 0x38:	/* IDIV AL, Ew */
	{
	    INT16 result;
        
	    result = ReadWord(&wregs[AX]);
        
	    if (tmp)
	    {
            tmp2 = result % (INT16)((INT8)tmp);
            
            if ((result /= (INT16)((INT8)tmp)) > 0xff)
            {
                interrupt(0);
                break;
            }
            else
            {
                *bregs[AL] = result;
                *bregs[AH] = tmp2;
            }
	    }
	    else
	    {
            interrupt(0);
            break;
	    }
	}
        break;
    }
}


static INLINE2 void i_f7pre(void)
{
	/* Opcode 0xf7 */
    unsigned ModRM = GetMemInc(c_cs,ip);
    WORD *wrd = GetModRMRMW(ModRM);
    register unsigned tmp = ReadWord(wrd);
    register unsigned tmp2;
    
    
    switch (ModRM & 0x38)
    {
    case 0x00:	/* TEST Ew, data16 */
    case 0x08:  /* ??? */
        tmp2 = GetMemInc(c_cs,ip);
        tmp2 += GetMemInc(c_cs,ip) << 8;
        
        tmp &= tmp2;
        
        CF = OF = AF = 0;
        SetZFW(tmp);
        SetSFW(tmp);
        SetPF(tmp);
        break;
        
    case 0x10:	/* NOT Ew */
        tmp = ~tmp;
        WriteWord(wrd,tmp);
        break;
        
    case 0x18:	/* NEG Ew */
        tmp2 = tmp;
        tmp = -tmp;
        
        CF = (int)tmp2 > 0;
        
        SetAF(tmp,0,tmp2);
        SetZFW(tmp);
        SetSFW(tmp);
        SetPF(tmp);
        
        WriteWord(wrd,tmp);
        break;
    case 0x20:	/* MUL AX, Ew */
	{
	    UINT32 result;
	    tmp2 = ReadWord(&wregs[AX]);
        
	    SetSFW(tmp2);
	    SetPF(tmp2);

	    result = (UINT32)tmp2*tmp;
	    WriteWord(&wregs[AX],(WORD)result);
        result >>= 16;
	    WriteWord(&wregs[DX],result);

	    SetZFW(wregs[AX] | wregs[DX]);
	    CF = OF = (wregs[DX] != ChangeE(0));
	}
        break;
        
    case 0x28:	/* IMUL AX, Ew */
	{
	    INT32 result;
        
	    tmp2 = ReadWord(&wregs[AX]);
        
	    SetSFW(tmp2);
	    SetPF(tmp2);
        
	    result = (INT32)((INT16)tmp2)*(INT32)((INT16)tmp);
        OF = (result >> 15 != 0) && (result >> 15 != -1);

	    WriteWord(&wregs[AX],(WORD)result);
        result = (WORD)(result >> 16);
	    WriteWord(&wregs[DX],result);

	    SetZFW(wregs[AX] | wregs[DX]);
        
        CF = !(!OF);
	}
        break;
    case 0x30:	/* DIV AX, Ew */
	{
	    UINT32 result;
        
	    result = (ReadWord(&wregs[DX]) << 16) + ReadWord(&wregs[AX]);
        
	    if (tmp)
	    {
            tmp2 = result % tmp;
            if ((result / tmp) > 0xffff)
            {
                interrupt(0);
                break;
            }
            else
            {
                WriteWord(&wregs[DX],tmp2);
                result /= tmp;
                WriteWord(&wregs[AX],result);
            }
	    }
	    else
	    {
            interrupt(0);
            break;
	    }
	}
        break;
    case 0x38:	/* IDIV AX, Ew */
	{
	    INT32 result;
        
	    result = (ReadWord(&wregs[DX]) << 16) + ReadWord(&wregs[AX]);
        
	    if (tmp)
	    {
            tmp2 = result % (INT32)((INT16)tmp);
            
            if ((result /= (INT32)((INT16)tmp)) > 0xffff)
            {
                interrupt(0);
                break;
            }
            else
            {
                WriteWord(&wregs[AX],result);
                WriteWord(&wregs[DX],tmp2);
            }
	    }
	    else
	    {
            interrupt(0);
            break;
	    }
	}
        break;
    }
}


static INLINE2 void i_clc(void)
{
    /* Opcode 0xf8 */

    CF = 0;
}


static INLINE2 void i_stc(void)
{
	/* Opcode 0xf9 */

	CF = 1;
}


static INLINE2 void i_cli(void)
{
    /* Opcode 0xfa */

    IF = 0;
}


static INLINE2 void i_sti(void)
{
    /* Opcode 0xfb */

    IF = 1;

    if (int_blocked)
    {
        int_pending = int_blocked;
        int_blocked = 0;
        D2(printf("Unblocking interrupt\n"););
    }
}


static INLINE2 void i_cld(void)
{
    /* Opcode 0xfc */

    DF = 0;
}


static INLINE2 void i_std(void)
{
    /* Opcode 0xfd */

    DF = 1;
}


static INLINE2 void i_fepre(void)
{
    /* Opcode 0xfe */

    unsigned ModRM = GetMemInc(c_cs,ip);
    register BYTE *dest = GetModRMRMB(ModRM);
    register unsigned tmp = *dest;
    register unsigned tmp1;
    
    if ((ModRM & 0x38) == 0)
    {
        tmp1 = tmp+1;
        SetOFB_Add(tmp1,tmp,1);
    }
    else
    {
        tmp1 = tmp-1;
        SetOFB_Sub(tmp1,1,tmp);
    }
    
    SetAF(tmp1,tmp,1);
    SetZFB(tmp1);
    SetSFB(tmp1);
    SetPF(tmp1);
    
    *dest = (BYTE)tmp1;
}


static INLINE2 void i_ffpre(void)
{
    /* Opcode 0xff */

    unsigned ModRM = GetMemInc(c_cs,ip);
    register WORD *dest = GetModRMRMW(ModRM);
    register unsigned tmp;
    register unsigned tmp1;

    switch(ModRM & 0x38)
    {
    case 0x00:  /* INC ew */
        tmp = ReadWord(dest);
        tmp1 = tmp+1;
        
        SetOFW_Add(tmp1,tmp,1);
        SetAF(tmp1,tmp,1);
        SetZFW(tmp1);
        SetSFW(tmp1);
        SetPF(tmp1);
        
        WriteWord(dest,(WORD)tmp1);
        break;
        
    case 0x08:  /* DEC ew */
        tmp = ReadWord(dest);
        tmp1 = tmp-1;
        
        SetOFW_Sub(tmp1,1,tmp);
        SetAF(tmp1,tmp,1);
        SetZFW(tmp1);
        SetSFW(tmp1);
        SetPF(tmp1);
        
        WriteWord(dest,(WORD)tmp1);
        break;
        
    case 0x10:  /* CALL ew */
        tmp = ReadWord(dest);
        tmp1 = (WORD)(ReadWord(&wregs[SP])-2);
        
        PutMemW(c_stack,tmp1,ip);
        WriteWord(&wregs[SP],tmp1);
        
        ip = (WORD)tmp;
        break;
        
    case 0x18:  /* CALL FAR ea */
        tmp1 = (WORD)(ReadWord(&wregs[SP])-2);
        
        PutMemW(c_stack,tmp1,sregs[CS]);
        tmp1 = (WORD)(tmp1-2);
        PutMemW(c_stack,tmp1,ip);
        WriteWord(&wregs[SP],tmp1);
        
        ip = ReadWord(dest);
        dest++;
        sregs[CS] = ReadWord(dest);
        c_cs = SegToMemPtr(CS);
        break;
        
    case 0x20:  /* JMP ea */
        ip = ReadWord(dest);
        break;
        
    case 0x28:  /* JMP FAR ea */
        ip = ReadWord(dest);
        dest++;
        sregs[CS] = ReadWord(dest);
        c_cs = SegToMemPtr(CS);
        break;
        
    case 0x30:  /* PUSH ea */
        tmp1 = (WORD)(ReadWord(&wregs[SP])-2);
        tmp = ReadWord(dest);
        
        PutMemW(c_stack,tmp1,tmp);
        WriteWord(&wregs[SP],tmp1);
        break;
    }
}


static INLINE2 void i_notdone(void)
{
    fprintf(stderr,"Error: Unimplemented opcode %02X at cs:ip = %04X:%04X\n",
		    c_cs[ip-1],sregs[CS],ip-1);
    exit(1);
}


void trap(void)
{
    instruction[GetMemInc(c_cs,ip)]();
    interrupt(1);
}


void execute(void)
{
    c_cs = SegToMemPtr(CS);
    c_ds = SegToMemPtr(DS);
    c_es = SegToMemPtr(ES);
    c_stack = c_ss = SegToMemPtr(SS);
    
    for(;;)
    {
        if (int_pending)
            external_int();

#ifdef DEBUGGER
        call_debugger(D_TRACE);
#endif

#if defined(BIGCASE) && !defined(RS6000)
  /* Some compilers cannot handle large case statements */
        switch(GetMemInc(c_cs,ip))
        {
        case 0x00:    i_add_br8(); break;
        case 0x01:    i_add_wr16(); break;
        case 0x02:    i_add_r8b(); break;
        case 0x03:    i_add_r16w(); break;
        case 0x04:    i_add_ald8(); break;
        case 0x05:    i_add_axd16(); break;
        case 0x06:    i_push_es(); break;
        case 0x07:    i_pop_es(); break;
        case 0x08:    i_or_br8(); break;
        case 0x09:    i_or_wr16(); break;
        case 0x0a:    i_or_r8b(); break;
        case 0x0b:    i_or_r16w(); break;
        case 0x0c:    i_or_ald8(); break;
        case 0x0d:    i_or_axd16(); break;
        case 0x0e:    i_push_cs(); break;
        case 0x0f:    i_notdone(); break;
        case 0x10:    i_adc_br8(); break;
        case 0x11:    i_adc_wr16(); break;
        case 0x12:    i_adc_r8b(); break;
        case 0x13:    i_adc_r16w(); break;
        case 0x14:    i_adc_ald8(); break;
        case 0x15:    i_adc_axd16(); break;
        case 0x16:    i_push_ss(); break;
        case 0x17:    i_pop_ss(); break;
        case 0x18:    i_sbb_br8(); break;
        case 0x19:    i_sbb_wr16(); break;
        case 0x1a:    i_sbb_r8b(); break;
        case 0x1b:    i_sbb_r16w(); break;
        case 0x1c:    i_sbb_ald8(); break;
        case 0x1d:    i_sbb_axd16(); break;
        case 0x1e:    i_push_ds(); break;
        case 0x1f:    i_pop_ds(); break;
        case 0x20:    i_and_br8(); break;
        case 0x21:    i_and_wr16(); break;
        case 0x22:    i_and_r8b(); break;
        case 0x23:    i_and_r16w(); break;
        case 0x24:    i_and_ald8(); break;
        case 0x25:    i_and_axd16(); break;
        case 0x26:    i_es(); break;
        case 0x27:    i_daa(); break;
        case 0x28:    i_sub_br8(); break;
        case 0x29:    i_sub_wr16(); break;
        case 0x2a:    i_sub_r8b(); break;
        case 0x2b:    i_sub_r16w(); break;
        case 0x2c:    i_sub_ald8(); break;
        case 0x2d:    i_sub_axd16(); break;
        case 0x2e:    i_cs(); break;
        case 0x2f:    i_notdone(); break;
        case 0x30:    i_xor_br8(); break;
        case 0x31:    i_xor_wr16(); break;
        case 0x32:    i_xor_r8b(); break;
        case 0x33:    i_xor_r16w(); break;
        case 0x34:    i_xor_ald8(); break;
        case 0x35:    i_xor_axd16(); break;
        case 0x36:    i_ss(); break;
        case 0x37:    i_aaa(); break;
        case 0x38:    i_cmp_br8(); break;
        case 0x39:    i_cmp_wr16(); break;
        case 0x3a:    i_cmp_r8b(); break;
        case 0x3b:    i_cmp_r16w(); break;
        case 0x3c:    i_cmp_ald8(); break;
        case 0x3d:    i_cmp_axd16(); break;
        case 0x3e:    i_ds(); break;
        case 0x3f:    i_aas(); break;
        case 0x40:    i_inc_ax(); break;
        case 0x41:    i_inc_cx(); break;
        case 0x42:    i_inc_dx(); break;
        case 0x43:    i_inc_bx(); break;
        case 0x44:    i_inc_sp(); break;
        case 0x45:    i_inc_bp(); break;
        case 0x46:    i_inc_si(); break;
        case 0x47:    i_inc_di(); break;
        case 0x48:    i_dec_ax(); break;
        case 0x49:    i_dec_cx(); break;
        case 0x4a:    i_dec_dx(); break;
        case 0x4b:    i_dec_bx(); break;
        case 0x4c:    i_dec_sp(); break;
        case 0x4d:    i_dec_bp(); break;
        case 0x4e:    i_dec_si(); break;
        case 0x4f:    i_dec_di(); break;
        case 0x50:    i_push_ax(); break;
        case 0x51:    i_push_cx(); break;
        case 0x52:    i_push_dx(); break;
        case 0x53:    i_push_bx(); break;
        case 0x54:    i_push_sp(); break;
        case 0x55:    i_push_bp(); break;
        case 0x56:    i_push_si(); break;
        case 0x57:    i_push_di(); break;
        case 0x58:    i_pop_ax(); break;
        case 0x59:    i_pop_cx(); break;
        case 0x5a:    i_pop_dx(); break;
        case 0x5b:    i_pop_bx(); break;
        case 0x5c:    i_pop_sp(); break;
        case 0x5d:    i_pop_bp(); break;
        case 0x5e:    i_pop_si(); break;
        case 0x5f:    i_pop_di(); break;
        case 0x60:    i_notdone(); break;
        case 0x61:    i_notdone(); break;
        case 0x62:    i_notdone(); break;
        case 0x63:    i_notdone(); break;
        case 0x64:    i_notdone(); break;
        case 0x65:    i_notdone(); break;
        case 0x66:    i_notdone(); break;
        case 0x67:    i_notdone(); break;
	case 0x68:    i_push_immed16(); break;	/* 186 and above */
        case 0x69:    i_imul_r16_immed16(); break; /* 186 and above */
        case 0x6a:    i_notdone(); break;
        case 0x6b:    i_notdone(); break;
        case 0x6c:    i_notdone(); break;
        case 0x6d:    i_notdone(); break;
        case 0x6e:    i_outs_dx_rm8(); break;	/* 186 and above */
        case 0x6f:    i_outs_dx_rm16(); break;	/* 186 and above */
        case 0x70:    i_jo(); break;
        case 0x71:    i_jno(); break;
        case 0x72:    i_jb(); break;
        case 0x73:    i_jnb(); break;
        case 0x74:    i_jz(); break;
        case 0x75:    i_jnz(); break;
        case 0x76:    i_jbe(); break;
        case 0x77:    i_jnbe(); break;
        case 0x78:    i_js(); break;
        case 0x79:    i_jns(); break;
        case 0x7a:    i_jp(); break;
        case 0x7b:    i_jnp(); break;
        case 0x7c:    i_jl(); break;
        case 0x7d:    i_jnl(); break;
        case 0x7e:    i_jle(); break;
        case 0x7f:    i_jnle(); break;
        case 0x80:    i_80pre(); break;
        case 0x81:    i_81pre(); break;
        case 0x82:    i_notdone(); break;
        case 0x83:    i_83pre(); break;
        case 0x84:    i_test_br8(); break;
        case 0x85:    i_test_wr16(); break;
        case 0x86:    i_xchg_br8(); break;
        case 0x87:    i_xchg_wr16(); break;
        case 0x88:    i_mov_br8(); break;
        case 0x89:    i_mov_wr16(); break;
        case 0x8a:    i_mov_r8b(); break;
        case 0x8b:    i_mov_r16w(); break;
        case 0x8c:    i_mov_wsreg(); break;
        case 0x8d:    i_lea(); break;
        case 0x8e:    i_mov_sregw(); break;
        case 0x8f:    i_popw(); break;
        case 0x90:    i_nop(); break;
        case 0x91:    i_xchg_axcx(); break;
        case 0x92:    i_xchg_axdx(); break;
        case 0x93:    i_xchg_axbx(); break;
        case 0x94:    i_xchg_axsp(); break;
        case 0x95:    i_xchg_axbp(); break;
        case 0x96:    i_xchg_axsi(); break;
        case 0x97:    i_xchg_axdi(); break;
        case 0x98:    i_cbw(); break;
        case 0x99:    i_cwd(); break;
        case 0x9a:    i_call_far(); break;
        case 0x9b:    i_wait(); break;
        case 0x9c:    i_pushf(); break;
        case 0x9d:    i_popf(); break;
        case 0x9e:    i_sahf(); break;
        case 0x9f:    i_lahf(); break;
        case 0xa0:    i_mov_aldisp(); break;
        case 0xa1:    i_mov_axdisp(); break;
        case 0xa2:    i_mov_dispal(); break;
        case 0xa3:    i_mov_dispax(); break;
        case 0xa4:    i_movsb(); break;
        case 0xa5:    i_movsw(); break;
        case 0xa6:    i_cmpsb(); break;
        case 0xa7:    i_cmpsw(); break;
        case 0xa8:    i_test_ald8(); break;
        case 0xa9:    i_test_axd16(); break;
        case 0xaa:    i_stosb(); break;
        case 0xab:    i_stosw(); break;
        case 0xac:    i_lodsb(); break;
        case 0xad:    i_lodsw(); break;
        case 0xae:    i_scasb(); break;
        case 0xaf:    i_scasw(); break;
        case 0xb0:    i_mov_ald8(); break;
        case 0xb1:    i_mov_cld8(); break;
        case 0xb2:    i_mov_dld8(); break;
        case 0xb3:    i_mov_bld8(); break;
        case 0xb4:    i_mov_ahd8(); break;
        case 0xb5:    i_mov_chd8(); break;
        case 0xb6:    i_mov_dhd8(); break;
        case 0xb7:    i_mov_bhd8(); break;
        case 0xb8:    i_mov_axd16(); break;
        case 0xb9:    i_mov_cxd16(); break;
        case 0xba:    i_mov_dxd16(); break;
        case 0xbb:    i_mov_bxd16(); break;
        case 0xbc:    i_mov_spd16(); break;
        case 0xbd:    i_mov_bpd16(); break;
        case 0xbe:    i_mov_sid16(); break;
        case 0xbf:    i_mov_did16(); break;
        case 0xc0:    i_notdone(); break;
        case 0xc1:    i_notdone(); break;
        case 0xc2:    i_ret_d16(); break;
        case 0xc3:    i_ret(); break;
        case 0xc4:    i_les_dw(); break;
        case 0xc5:    i_lds_dw(); break;
        case 0xc6:    i_mov_bd8(); break;
        case 0xc7:    i_mov_wd16(); break;
        case 0xc8:    i_notdone(); break;
        case 0xc9:    i_notdone(); break;
        case 0xca:    i_retf_d16(); break;
        case 0xcb:    i_retf(); break;
        case 0xcc:    i_int3(); break;
        case 0xcd:    i_int(); break;
        case 0xce:    i_into(); break;
        case 0xcf:    i_iret(); break;
        case 0xd0:    i_d0pre(); break;
        case 0xd1:    i_d1pre(); break;
        case 0xd2:    i_d2pre(); break;
        case 0xd3:    i_d3pre(); break;
        case 0xd4:    i_aam(); break;
        case 0xd5:    i_aad(); break;
        case 0xd6:    i_notdone(); break;
        case 0xd7:    i_xlat(); break;
        case 0xd8:    i_escape(); break;
        case 0xd9:    i_escape(); break;
        case 0xda:    i_escape(); break;
        case 0xdb:    i_escape(); break;
        case 0xdc:    i_escape(); break;
        case 0xdd:    i_escape(); break;
        case 0xde:    i_escape(); break;
        case 0xdf:    i_escape(); break;
        case 0xe0:    i_loopne(); break;
        case 0xe1:    i_loope(); break;
        case 0xe2:    i_loop(); break;
        case 0xe3:    i_jcxz(); break;
        case 0xe4:    i_inal(); break;
        case 0xe5:    i_inax(); break;
        case 0xe6:    i_outal(); break;
        case 0xe7:    i_outax(); break;
        case 0xe8:    i_call_d16(); break;
        case 0xe9:    i_jmp_d16(); break;
        case 0xea:    i_jmp_far(); break;
        case 0xeb:    i_jmp_d8(); break;
        case 0xec:    i_inaldx(); break;
        case 0xed:    i_inaxdx(); break;
        case 0xee:    i_outdxal(); break;
        case 0xef:    i_outdxax(); break;
        case 0xf0:    i_lock(); break;
        case 0xf1:    i_gobios(); break;
        case 0xf2:    i_repne(); break;
        case 0xf3:    i_repe(); break;
        case 0xf4:    i_notdone(); break;
        case 0xf5:    i_cmc(); break;
        case 0xf6:    i_f6pre(); break;
        case 0xf7:    i_f7pre(); break;
        case 0xf8:    i_clc(); break;
        case 0xf9:    i_stc(); break;
        case 0xfa:    i_cli(); break;
        case 0xfb:    i_sti(); break;
        case 0xfc:    i_cld(); break;
        case 0xfd:    i_std(); break;
        case 0xfe:    i_fepre(); break;
        case 0xff:    i_ffpre(); break;
        };
#else
        instruction[GetMemInc(c_cs,ip)]();
#endif
    }
}

