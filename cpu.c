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

   16 Aug 02

   No optimisations on.                    4.96/ 5.77/ 8.02/ 5.74/ 7.64  6.43
   -O3 -fomit-fp -fno-str-reduce           8.08/10.99/13.68/11.56/13.25 11.51
   With the compiled version switch        4.40/ 5.17/ 7.14/ 5.26/ 6.84  5.76
*/
  
/* This is CPU.C  It emulates the 8086 processor */

#include "global.h"

#include <stdio.h>
#include <assert.h>

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

#ifdef PROFILER
/* For data separation profiling */
enum {
  IS_CODE = 1
};

static BYTE separation_flags[MEMORY_SIZE];
static UINT32 code_reads;
static UINT32 code_writes;
static UINT32 total_writes;
static UINT32 non_memory_writes;

static INLINEP void RecordWrite(void *x)
{
    if ((BYTE *)x >= memory && (BYTE *)x < (memory + MEMORY_SIZE))
    {
	int at;
	total_writes++;
	at = (BYTE *)x - memory;

	if (separation_flags[at] & IS_CODE)
	{
	    code_writes++;
	    separation_flags[at] &= IS_CODE;
	}
    }
    else
    {
	non_memory_writes++;
    }
}

static INLINEP void RecordCodeRead(void *x)
{
    int at;
    code_reads++;
    at = (BYTE *)x - memory;
    separation_flags[at] |= IS_CODE;
}

static INT32 last_jump_target;
static INT32 jump_target_hits[MEMORY_SIZE];
static INT32 bb_length[MEMORY_SIZE];

static INLINEP void PreJumpHook(void *x)
{
    INT32 offset = (BYTE *)x - memory;
    assert(offset >= 0 && offset < MEMORY_SIZE);

    if (last_jump_target != 0)
    {
	assert(offset >= last_jump_target);
	bb_length[last_jump_target] += offset - last_jump_target;
	assert(bb_length[last_jump_target] >= 0);
    }
}

static INLINEP void PostJumpHook(void *x)
{
    INT32 offset = (BYTE *)x - memory;
    assert(offset >= 0 && offset < MEMORY_SIZE);

    jump_target_hits[offset]++;
    last_jump_target = offset;
}
#endif

#if defined(PCEMU_LITTLE_ENDIAN) && !defined(ALIGNED_ACCESS)
#   define ReadWord(x) (*(x))
#   define WriteWord(x,y) (*(x) = (y))
#   define CopyWord(x,y) (*x = *y)
#   define PutMemW(Seg,Off,x) (Seg[Off] = (BYTE)(x), Seg[(WORD)(Off)+1] = (BYTE)((x) >> 8))
#if 0
#   define PutMemW(Seg,Off,x) (*(WORD *)((Seg)+(WORD)(Off)) = (WORD)(x))
#endif
/* dtrg. `possible pointer alignment problem, Lint says */
#   define GetMemW(Seg,Off) ((WORD)Seg[Off] + ((WORD)Seg[(WORD)(Off)+1] << 8))
#if 0
#   define GetMemW(Seg,Off) (*(WORD *)((Seg)+(Off))) */
#endif

#else
static INLINEP WORD ReadWord(void *x) 
{
    return ((WORD)(*((BYTE *)(x))) + ((WORD)(*((BYTE *)(x)+1)) << 8));
}

static INLINEP void WriteWord(void *x, WORD y) 
{
    *(BYTE *)(x) = (BYTE)(y);
    RecordWrite(x);
    *((BYTE *)(x)+1) = (BYTE)((y) >> 8);
    RecordWrite(x+1);
}

static INLINEP void CopyWord(void *x, void *y) 
{
    /* May be un-aligned, so can't use WORD * = WORD * */
    ((BYTE *)x)[0] = ((BYTE *)y)[0];
    RecordWrite(x);
    ((BYTE *)x)[1] = ((BYTE *)y)[1];
    RecordWrite(x+1);
}

static INLINEP BYTE ReadByte(void *x) 
{
    return *(BYTE *)(x);
}

static INLINEP void WriteByte(void *x, BYTE y) 
{
    *(BYTE *)(x) = y;
    RecordWrite(x);
}

static INLINEP void CopyByte(void *x, void *y) 
{
    ((BYTE *)x)[0] = ((BYTE *)y)[0];
}

#if 0
static INLINEP void PutMemW(BYTE *Seg, int Off, WORD x) 
{
    Seg[Off] = (BYTE)(x);
    Seg[(WORD)(Off)+1] = (BYTE)((x) >> 8);
}

static INLINEP WORD GetMemW(BYTE *Seg, int Off) 
{
    ((WORD)Seg[Off] + ((WORD)Seg[(WORD)(Off)+1] << 8));
}
#endif
#endif

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


#ifdef PROFILER
/* The following statistics can be profiled:
   1. Instruction usage
       Counts the number of times each instruction is executed.
       Counts at the first opcode byte level, so many of the prefixed
       instructions are bunched together.
   2. Translatable instruction run lengths.
       Given a list of those instructions that could be translated,
       records the number of translatable instructions that occur in a
       row.  Useful for deciding if it is worth translating, and how
       many instructions need to be implemented to have a reasonable
       run length.
   3. Program/data memory separation.
       Tracks memory and records the number of times that a program
       writes to memory that it is also executing in.  Useful to
       detect how much a program performs self modification and how
       often a translated piece of code may have to be flushed and
       re-translated.
   4. Jump targets and basic block length
       Tracks how many times a location is jumped to and the number of
       bytes executed before the next jump.  Useful to determine how
       'hot' blocks of code can get and how much code can be
       translated.
*/
#define NUM_ELEMS(_a)	(sizeof(_a)/sizeof(_a[0]))

/* For instruction usage profiling */
static UINT32 instruction_hits[256];

/* For run length profiling */
static UINT32 max_run[1024];
static int current_run;
static BYTE can_be_compiled[256];

/* List of all instructions that could be translated.  Used for the
   run length profiling.
*/
static INT16 can_be_compiled_seed[] = 
{
    0xD1,	/* 6232947	i_d1pre */
    0x8B,	/* 3167798	mov_r16w */
    0x72,	/* 2815182	i_jb */
    0x75,	/* 2749755	i_jnz */
    0x26,	/* 2123379	i_es */
    0x3B,	/* 1938689	i_cmp_r16w */
    0x74,	/* 1805286	i_jz */
    0x8A,	/* 1364458	i_mov_r8b */
    0x73,	/* 1221437	i_jnb */
    0xE8,	/* 1191827	i_call_d16 */
    0xC3,	/* 1158697	i_ret */
    0xD0,	/* 1032905	i_d0pre */
    0x83,	/* 977872	i_83pre */
    0x03,	/* 973522	i_add_r16w */
    0x1B,	/* 958733	i_sbb_r16w */
    0xE2,	/* 823567	i_loop */
    0xB8,	/* 714762	i_mov_axd16 */
    0x3C,	/* 697046	i_cmp_ald8 */
    0xEB,	/* 692070	i_jmp_d8 */
    0x57,	/* 690797	i_push_di */
    0x80,	/* 673698	i_80pre */
    0xC4,	/* 645550	i_les_dw */
    0x8E,	/* 622260	i_mov_sregw */
    0x50,	/* 617861	i_push_ax */
    0x0A,	/* 607696	i_or_r8b */
    0xCA,	/* 592889	i_ref_d16 */
    0x1E,	/* 591390	i_push_ds */
    0x06,	/* 565094	i_push_es */
    0x1F,	/* 562275	i_pop_ds */
    0xEE,	/* 514628	i_outdxal */
    0x89,	/* 510871	i_mov_wr16 */
    0x2A,	/* 506934	i_sub_r8b */
    0x0B,	/* 500615	i_or_r16w */
    0xFE,	/* 476796	i_fepre */
    0x9A,	/* 472654	i_call_far */
    0x55,	/* 457752 */
    0x33,	/* 450965 */
    0x5D,	/* 440111 */
    0xF7,	/* 384487 */
    0xF6,	/* 369915 */
    0x02,	/* 369334 */
    0x78,	/* 352733 */
    0xB0,	/* 350643 */
    0x5F,	/* 345447 */
    0x36,	/* 344070 */
    0x05,	/* 335741 */
    0x32,	/* 333639 */
    0x77,	/* 310386 */
    0xFF,	/* 289712 */
    0x07,	/* 278407 */
    -1
};

void dump_profiling(void)
{
    int i;

    printf("# Instruction usage summary\n");

    for (i = 0; i < NUM_ELEMS(instruction_hits); i++)
    {
	printf("0x%02X %u\n", i, instruction_hits[i]);
    }

    printf("# Maximum run summary\n");

    for (i = 0; i < NUM_ELEMS(max_run); i++)
    {
	if (max_run[i] > 0)
	{
	    printf("%u\t%u\n", i, max_run[i]);
	}
    }

    printf("# Separation summary\n"
	   "code_reads: %u\n"
	   "code_writes: %u\n"
	   "total_writes: %u\n"
	   "non_memory_writes: %u\n",
	   code_reads,
	   code_writes,
	   total_writes,
	   non_memory_writes
	);

    printf("# Jump target summary\n");

    for (i = 0; i < NUM_ELEMS(jump_target_hits); i++)
    {
	if (jump_target_hits[i] != 0)
	{
	    printf("0x%06X %u %u\n", i, jump_target_hits[i], bb_length[i]/jump_target_hits[i]);
	}
    }
}

static void init_profiler(void)
{
    int i;

    for (i = 0; can_be_compiled_seed[i] >= 0; i++)
    {
	can_be_compiled[can_be_compiled_seed[i]] = 1;
    }
}
#endif

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

#ifdef PROFILER
    init_profiler();
#endif
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

    /* PENDING: Not really a jump, but we have to include this to
       stop any unreasonable values getting into the profiling table. 
    */
    PreJumpHook(c_cs + ip);

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

    PostJumpHook(c_cs + ip);
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

#include "cpu-instr.c"

void trap(void)
{
    instruction[GetMemInc(c_cs,ip)]();
    interrupt(1);
}


void execute(void)
{
#ifdef PROFILER
    BYTE is;
    UINT32 run_length = 0;
#endif

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
#ifdef PROFILER
	RecordCodeRead(c_cs + ip);

	is = GetMemInc(c_cs, ip);
	instruction_hits[is]++;

	if (can_be_compiled[is])
	{
	    run_length++;
	}
	else
	{
	    if (run_length < NUM_ELEMS(max_run))
	    {
		max_run[run_length]++;
	    }
	    run_length = 1;
	}

	switch (is)
        {
#else
        switch(GetMemInc(c_cs,ip))
        {
#endif
#include "cpu-dispatch.c"	  
        };
#else
        instruction[GetMemInc(c_cs,ip)]();
#endif
    }
}

