MACRO i__br8;
static INLINE2 void i_#{$name}#_br8(void)
{
    /* Opcode: #{$opcode}# 
       Attr: HasModRMRMB
     */
    unsigned ModRM = (unsigned)GetMemInc(c_cs,ip);
    register unsigned src = (unsigned)*GetModRMRegB(ModRM);
    BYTE *dest = GetModRMRMB(ModRM);
    register unsigned tmp = (unsigned) *dest;
    tmp #{$op}#= src;
    CF = OF = AF = 0;
    SetZFB(tmp);
    SetSFB(tmp);
    SetPF(tmp);
    WriteByte(dest, tmp);
}

MACRO i__wr16;
static INLINE2 void i_#{$name}#_wr16(void)
{
    /* Opcode: #{$opcode}# 
       Attr: HasModRMRMW
     */
    unsigned ModRM = GetMemInc(c_cs,ip);
    WORD *src = GetModRMRegW(ModRM);
    WORD *dest = GetModRMRMW(ModRM);
    register unsigned tmp1 = (unsigned)ReadWord(src);
    register unsigned tmp2 = (unsigned)ReadWord(dest);
    register unsigned tmp3 = tmp1 #{$op}# tmp2;
    CF = OF = AF = 0;
    SetZFW(tmp3);
    SetSFW(tmp3);
    SetPF(tmp3);
    WriteWord(dest,tmp3);
}

MACRO i__r8b;
static INLINE2 void i_#{$name}#_r8b(void)
{
    /* Opcode: #{$opcode}# 
       Attr: HasModRMRMB
     */
    unsigned ModRM = (unsigned)GetMemInc(c_cs,ip);
    register unsigned src = (unsigned)*GetModRMRMB(ModRM);
    BYTE *dest = GetModRMRegB(ModRM);
    register unsigned tmp = (unsigned) *dest;
    tmp #{$op}#= src;
    CF = OF = AF = 0;
    SetZFB(tmp);
    SetSFB(tmp);
    SetPF(tmp);
    WriteByte(dest, tmp);
}

MACRO i__r16w;
static INLINE2 void i_#{$name}#_r16w(void)
{
    /* Opcode: #{$opcode}# 
       Attr: HasModRMRMW
     */
    unsigned ModRM = GetMemInc(c_cs,ip);
    WORD *dest = GetModRMRegW(ModRM);
    WORD *src = GetModRMRMW(ModRM);
    register unsigned tmp1 = (unsigned)ReadWord(src);
    register unsigned tmp2 = (unsigned)ReadWord(dest);
    register unsigned tmp3 = tmp1 #{$op}# tmp2;
    CF = OF = AF = 0;
    SetZFW(tmp3);
    SetSFW(tmp3);
    SetPF(tmp3);
    WriteWord(dest,tmp3);
}

MACRO i__ald8;
static INLINE2 void i_#{$name}#_ald8(void)
{
    /* Opcode: #{$opcode}# 
     */
    register unsigned tmp = *bregs[AL];
    tmp #{$op}#= (unsigned)GetMemInc(c_cs,ip);
    CF = OF = AF = 0;
    SetZFB(tmp);
    SetSFB(tmp);
    SetPF(tmp);
    *bregs[AL] = (BYTE)tmp;
}

MACRO i__axd16;
static INLINE2 void i_#{$name}#_axd16(void)
{
    /* Opcode: #{$opcode}# 
     */
    register unsigned src;
    register unsigned tmp = ReadWord(&wregs[AX]);
    src = GetMemInc(c_cs,ip);
    src += GetMemInc(c_cs,ip) << 8;
    tmp #{$op}#= src;
    CF = OF = AF = 0;
    SetZFW(tmp);
    SetSFW(tmp);
    SetPF(tmp);
    WriteWord(&wregs[AX],tmp);
}

IMPLEMENT i__br8 name=or,op=|,opcode=0x08;
IMPLEMENT i__wr16 name=or,op=|,opcode=0x09;
IMPLEMENT i__r8b name=or,op=|,opcode=0x0a;
IMPLEMENT i__r16w name=or,op=|,opcode=0x0b;
IMPLEMENT i__ald8 name=or,op=|,opcode=0x0c;
IMPLEMENT i__axd16 name=or,op=|,opcode=0x0d;

IMPLEMENT i__br8 name=and,op=&,opcode=0x20;
IMPLEMENT i__wr16 name=and,op=&,opcode=0x21;
IMPLEMENT i__r8b name=and,op=&,opcode=0x22;
IMPLEMENT i__r16w name=and,op=&,opcode=0x23;
IMPLEMENT i__ald8 name=and,op=&,opcode=0x24;
IMPLEMENT i__axd16 name=and,op=&,opcode=0x25;

IMPLEMENT i__br8 name=xor,op=^,opcode=0x30;
IMPLEMENT i__wr16 name=xor,op=^,opcode=0x31;
IMPLEMENT i__r8b name=xor,op=^,opcode=0x32;
IMPLEMENT i__r16w name=xor,op=^,opcode=0x33;
IMPLEMENT i__ald8 name=xor,op=^,opcode=0x34;
IMPLEMENT i__axd16 name=xor,op=^,opcode=0x35;

MACRO JumpCond;
static INLINE2 void i_j#{$name}#(void)
{
    /* Opcode: #{$opcode}# 
       Attr: IsJump,IsShortJump
     */
    register int tmp;
    PreJumpHook(c_cs + ip);

    tmp = (int)((INT8)GetMemInc(c_cs,ip));
    if (#{$cond}#) ip = (WORD)(ip+tmp);

    PostJumpHook(c_cs + ip);
}

/* 0x70 = Jump if overflow */
IMPLEMENT JumpCond name=o,cond=OF,opcode=0x70;
/* 0x71 = Jump if no overflow */
IMPLEMENT JumpCond name=no,cond=!OF,opcode=0x71;
/* 0x72 = Jump if below */
IMPLEMENT JumpCond name=b,cond=CF,opcode=0x72;
/* 0x73 = Jump if not below */
IMPLEMENT JumpCond name=nb,cond=!CF,opcode=0x73;
/* 0x74 = Jump if zero */
IMPLEMENT JumpCond name=z,cond=ZF,opcode=0x74;
/* 0x75 = Jump if not zero */
IMPLEMENT JumpCond name=nz,cond=!ZF,opcode=0x75;
/* 0x76 = Jump if below or equal */
IMPLEMENT JumpCond name=be,cond=CF || ZF,opcode=0x76;
/* 0x77 = Jump if not below or equal */
IMPLEMENT JumpCond name=nbe,cond=!(CF || ZF),opcode=0x77;
/* 0x78 = Jump if sign */
IMPLEMENT JumpCond name=s,cond=SF,opcode=0x78;
/* 0x79 = Jump if no sign */
IMPLEMENT JumpCond name=ns,cond=!SF,opcode=0x79;
/* 0x7a = Jump if parity */
IMPLEMENT JumpCond name=p,cond=PF,opcode=0x7a;
/* 0x7b = Jump if no parity */
IMPLEMENT JumpCond name=np,cond=!PF,opcode=0x7b;
/* 0x7c = Jump if less */
IMPLEMENT JumpCond name=l,cond=(!(!SF)!=!(!OF))&&!ZF,opcode=0x7c;
/* 0x7d = Jump if not less */
IMPLEMENT JumpCond name=nl,cond=ZF||(!(!SF) == !(!OF)),opcode=0x7d;
/* 0x7e = Jump if less than or equal */
IMPLEMENT JumpCond name=le,cond=ZF||(!(!SF) != !(!OF)),opcode=0x7e;
/* 0x7f = Jump if not less than or equal*/
IMPLEMENT JumpCond name=nle,cond=(!(!SF)==!(!OF))&&!ZF,opcode=0x7f;

static INLINE2 void i_add_br8(void)
{
    /* Opcode: 0x00 
       Length: 2
       Attr: HasModRMRMB
     */

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

    WriteByte(dest, tmp);
}


static INLINE2 void i_add_wr16(void)
{
    /* Opcode: 0x01 
       Length: 2
       Attr: HasModRMRMW
     */

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
    /* Opcode: 0x02 
       Length: 2
       Attr: HasModRMRMB
     */

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

    WriteByte(dest, tmp);

}


static INLINE2 void i_add_r16w(void)
{
    /* Opcode: 0x03 
       Length: 2
       Attr: HasModRMRMW
     */

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
    /* Opcode: 0x04 
       Length: 2
     */

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
    /* Opcode: 0x05 
       Length: 3
     */

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
    /* Opcode: 0x06 
       Length: 1
     */

    PushSeg(ES);
}


static INLINE2 void i_pop_es(void)
{
    /* Opcode: 0x07 
       Length: 1
     */
    PopSeg(c_es,ES);
}

/* most OR instructions go here... */

static INLINE2 void i_push_cs(void)
{
    /* Opcode: 0x0e 
       Length: 1
     */

    PushSeg(CS);
}


static INLINE2 void i_adc_br8(void)
{
    /* Opcode: 0x10 
       Length: 2
       Attr: HasModRMRMB
     */

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

    WriteByte(dest, tmp);
}


static INLINE2 void i_adc_wr16(void)
{
    /* Opcode: 0x11 
       Length: 2
       Attr: HasModRMRMW
     */

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
    /* Opcode: 0x12 
       Attr: HasModRMRMB
     */

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

    WriteByte(dest, tmp);
}


static INLINE2 void i_adc_r16w(void)
{
    /* Opcode: 0x13 
       Length: 2
       Attr: HasModRMRMW
     */

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
    /* Opcode: 0x14 
     */

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
    /* Opcode: 0x15 
       Length: 3
     */

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
    /* Opcode: 0x16 
       Length: 1
     */

    PushSeg(SS);
}


static INLINE2 void i_pop_ss(void)
{
    /* Opcode: 0x17 
     */

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
    /* Opcode: 0x18 
       Attr: HasModRMRMB
     */

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

    WriteByte(dest, tmp);
}


static INLINE2 void i_sbb_wr16(void)
{
    /* Opcode: 0x19 
       Length: 3
       Attr: HasModRMRMW
     */

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
    /* Opcode: 0x1a 
       Attr: HasModRMRMB
     */

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

    WriteByte(dest, tmp);

}


static INLINE2 void i_sbb_r16w(void)
{
    /* Opcode: 0x1b 
       Length: 2
       Attr: HasModRMRMW
     */

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
    /* Opcode: 0x1c 
     */

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
    /* Opcode: 0x1d 
       Length: 3
     */

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
    /* Opcode: 0x1e 
       Length: 1
     */

    PushSeg(DS);
}


static INLINE2 void i_pop_ds(void)
{
    /* Opcode: 0x1f 
       Length: 1
     */
    PopSeg(c_ds,DS);
}

/* most AND instructions go here... */

static INLINE2 void i_es(void)
{
    /* Opcode: 0x26
       Attr: IsPrefix
     */

    c_ds = c_ss = c_es;

    instruction[GetMemInc(c_cs,ip)]();

    c_ds = SegToMemPtr(DS);
    c_ss = SegToMemPtr(SS);
}

static INLINE2 void i_daa(void)
{
    /* Opcode: 0x27
     */
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
    /* Opcode: 0x28 
       Length: 2
       Attr: HasModRMRMB
     */

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

    WriteByte(dest, tmp);

}


static INLINE2 void i_sub_wr16(void)
{
    /* Opcode: 0x29 
       Length: 2
       Attr: HasModRMRMW
     */

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
    /* Opcode: 0x2a 
       Length: 2
       Attr: HasModRMRMB
     */

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

    WriteByte(dest, tmp);
}


static INLINE2 void i_sub_r16w(void)
{
    /* Opcode: 0x2b 
       Length: 2
       Attr: HasModRMRMW
     */

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
    /* Opcode: 0x2c 
       Length: 2
     */

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
    /* Opcode: 0x2d 
       Length: 3
     */

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
    /* Opcode: 0x2e 
       Attr: IsPrefix
     */

    c_ds = c_ss = c_cs;

    instruction[GetMemInc(c_cs,ip)]();

    c_ds = SegToMemPtr(DS);
    c_ss = SegToMemPtr(SS);
}


/* most XOR instructions go here */

static INLINE2 void i_ss(void)
{
    /* Opcode: 0x36 
       Length: 3
     */

    c_ds = c_ss;

    instruction[GetMemInc(c_cs,ip)]();

    c_ds = SegToMemPtr(DS);
}

static INLINE2 void i_aaa(void)
{
    /* Opcode: 0x37 
       Length: 1
     */
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
    /* Opcode: 0x38 
       Length: 2
       Attr: HasModRMRMB
     */

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
    /* Opcode: 0x39
       Length: 2
       Attr: HasModRMRMW
     */

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
    /* Opcode: 0x3a 
       Length: 2
       Attr: HasModRMRMB
     */

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
    /* Opcode: 0x3b 
       Length: 2
       Attr: HasModRMRMW
     */

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
    /* Opcode: 0x3c 
       Length: 2
     */

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
    /* Opcode: 0x3d 
       Length: 3
     */

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
    /* Opcode: 0x3e 
     */

    c_ss = c_ds;

    instruction[GetMemInc(c_cs,ip)]();

    c_ss = SegToMemPtr(SS);
}

static INLINE2 void i_aas(void)
{
    /* Opcode: 0x3f 
       Length: 1
     */
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
    /* Opcode: 0x40 
       Length: 1
     */
    IncWordReg(AX);
}


static INLINE2 void i_inc_cx(void)
{
    /* Opcode: 0x41 
       Length: 1
     */
    IncWordReg(CX);
}


static INLINE2 void i_inc_dx(void)
{
    /* Opcode: 0x42 
       Length: 1
     */
    IncWordReg(DX);
}


static INLINE2 void i_inc_bx(void)
{
    /* Opcode: 0x43 
       Length: 1
     */
    IncWordReg(BX);
}


static INLINE2 void i_inc_sp(void)
{
    /* Opcode: 0x44 
     */
    IncWordReg(SP);
}


static INLINE2 void i_inc_bp(void)
{
    /* Opcode: 0x45 
     */
    IncWordReg(BP);
}


static INLINE2 void i_inc_si(void)
{
    /* Opcode: 0x46 
       Length: 1
     */
    IncWordReg(SI);
}


static INLINE2 void i_inc_di(void)
{
    /* Opcode: 0x47 
       Length: 1
     */
    IncWordReg(DI);
}


static INLINE2 void i_dec_ax(void)
{
    /* Opcode: 0x48 
       Length: 1
     */
    DecWordReg(AX);
}


static INLINE2 void i_dec_cx(void)
{
    /* Opcode: 0x49 
       Length: 1
     */
    DecWordReg(CX);
}


static INLINE2 void i_dec_dx(void)
{
    /* Opcode: 0x4a 
       Length: 1
     */
    DecWordReg(DX);
}


static INLINE2 void i_dec_bx(void)
{
    /* Opcode: 0x4b 
       Length: 1
     */
    DecWordReg(BX);
}


static INLINE2 void i_dec_sp(void)
{
    /* Opcode: 0x4c 
       Length: 1
     */
    DecWordReg(SP);
}


static INLINE2 void i_dec_bp(void)
{
    /* Opcode: 0x4d 
     */
    DecWordReg(BP);
}


static INLINE2 void i_dec_si(void)
{
    /* Opcode: 0x4e 
       Length: 1
     */
    DecWordReg(SI);
}


static INLINE2 void i_dec_di(void)
{
    /* Opcode: 0x4f 
       Length: 1
     */
    DecWordReg(DI);
}


static INLINE2 void i_push_ax(void)
{
    /* Opcode: 0x50 
       Length: 1
     */
    PushWordReg(AX);
}


static INLINE2 void i_push_cx(void)
{
    /* Opcode: 0x51 
       Length: 1
     */
    PushWordReg(CX);
}


static INLINE2 void i_push_dx(void)
{
    /* Opcode: 0x52 
       Length: 1
     */
    PushWordReg(DX);
}


static INLINE2 void i_push_bx(void)
{
    /* Opcode: 0x53 
       Length: 1
     */
    PushWordReg(BX);
}


static INLINE2 void i_push_sp(void)
{
    /* Opcode: 0x54 
     */
    PushWordReg(SP);
}


static INLINE2 void i_push_bp(void)
{
    /* Opcode: 0x55 
       Length: 1
     */
    PushWordReg(BP);
}



static INLINE2 void i_push_si(void)
{
    /* Opcode: 0x56 
       Length: 1
     */
    PushWordReg(SI);
}


static INLINE2 void i_push_di(void)
{
    /* Opcode: 0x57 
       Length: 1
     */
    PushWordReg(DI);
}


static INLINE2 void i_pop_ax(void)
{
    /* Opcode: 0x58 
       Length: 1
     */
    PopWordReg(AX);
}


static INLINE2 void i_pop_cx(void)
{
    /* Opcode: 0x59 
       Length: 1
     */
    PopWordReg(CX);
}


static INLINE2 void i_pop_dx(void)
{
    /* Opcode: 0x5a 
       Length: 1
     */
    PopWordReg(DX);
}


static INLINE2 void i_pop_bx(void)
{
    /* Opcode: 0x5b 
       Length: 1
     */
    PopWordReg(BX);
}


static INLINE2 void i_pop_sp(void)
{
    /* Opcode: 0x5c 
     */
    PopWordReg(SP);
}


static INLINE2 void i_pop_bp(void)
{
    /* Opcode: 0x5d 
       Length: 1
     */
    PopWordReg(BP);
}


static INLINE2 void i_pop_si(void)
{
    /* Opcode: 0x5e 
       Length: 1
     */
    PopWordReg(SI);
}


static INLINE2 void i_pop_di(void)
{
    /* Opcode: 0x5f 
       Length: 1
     */
    PopWordReg(DI);
}

static INLINE2 void i_push_immed16(void)
{
    /* Opcode: 0x68 
     */
    register unsigned tmp1 = (WORD)(ReadWord(&wregs[SP])-2);
    unsigned val = GetMemInc(c_cs, ip);
    val |= GetMemInc(c_cs, ip) << 8;

    WriteWord(&wregs[SP],tmp1);
    PutMemW(c_stack,tmp1,val);
}

static INLINE2 void i_imul_r16_immed16(void)
{
    /* Opcode: 0x69
     */
    /* PENDING */
}

static INLINE2 void i_outs_dx_rm8(void)
{
    /* Opcode: 0x6e
     */
    /* PENDING */
}

static INLINE2 void i_outs_dx_rm16(void)
{
    /* Opcode: 0x6f
     */
    /* PENDING */
}

/* Conditional jumps from 0x70 to 0x7f */

static INLINE2 void i_80pre(void)
{
    /* Opcode: 0x80
       Length: 3
       Attr: HasModRMRMB
     */
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
        
        WriteByte(dest, tmp2);
        break;
    case 0x08:  /* OR eb,d8 */
        tmp |= src;
        
        CF = OF = AF = 0;
        
        SetZFB(tmp);
        SetSFB(tmp);
        SetPF(tmp);
        
        WriteByte(dest, tmp);
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
        
        WriteByte(dest, tmp2);
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
        
        WriteByte(dest, tmp);
        break;
    case 0x20:  /* AND eb,d8 */
        tmp &= src;
        
        CF = OF = AF = 0;
        
        SetZFB(tmp);
        SetSFB(tmp);
        SetPF(tmp);
        
        WriteByte(dest, tmp);
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
        
        WriteByte(dest, tmp);
        break;
    case 0x30:  /* XOR eb,d8 */
        tmp ^= src;
        
        CF = OF = AF = 0;
        
        SetZFB(tmp);
        SetSFB(tmp);
        SetPF(tmp);
        
        WriteByte(dest, tmp);
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
    /* Opcode: 0x81
       Length: 4
       Attr: HasModRMRMW
     */

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
    /* Opcode: 0x83 
       Length: 3
       Attr: IsPrefix
       Attr: HasModRMRMW
     */
    /*        PENDING */

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
    /* Opcode: 0x84 
       Length: 2
       Attr: HasModRMRMB
     */

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
    /* Opcode: 0x85 
       Length: 2
       Attr: HasModRMRMW
     */

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
    /* Opcode: 0x86 
       Length: 2
       Attr: HasModRMRMB
     */

    unsigned ModRM = (unsigned)GetMemInc(c_cs,ip);
    register BYTE *src = GetModRMRegB(ModRM);
    register BYTE *dest = GetModRMRMB(ModRM);
    BYTE tmp;

    tmp = *src;
    WriteByte(src, *dest);
    WriteByte(dest, tmp);
}


static INLINE2 void i_xchg_wr16(void)
{
    /* Opcode: 0x87 
       Length: 2
       Attr: HasModRMRMW
     */

    unsigned ModRM = GetMemInc(c_cs,ip);
    register BYTE *src = (BYTE *)GetModRMRegW(ModRM);
    register BYTE *dest = (BYTE *)GetModRMRMW(ModRM);
    BYTE tmp1,tmp2;

    /* PENDING: Optimise this? */
    tmp1 = src[0];
    tmp2 = src[1];
    WriteByte(src, dest[0]);
    WriteByte(src+1, dest[1]);
    WriteByte(dest, tmp1);
    WriteByte(dest+1, tmp2);

}


static INLINE2 void i_mov_br8(void)
{
    /* Opcode: 0x88 
       Length: 2
       Attr: HasModRMRMB
     */

    register unsigned ModRM = GetMemInc(c_cs,ip);
    register BYTE src = *GetModRMRegB(ModRM);
    register BYTE *dest = GetModRMRMB(ModRM);

    WriteByte(dest, src);
}


static INLINE2 void i_mov_wr16(void)
{
    /* Opcode: 0x89 
       Length: 2
       Args: ModRMW
       Attr: HasModRMRMW
     */

    register unsigned ModRM = GetMemInc(c_cs,ip);
    register WORD *src = GetModRMRegW(ModRM);
    register WORD *dest = GetModRMRMW(ModRM);

    CopyWord(dest,src);
}


static INLINE2 void i_mov_r8b(void)
{
    /* Opcode: 0x8a
       Length: 2
       Args: ModRMB
       Attr: HasModRMRMB
     */

    register unsigned ModRM = GetMemInc(c_cs,ip);
    register BYTE *dest = GetModRMRegB(ModRM);
    register BYTE src = *GetModRMRMB(ModRM);

    WriteByte(dest, src);
}


static INLINE2 void i_mov_r16w(void)
{
    /* Opcode: 0x8b 
       Length: 2
       Attr: HasModRMRMW
     */

    register unsigned ModRM = GetMemInc(c_cs,ip);
    register WORD *dest = GetModRMRegW(ModRM);
    register WORD *src = GetModRMRMW(ModRM);

    CopyWord(dest,src);
}


static INLINE2 void i_mov_wsreg(void)
{
    /* Opcode: 0x8c 
       Length: 2
       Attr: HasModRMRMW
     */

    register unsigned ModRM = GetMemInc(c_cs,ip);
    register WORD *dest = GetModRMRMW(ModRM);

    WriteWord(dest,sregs[(ModRM & 0x38) >> 3]);
}


static INLINE2 void i_lea(void)
{
    /* Opcode: 0x8d 
       Length: 3
     */

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
    /* Opcode: 0x8e
       Attr: IsPrefix,HasModRMRMW
     */
    /* PENDING */

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
    /* Opcode: 0x8f
       Length: 2
       Attr: HasModRMRMW
     */
    
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
    /* Opcode: 0x90 
       Length: 1
     */
}


static INLINE2 void i_xchg_axcx(void)
{
    /* Opcode: 0x91 
       Length: 1
     */
    XchgAXReg(CX);
}


static INLINE2 void i_xchg_axdx(void)
{
    /* Opcode: 0x92 
       Length: 1
     */
    XchgAXReg(DX);
}


static INLINE2 void i_xchg_axbx(void)
{
    /* Opcode: 0x93 
       Length: 1
     */
    XchgAXReg(BX);
}


static INLINE2 void i_xchg_axsp(void)
{
    /* Opcode: 0x94 
     */
    XchgAXReg(SP);
}


static INLINE2 void i_xchg_axbp(void)
{
    /* Opcode: 0x95 
     */
    XchgAXReg(BP);
}


static INLINE2 void i_xchg_axsi(void)
{
    /* Opcode: 0x96 
       Length: 1
     */
    XchgAXReg(SI);
}


static INLINE2 void i_xchg_axdi(void)
{
    /* Opcode: 0x97 
       Length: 1
     */
    XchgAXReg(DI);
}

static INLINE2 void i_cbw(void)
{
    /* Opcode: 0x98 
       Length: 1
     */

    *bregs[AH] = (*bregs[AL] & 0x80) ? 0xff : 0;
}
        
static INLINE2 void i_cwd(void)
{
    /* Opcode: 0x99 
       Length: 1
     */

    wregs[DX] = (*bregs[AH] & 0x80) ? ChangeE(0xffff) : ChangeE(0);
}


static INLINE2 void i_call_far(void)
{
    /* Opcode: 0x9a
       Attr: IsJump,IsCall
     */
    register unsigned tmp, tmp1, tmp2;

    PreJumpHook(c_cs + ip);

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

    PostJumpHook(c_cs + ip);
}


static INLINE2 void i_wait(void)
{
    /* Opcode: 0x9b 
     */
    
    return;
}


static INLINE2 void i_pushf(void)
{
    /* Opcode: 0x9c 
       Length: 1
     */

    register unsigned tmp1 = (ReadWord(&wregs[SP])-2);
    WORD tmp2 = CompressFlags() | 0xf000;

    PutMemW(c_stack,tmp1,tmp2);
    WriteWord(&wregs[SP],tmp1);
}


static INLINE2 void i_popf(void)
{
    /* Opcode: 0x9d 
       Length: 1
     */

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
    /* Opcode: 0x9e 
     */

    unsigned tmp = (CompressFlags() & 0xff00) | (*bregs[AH] & 0xd5);

    ExpandFlags(tmp);
}


static INLINE2 void i_lahf(void)
{
    /* Opcode: 0x9f 
     */

    *bregs[AH] = CompressFlags() & 0xff;
}

static INLINE2 void i_mov_aldisp(void)
{
    /* Opcode: 0xa0 
       Length: 3
     */

    register unsigned addr;

    addr = GetMemInc(c_cs,ip);
    addr += GetMemInc(c_cs, ip) << 8;

    *bregs[AL] = GetMemB(c_ds, addr);
}


static INLINE2 void i_mov_axdisp(void)
{
    /* Opcode: 0xa1 
       Length: 3
     */

    register unsigned addr;

    addr = GetMemInc(c_cs, ip);
    addr += GetMemInc(c_cs, ip) << 8;

    *bregs[AL] = GetMemB(c_ds, addr);
    *bregs[AH] = GetMemB(c_ds, addr+1);
}


static INLINE2 void i_mov_dispal(void)
{
    /* Opcode: 0xa2 
       Length: 3
     */

    register unsigned addr;

    addr = GetMemInc(c_cs,ip);
    addr += GetMemInc(c_cs, ip) << 8;

    PutMemB(c_ds, addr, *bregs[AL]);
}


static INLINE2 void i_mov_dispax(void)
{
    /* Opcode: 0xa3 
       Length: 3
     */

    register unsigned addr;

    addr = GetMemInc(c_cs, ip);
    addr += GetMemInc(c_cs, ip) << 8;

    PutMemB(c_ds, addr, *bregs[AL]);
    PutMemB(c_ds, addr+1, *bregs[AH]);
}


static INLINE2 void i_movsb(void)
{
    /* Opcode: 0xa4 
       Length: 1
     */

    register unsigned di = ReadWord(&wregs[DI]);
    register unsigned si = ReadWord(&wregs[SI]);

    BYTE tmp = GetMemB(c_ds,si);

    PutMemB(c_es, di, tmp);

    di += -2*DF +1;
    si += -2*DF +1;

    WriteWord(&wregs[DI],di);
    WriteWord(&wregs[SI],si);
}


static INLINE2 void i_movsw(void)
{
    /* Opcode: 0xa5 
       Length: 1
     */

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
    /* Opcode: 0xa6 
     */

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
    /* Opcode: 0xa7 
     */

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
    /* Opcode: 0xa8 
       Length: 2
     */

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
    /* Opcode: 0xa9 
       Length: 3
     */

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
    /* Opcode: 0xaa 
       Length: 1
     */

    register unsigned di = ReadWord(&wregs[DI]);

    PutMemB(c_es,di,*bregs[AL]);
    di += -2*DF +1;
    WriteWord(&wregs[DI],di);
}

static INLINE2 void i_stosw(void)
{
    /* Opcode: 0xab 
       Length: 1
     */

    register unsigned di = ReadWord(&wregs[DI]);

    PutMemB(c_es,di,*bregs[AL]);
    PutMemB(c_es,di+1,*bregs[AH]);
    di += -4*DF +2;;
    WriteWord(&wregs[DI],di);
}

static INLINE2 void i_lodsb(void)
{
    /* Opcode: 0xac 
       Length: 1
     */

    register unsigned si = ReadWord(&wregs[SI]);

    *bregs[AL] = GetMemB(c_ds,si);
    si += -2*DF +1;
    WriteWord(&wregs[SI],si);
}

static INLINE2 void i_lodsw(void)
{
    /* Opcode: 0xad 
       Length: 1
     */

    register unsigned si = ReadWord(&wregs[SI]);
    register unsigned tmp = GetMemW(c_ds,si);

    si +=  -4*DF+2;
    WriteWord(&wregs[SI],si);
    WriteWord(&wregs[AX],tmp);
}
    
static INLINE2 void i_scasb(void)
{
    /* Opcode: 0xae 
       Length: 1
     */

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
    /* Opcode: 0xaf 
       Length: 1
     */

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
    /* Opcode: 0xb0 
       Length: 2
     */

    *bregs[AL] = GetMemInc(c_cs,ip);
}


static INLINE2 void i_mov_cld8(void)
{
    /* Opcode: 0xb1 
       Length: 2
     */

    *bregs[CL] = GetMemInc(c_cs,ip);
}


static INLINE2 void i_mov_dld8(void)
{
    /* Opcode: 0xb2 
       Length: 2
     */

    *bregs[DL] = GetMemInc(c_cs,ip);
}


static INLINE2 void i_mov_bld8(void)
{
    /* Opcode: 0xb3 
       Length: 2
     */

    *bregs[BL] = GetMemInc(c_cs,ip);
}


static INLINE2 void i_mov_ahd8(void)
{
    /* Opcode: 0xb4 
       Length: 2
     */

    *bregs[AH] = GetMemInc(c_cs,ip);
}


static INLINE2 void i_mov_chd8(void)
{
    /* Opcode: 0xb5 
       Length: 2
     */

    *bregs[CH] = GetMemInc(c_cs,ip);
}


static INLINE2 void i_mov_dhd8(void)
{
    /* Opcode: 0xb6 
       Length: 2
     */

    *bregs[DH] = GetMemInc(c_cs,ip);
}


static INLINE2 void i_mov_bhd8(void)
{
    /* Opcode: 0xb7 
       Length: 2
     */

    *bregs[BH] = GetMemInc(c_cs,ip);
}


static INLINE2 void i_mov_axd16(void)
{
    /* Opcode: 0xb8 
       Length: 3
     */

    *bregs[AL] = GetMemInc(c_cs,ip);
    *bregs[AH] = GetMemInc(c_cs,ip);
}


static INLINE2 void i_mov_cxd16(void)
{
    /* Opcode: 0xb9 
       Length: 3
     */

    *bregs[CL] = GetMemInc(c_cs,ip);
    *bregs[CH] = GetMemInc(c_cs,ip);
}


static INLINE2 void i_mov_dxd16(void)
{
    /* Opcode: 0xba 
       Length: 3
     */

    *bregs[DL] = GetMemInc(c_cs,ip);
    *bregs[DH] = GetMemInc(c_cs,ip);
}


static INLINE2 void i_mov_bxd16(void)
{
    /* Opcode: 0xbb 
       Length: 3
     */

    *bregs[BL] = GetMemInc(c_cs,ip);
    *bregs[BH] = GetMemInc(c_cs,ip);
}


static INLINE2 void i_mov_spd16(void)
{
    /* Opcode: 0xbc 
     */

    *bregs[SPL] = GetMemInc(c_cs,ip);
    *bregs[SPH] = GetMemInc(c_cs,ip);
}


static INLINE2 void i_mov_bpd16(void)
{
    /* Opcode: 0xbd 
       Length: 3
     */

    *bregs[BPL] = GetMemInc(c_cs,ip);
    *bregs[BPH] = GetMemInc(c_cs,ip);
}


static INLINE2 void i_mov_sid16(void)
{
    /* Opcode: 0xbe 
       Length: 3
     */

    *bregs[SIL] = GetMemInc(c_cs,ip);
    *bregs[SIH] = GetMemInc(c_cs,ip);
}


static INLINE2 void i_mov_did16(void)
{
    /* Opcode: 0xbf 
       Length: 3
     */

    *bregs[DIL] = GetMemInc(c_cs,ip);
    *bregs[DIH] = GetMemInc(c_cs,ip);
}


static INLINE2 void i_ret_d16(void)
{
    /* Opcode: 0xc2 
       Attr: IsJump,IsRet
     */

    register unsigned tmp = ReadWord(&wregs[SP]);
    register unsigned count;

    PreJumpHook(c_cs + ip);

    count = GetMemInc(c_cs,ip);
    count += GetMemInc(c_cs,ip) << 8;

    ip = GetMemW(c_stack,tmp);
    tmp += count+2;
    WriteWord(&wregs[SP],tmp);

    PostJumpHook(c_cs + ip);
}


static INLINE2 void i_ret(void)
{
    /* Opcode: 0xc3 
       Attr: IsJump,IsRet
     */

    register unsigned tmp = ReadWord(&wregs[SP]);
    
    PreJumpHook(c_cs + ip);

    ip = GetMemW(c_stack,tmp);
    tmp += 2;
    WriteWord(&wregs[SP],tmp);

    PostJumpHook(c_cs + ip);
}


static INLINE2 void i_les_dw(void)
{
    /* Opcode: 0xc4
       Length: 2
       Attr: HasModRMRMW
     */

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
    /* Opcode: 0xc5 
       Length: 3
       Attr: HasModRMRMW
     */

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
    /* Opcode: 0xc6 
       Length: 3
       Attr: HasModRMRMB
     */

    unsigned ModRM = GetMemInc(c_cs,ip);
    register BYTE *dest = GetModRMRMB(ModRM);

    *dest = GetMemInc(c_cs,ip);
}


static INLINE2 void i_mov_wd16(void)
{
    /* Opcode: 0xc7 
       Length: 4
       Attr: HasModRMRMW
     */

    unsigned ModRM = GetMemInc(c_cs,ip);
    register BYTE *dest = (BYTE *)GetModRMRMW(ModRM);

    /* PENDING: Optimise */
    WriteByte(dest, GetMemInc(c_cs,ip));
    dest++;
    WriteByte(dest, GetMemInc(c_cs,ip));
}


static INLINE2 void i_retf_d16(void)
{
    /* Opcode: 0xca 
       Attr: IsJump,IsRet
     */

    register unsigned tmp = ReadWord(&wregs[SP]);
    register unsigned count;

    PreJumpHook(c_cs + ip);

    count = GetMemInc(c_cs,ip);
    count += GetMemInc(c_cs,ip) << 8;

    ip = GetMemW(c_stack,tmp);
    tmp = (WORD)(tmp+2);
    sregs[CS] = GetMemW(c_stack,tmp);
    c_cs = SegToMemPtr(CS);
    tmp += count+2;
    WriteWord(&wregs[SP],tmp);

    PostJumpHook(c_cs + ip);
}


static INLINE2 void i_retf(void)
{
    /* Opcode: 0xcb 
       Attr: IsJump,IsRet
     */

    register unsigned tmp = ReadWord(&wregs[SP]);

    PreJumpHook(c_cs + ip);

    ip = GetMemW(c_stack,tmp);
    tmp = (WORD)(tmp+2);
    sregs[CS] = GetMemW(c_stack,tmp);
    c_cs = SegToMemPtr(CS);
    tmp += 2;
    WriteWord(&wregs[SP],tmp);

    PostJumpHook(c_cs + ip);
}


static INLINE2 void i_int3(void)
{
    /* Opcode: 0xcc 
     */

    interrupt(3);
}


static INLINE2 void i_int(void)
{
    /* Opcode: 0xcd
       Attr: IsJump
     */

    unsigned int_num = GetMemInc(c_cs,ip);

    interrupt(int_num);
}


static INLINE2 void i_into(void)
{
    /* Opcode: 0xce 
     */

    if (OF)
        interrupt(4);
}


static INLINE2 void i_iret(void)
{
    /* Opcode: 0xcf 
       Attr: IsJump,IsRet
     */

    register unsigned tmp = ReadWord(&wregs[SP]);

    PreJumpHook(c_cs + ip);

    ip = GetMemW(c_stack,tmp);
    tmp = (WORD)(tmp+2);
    sregs[CS] = GetMemW(c_stack,tmp);
    c_cs = SegToMemPtr(CS);
    tmp += 2;
    WriteWord(&wregs[SP],tmp);

    PostJumpHook(c_cs + ip);

    i_popf();
#ifdef DEBUGGER
    call_debugger(D_TRACE);
#endif
}    


static INLINE2 void i_d0pre(void)
{
    /* Opcode: 0xd0 
       Length: 2
       Attr: HasModRMRMB
     */

    unsigned ModRM = GetMemInc(c_cs,ip);
    register BYTE *dest = GetModRMRMB(ModRM);
    register unsigned tmp = *dest;
    register unsigned tmp2 = tmp;

    switch (ModRM & 0x38)
    {
    case 0x00:  /* ROL eb,1 */
        CF = (tmp & 0x80) != 0;
        WriteByte(dest, (tmp << 1) + CF);
        OF = !(!(tmp & 0x40)) != CF;
        break;
    case 0x08:  /* ROR eb,1 */
        CF = (tmp & 0x01) != 0;
        WriteByte(dest, (tmp >> 1) + (CF << 7));
        OF = !(!(tmp & 0x80)) != CF;
        break;
    case 0x10:  /* RCL eb,1 */
        OF = (tmp ^ (tmp << 1)) & 0x80;
        WriteByte(dest, (tmp << 1) + CF);
        CF = (tmp & 0x80) != 0;
        break;
    case 0x18:  /* RCR eb,1 */
        WriteByte(dest, (tmp >> 1) + (CF << 7));
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
        
        WriteByte(dest, (BYTE)tmp);
        break;
    case 0x28:  /* SHR eb,1 */
        CF = (tmp & 0x01) != 0;
        OF = tmp & 0x80;
        
        tmp2 = tmp >> 1;
        
        SetSFB(tmp2);
        SetPF(tmp2);
        SetZFB(tmp2);
        AF = 1;
        WriteByte(dest, (BYTE)tmp2);
        break;
    case 0x38:  /* SAR eb,1 */
        CF = (tmp & 0x01) != 0;
        OF = 0;
        
        tmp2 = (tmp >> 1) | (tmp & 0x80);
        
        SetSFB(tmp2);
        SetPF(tmp2);
        SetZFB(tmp2);
        AF = 1;
        WriteByte(dest, (BYTE)tmp2);
        break;
    }
}


static INLINE2 void i_d1pre(void)
{
    /* Opcode: 0xd1 
       Attr: HasModRMRMW,IsPrefix
     */

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
    /* Opcode: 0xd2 
       Length: 2
       Attr: HasModRMRMB
     */

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
        WriteByte(dest, (BYTE)tmp);
        break;
    case 0x08:  /* ROR eb,CL */
        for (; count > 0; count--)
        {
            CF = (tmp & 0x01) != 0;
            tmp = (tmp >> 1) + (CF << 7);
        }
        WriteByte(dest, (BYTE)tmp);
        break;
    case 0x10:  /* RCL eb,CL */
        for (; count > 0; count--)
        {
            tmp2 = CF;
            CF = (tmp & 0x80) != 0;
            tmp = (tmp << 1) + tmp2;
        }
        WriteByte(dest, (BYTE)tmp);
        break;
    case 0x18:  /* RCR eb,CL */
        for (; count > 0; count--)
        {
            tmp2 = (tmp >> 1) + (CF << 7);
            CF = (tmp & 0x01) != 0;
            tmp = tmp2;
        }
        WriteByte(dest, (BYTE)tmp);
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
        
        WriteByte(dest, (BYTE)tmp);
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
        WriteByte(dest, (BYTE)tmp);
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
        WriteByte(dest, (BYTE)tmp);
        break;
    }
}


static INLINE2 void i_d3pre(void)
{
    /* Opcode: 0xd3 
       Length: 2
       Attr: HasModRMRMW
     */

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
    /* Opcode: 0xd4 
       Length: 2
     */
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
    /* Opcode: 0xd5 
       Length: 2
     */
    unsigned mult = GetMemInc(c_cs,ip);

    *bregs[AL] = *bregs[AH] * mult + *bregs[AL];
    *bregs[AH] = 0;

    SetPF(*bregs[AL]);
    SetZFB(*bregs[AL]);
    SF = 0;
}

static INLINE2 void i_xlat(void)
{
    /* Opcode: 0xd7 
     */

    unsigned dest = ReadWord(&wregs[BX])+*bregs[AL];
    
    *bregs[AL] = GetMemB(c_ds, dest);
}

static INLINE2 void i_escape(void)
{
    /* Opcode: 0xd8,0xd9,0xda,0xdb,0xdc,0xdd,0xde,0xdf 
       Attr: HasModRMRMB
     */

    unsigned ModRM = GetMemInc(c_cs,ip);
    GetModRMRMB(ModRM);
}

static INLINE2 void i_loopne(void)
{
    /* Opcode: 0xe0 
       Attr: IsJump,IsShortJump
     */

    register int disp = (int)((INT8)GetMemInc(c_cs,ip));
    register unsigned tmp = ReadWord(&wregs[CX])-1;

    PreJumpHook(c_cs + ip);

    WriteWord(&wregs[CX],tmp);

    if (!ZF && tmp) ip = (WORD)(ip+disp);

    PostJumpHook(c_cs + ip);
}

static INLINE2 void i_loope(void)
{
    /* Opcode: 0xe1 
       Attr: IsJump,IsShortJump
     */

    register int disp = (int)((INT8)GetMemInc(c_cs,ip));
    register unsigned tmp = ReadWord(&wregs[CX])-1;

    PreJumpHook(c_cs + ip);

    WriteWord(&wregs[CX],tmp);

    if (ZF && tmp) ip = (WORD)(ip+disp);

    PostJumpHook(c_cs + ip);
}

static INLINE2 void i_loop(void)
{
    /* Opcode: 0xe2 
       Attr: IsJump,IsShortJump
     */

    register int disp = (int)((INT8)GetMemInc(c_cs,ip));
    register unsigned tmp = ReadWord(&wregs[CX])-1;

    PreJumpHook(c_cs + ip);

    WriteWord(&wregs[CX],tmp);

    if (tmp) ip = (WORD)(ip+disp);

    PostJumpHook(c_cs + ip);
}

static INLINE2 void i_jcxz(void)
{
    /* Opcode: 0xe3 
       Attr: IsJump,IsShortJump
     */

    register int disp = (int)((INT8)GetMemInc(c_cs,ip));

    PreJumpHook(c_cs + ip);
    
    if (wregs[CX] == 0)
        ip = (WORD)(ip+disp);

    PostJumpHook(c_cs + ip);
}

static INLINE2 void i_inal(void)
{
    /* Opcode: 0xe4 
       Length: 2
     */

    unsigned port = GetMemInc(c_cs,ip);

    *bregs[AL] = read_port(port);
}

static INLINE2 void i_inax(void)
{
    /* Opcode: 0xe5 
     */

    unsigned port = GetMemInc(c_cs,ip);

    *bregs[AL] = read_port(port);
    *bregs[AH] = read_port(port+1);
}
    
static INLINE2 void i_outal(void)
{
    /* Opcode: 0xe6 
       Length: 2
     */

    unsigned port = GetMemInc(c_cs,ip);

    write_port(port, *bregs[AL]);
}

static INLINE2 void i_outax(void)
{
    /* Opcode: 0xe7 
     */

    unsigned port = GetMemInc(c_cs,ip);

    write_port(port, *bregs[AL]);
    write_port(port+1, *bregs[AH]);
}

static INLINE2 void i_call_d16(void)
{
    /* Opcode: 0xe8 
       Attr: IsJump,IsCall
     */

    register unsigned tmp;
    register unsigned tmp1 = (WORD)(ReadWord(&wregs[SP])-2);

    PreJumpHook(c_cs + ip);

    tmp = GetMemInc(c_cs,ip);
    tmp += GetMemInc(c_cs,ip) << 8;

    PutMemW(c_stack,tmp1,ip);
    WriteWord(&wregs[SP],tmp1);

    ip = (WORD)(ip+(INT16)tmp);

    PostJumpHook(c_cs + ip);
}


static INLINE2 void i_jmp_d16(void)
{
    /* Opcode: 0xe9 
       Attr: IsJump,IsShortJump
     */

    register int tmp = GetMemInc(c_cs,ip);

    PreJumpHook(c_cs + ip);

    tmp += GetMemInc(c_cs,ip) << 8;

    ip = (WORD)(ip+(INT16)tmp);

    PostJumpHook(c_cs + ip);
}


static INLINE2 void i_jmp_far(void)
{
    /* Opcode: 0xea 
       Attr: IsJump
     */

    register unsigned tmp,tmp1;

    PreJumpHook(c_cs + ip);

    tmp = GetMemInc(c_cs,ip);
    tmp += GetMemInc(c_cs,ip) << 8;

    tmp1 = GetMemInc(c_cs,ip);
    tmp1 += GetMemInc(c_cs,ip) << 8;

    sregs[CS] = (WORD)tmp1;
    c_cs = SegToMemPtr(CS);
    ip = (WORD)tmp;

    PostJumpHook(c_cs + ip);
}


static INLINE2 void i_jmp_d8(void)
{
    /* Opcode: 0xeb 
       Attr: IsJump,IsShortJump
     */
    register int tmp = (int)((INT8)GetMemInc(c_cs,ip));

    PreJumpHook(c_cs + ip);

    ip = (WORD)(ip+tmp);

    PostJumpHook(c_cs + ip);
}


static INLINE2 void i_inaldx(void)
{
    /* Opcode: 0xec 
       Length: 1
     */

    *bregs[AL] = read_port(ReadWord(&wregs[DX]));
}

static INLINE2 void i_inaxdx(void)
{
    /* Opcode: 0xed 
     */

    unsigned port = ReadWord(&wregs[DX]);

    *bregs[AL] = read_port(port);
    *bregs[AH] = read_port(port+1);
}

static INLINE2 void i_outdxal(void)
{
    /* Opcode: 0xee 
     */

    write_port(ReadWord(&wregs[DX]), *bregs[AL]);
}

static INLINE2 void i_outdxax(void)
{
    /* Opcode: 0xef 
     */
    unsigned port = ReadWord(&wregs[DX]);

    write_port(port, *bregs[AL]);
    write_port(port+1, *bregs[AH]);
}

static INLINE2 void i_lock(void)
{
    /* Opcode: 0xf0 
     */
}

static INLINE2 void i_gobios(void)
{
    /* Opcode: 0xf1 
       Length: 6
     */
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
    /* Opcode: 0xf2 
       Length: 2
     */

    rep(0);
}


static INLINE2 void i_repe(void)
{
    /* Opcode: 0xf3 
       Length: 2
     */

    rep(1);
}


static INLINE2 void i_cmc(void)
{
    /* Opcode: 0xf5 
       Length: 1
     */

    CF = !CF;
}


static INLINE2 void i_f6pre(void)
{
    /* Opcode: 0xf6 
       Attr: IsPrefix,HasModRMRMB
     */
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
        WriteByte(byte, ~tmp);
        break;
        
    case 0x18:	/* NEG Eb */
        tmp2 = tmp;
        tmp = -tmp;
        
        CF = (int)tmp2 > 0;
        
        SetAF(tmp,0,tmp2);
        SetZFB(tmp);
        SetSFB(tmp);
        SetPF(tmp);
        
        WriteByte(byte, tmp);
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
    /* Opcode: 0xf7
       Attr: IsPrefix,HasModRMRMW
     */
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
    /* Opcode: 0xf8 
       Length: 1
     */

    CF = 0;
}


static INLINE2 void i_stc(void)
{
    /* Opcode: 0xf9 
       Length: 1
     */

    CF = 1;
}


static INLINE2 void i_cli(void)
{
    /* Opcode: 0xfa 
       Length: 1
     */

    IF = 0;
}


static INLINE2 void i_sti(void)
{
    /* Opcode: 0xfb 
       Length: 1
     */

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
    /* Opcode: 0xfc 
       Length: 1
     */

    DF = 0;
}


static INLINE2 void i_std(void)
{
    /* Opcode: 0xfd 
       Length: 1
     */

    DF = 1;
}


static INLINE2 void i_fepre(void)
{
    /* Opcode: 0xfe 
       Length: 2
       Attr: HasModRMRMB
     */

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
    
    WriteByte(dest, (BYTE)tmp1);
}


static INLINE2 void i_ffpre(void)
{
    /* PENDING: Some are jumps so mark all as such */
    /* Opcode: 0xff 
       Attr: IsJump,HasModRMRMW
     */

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
	PreJumpHook(c_cs + ip);

        tmp = ReadWord(dest);
        tmp1 = (WORD)(ReadWord(&wregs[SP])-2);
        
        PutMemW(c_stack,tmp1,ip);
        WriteWord(&wregs[SP],tmp1);
        
        ip = (WORD)tmp;

	PostJumpHook(c_cs + ip);
        break;
        
    case 0x18:  /* CALL FAR ea */
	PreJumpHook(c_cs + ip);

        tmp1 = (WORD)(ReadWord(&wregs[SP])-2);
        
        PutMemW(c_stack,tmp1,sregs[CS]);
        tmp1 = (WORD)(tmp1-2);
        PutMemW(c_stack,tmp1,ip);
        WriteWord(&wregs[SP],tmp1);
        
        ip = ReadWord(dest);
        dest++;
        sregs[CS] = ReadWord(dest);
        c_cs = SegToMemPtr(CS);

	PostJumpHook(c_cs + ip);
        break;
        
    case 0x20:  /* JMP ea */
	PreJumpHook(c_cs + ip);

        ip = ReadWord(dest);

	PostJumpHook(c_cs + ip);
        break;
        
    case 0x28:  /* JMP FAR ea */
	PreJumpHook(c_cs + ip);

        ip = ReadWord(dest);
        dest++;
        sregs[CS] = ReadWord(dest);
        c_cs = SegToMemPtr(CS);

	PostJumpHook(c_cs + ip);
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
    /* Opcode: 0x0f,0x2f,0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x6a,0x6b,0x6c,0x6d,0x82,0xc0,0xc1,0xc8,0xc9,0xd6,0xf4
     */
    fprintf(stderr,"Error: Unimplemented opcode %02X at cs:ip = %04X:%04X\n",
		    c_cs[ip-1],sregs[CS],ip-1);
    exit(1);
}
