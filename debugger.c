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

#ifdef DEBUGGER

#include "global.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>

#include "debugger.h"
#include "cpu.h"
#include "disasm.h"
#include "vgahard.h"

volatile int running;
volatile int breakpoint;
volatile int debug_abort;
static BYTE *bpoint;

static int numbase = 16;

#define mylower(c) ((c >= 'A' && c <= 'Z') ? c-'A'+'a' : c)

static BYTE instruction_byte;

static char wordp[] = "word ptr ";
static char bytep[] = "byte ptr ";
static char blank[] = "";

void print_regs(void)
{
    printf("\nAX=%02X%02X  BX=%02X%02X  CX=%02X%02X  DX=%02X%02X  "
           "SP=%02X%02X  BP=%02X%02X  SI=%02X%02X  DI=%02X%02X\n",
           *bregs[AH],*bregs[AL],*bregs[BH],*bregs[BL],*bregs[CH],
           *bregs[CL],*bregs[DH],*bregs[DL], *bregs[SPH],*bregs[SPL],
           *bregs[BPH],*bregs[BPL],*bregs[SIH],*bregs[SIL],*bregs[DIH],
           *bregs[DIL]);
    printf("DS=%04X  ES=%04X  SS=%04X  CS=%04X  IP=%04X %s %s %s "
           "%s %s %s %s %s %s\n",sregs[DS],sregs[ES],sregs[SS],sregs[CS],
           ip, OF ? "OV" : "NV", DF ? "DN" : "UP", IF ? "EI" : "DI",
           SF ? "NG" : "PL", ZF ? "ZR" : "NZ",AF ? "AC" : "NA",
           PF ? "PE" : "PO", CF ? "CY" : "NC",TF ? "TR" : "NT" );
}


static char *get_byte_reg(unsigned ModRM)
{
    return byte_reg[(ModRM & 0x38) >> 3];
}

static char *get_word_reg(unsigned ModRM)
{
    return word_reg[(ModRM & 0x38) >> 3];
}

static char *get_seg_reg(unsigned ModRM)
{
    return seg_reg[(ModRM & 0x38) >> 3];
}

static unsigned get_d8(BYTE *seg, unsigned *off)
{
    return GetMemInc(seg, (*off));
}

static unsigned get_d16(BYTE *seg, unsigned *off)
{
    unsigned num = GetMemInc(seg, (*off));
    num += GetMemInc(seg, (*off)) << 8;
    return num;
}

static char *get_mem(unsigned ModRM, BYTE *seg, unsigned *off, char **reg, char *msg)
{
    static char buffer[100];
    int num;
    char ch;

    switch(ModRM & 0xc0)
    {
    case 0x00:
        if ((ModRM & 0x07) != 6)
            sprintf(buffer,"%s[%s]", msg, index_reg[ModRM & 0x07]);
        else
            sprintf(buffer,"%s[%04X]", msg, get_d16(seg, off));
        break;
    case 0x40:
        if ((num = (INT8)get_d8(seg, off)) < 0)
        {
            ch = '-';
            num = -num;
        }
        else
            ch = '+';
        sprintf(buffer,"%s[%s%c%02X]", msg, index_reg[ModRM & 0x07], ch, num);
        break;
    case 0x80:
        if ((num = (INT16)get_d16(seg, off)) < 0)
        {
            ch = '-';
            num = -num;
        }
        else
            ch = '+';
        sprintf(buffer,"%s[%s%c%04X]", msg, index_reg[ModRM & 0x07], ch, num);
        break;
    case 0xc0:
        strcpy(buffer, reg[ModRM & 0x07]);
        break;
    }

    return buffer;
}

static WORD get_disp(BYTE *seg, unsigned *off)
{
    unsigned disp = GetMemInc(seg, (*off));

    return (WORD)(*off + (INT32)((INT8)disp));
}

static WORD get_disp16(BYTE *seg, unsigned *off)
{
    unsigned disp = GetMemInc(seg, (*off));
    disp += GetMemInc(seg, (*off)) << 8;

    return (WORD)(*off + (INT32)((INT16)disp));
}

static void print_instruction(BYTE *seg, unsigned off, int *tab, char *buf)
{
    unsigned ModRM = GetMemB(seg,off);
    sprintf(buf, "%-6s ", itext[tab[(ModRM & 0x38) >> 3]]);
}
    
static unsigned decode_br8(BYTE *seg, unsigned off, char *buf)
{
    unsigned ModRM = GetMemInc(seg, off);
    sprintf(buf, "%s,%s", get_mem(ModRM, seg, &off, byte_reg, blank), get_byte_reg(ModRM));
    return off;
}

static unsigned decode_r8b(BYTE *seg, unsigned off, char *buf)
{
    unsigned ModRM = GetMemInc(seg,off);
    sprintf(buf,"%s,%s", get_byte_reg(ModRM), get_mem(ModRM, seg, &off, byte_reg, blank));
    return off;
}

static unsigned decode_wr16(BYTE *seg, unsigned off, char *buf)
{
    unsigned ModRM = GetMemInc(seg,off);
    sprintf(buf, "%s,%s", get_mem(ModRM, seg, &off, word_reg, blank), get_word_reg(ModRM));
    return off;
}

static unsigned decode_r16w(BYTE *seg, unsigned off, char *buf)
{
    unsigned ModRM = GetMemInc(seg,off);
    sprintf(buf,"%s,%s", get_word_reg(ModRM), get_mem(ModRM, seg, &off, word_reg, blank));
    return off;
}

static unsigned decode_ald8(BYTE *seg, unsigned off, char *buf)
{
    sprintf(buf,"al,%02X",get_d8(seg, &off));
    return off;
}

static unsigned decode_axd16(BYTE *seg, unsigned off, char *buf)
{
    sprintf(buf,"ax,%04X",get_d16(seg, &off));
    return off;
}

static unsigned decode_pushpopseg(BYTE *seg, unsigned off, char *buf)
{
    strcpy(buf, get_seg_reg(instruction_byte));
    return off;
}

static unsigned decode_databyte(BYTE *seg, unsigned off, char *buf)
{
    sprintf(buf,"%02X", instruction_byte);
    return off;
}

static unsigned decode_wordreg(BYTE *seg, unsigned off, char *buf)
{
    strcat(buf, word_reg[instruction_byte & 0x7]);
    return off;
}


static unsigned decode_cond_jump(BYTE *seg, unsigned off, char *buf)
{
    sprintf(buf,"%-5s %04X", condition[instruction_byte & 0xf], get_disp(seg, &off));
    return off;
}

static unsigned decode_bd8(BYTE *seg, unsigned off, char *buf)
{
    unsigned ModRM = GetMemInc(seg, off);
    char *mem = get_mem(ModRM, seg, &off, byte_reg, bytep);
    sprintf(buf,"%s,%02X", mem, get_d8(seg, &off));
    return off;
}

static unsigned decode_wd16(BYTE *seg, unsigned off, char *buf)
{
    unsigned ModRM = GetMemInc(seg, off);
    char *mem = get_mem(ModRM, seg, &off, word_reg, wordp);
    sprintf(buf,"%s,%04X", mem, get_d16(seg, &off));
    return off;
}

static unsigned decode_wd8(BYTE *seg, unsigned off, char *buf)
{
    unsigned ModRM = GetMemInc(seg, off);
    char *mem = get_mem(ModRM, seg, &off, word_reg, wordp);
    sprintf(buf,"%s,%02X", mem, get_d8(seg, &off));
    return off;
}

static unsigned decode_ws(BYTE *seg, unsigned off, char *buf)
{
    unsigned ModRM = GetMemInc(seg, off);
    sprintf(buf,"%s,%s", get_mem(ModRM, seg, &off, word_reg, blank), get_seg_reg(ModRM));
    return off;
}

static unsigned decode_sw(BYTE *seg, unsigned off, char *buf)
{
    unsigned ModRM = GetMemInc(seg, off);
    sprintf(buf,"%s,%s", get_seg_reg(ModRM), get_mem(ModRM, seg, &off, word_reg, blank));
    return off;
}

static unsigned decode_w(BYTE *seg, unsigned off, char *buf)
{
    unsigned ModRM = GetMemInc(seg, off);
    strcpy(buf, get_mem(ModRM, seg, &off, word_reg, wordp));
    return off;
}

static unsigned decode_b(BYTE *seg, unsigned off, char *buf)
{
    unsigned ModRM = GetMemInc(seg, off);
    strcpy(buf, get_mem(ModRM, seg, &off, byte_reg, bytep));
    return off;
}

static unsigned decode_xchgax(BYTE *seg, unsigned off, char *buf)
{
    sprintf(buf, "ax,%s", word_reg[instruction_byte & 0x7]);
    return off;
}

static unsigned decode_far(BYTE *seg, unsigned off, char *buf)
{
    unsigned offset = get_d16(seg, &off);

    sprintf(buf,"%04X:%04X", get_d16(seg, &off), offset);
    return off;
}

static unsigned decode_almem(BYTE *seg, unsigned off, char *buf)
{
    sprintf(buf,"al,[%04X]", get_d16(seg, &off));
    return off;
}

static unsigned decode_axmem(BYTE *seg, unsigned off, char *buf)
{
    sprintf(buf,"ax,[%04X]", get_d16(seg, &off));
    return off;
}

static unsigned decode_memal(BYTE *seg, unsigned off, char *buf)
{
    sprintf(buf,"[%04X],al", get_d16(seg, &off));
    return off;
}

static unsigned decode_memax(BYTE *seg, unsigned off, char *buf)
{
    sprintf(buf,"[%04X],ax", get_d16(seg, &off));
    return off;
}

static unsigned decode_string(BYTE *seg, unsigned off, char *buf)
{
    if (instruction_byte & 0x01)
        strcat(buf,"w");
    else
        strcat(buf,"b");

    return off;
}

static unsigned decode_rd(BYTE *seg, unsigned off, char *buf)
{
    if ((instruction_byte & 0xf) > 7)
        sprintf(buf,"%s,%04X", word_reg[instruction_byte & 0x7], get_d16(seg, &off));
    else
        sprintf(buf,"%s,%02X", byte_reg[instruction_byte & 0x7], get_d8(seg, &off));

    return off;
}

static unsigned decode_d16(BYTE *seg, unsigned off, char *buf)
{
    sprintf(buf,"%04X", get_d16(seg, &off));
    return off;
}

static unsigned decode_int3(BYTE *seg, unsigned off, char *buf)
{
    strcpy(buf, "3");
    return off;
}

static unsigned decode_d8(BYTE *seg, unsigned off, char *buf)
{
    sprintf(buf,"%02X", get_d8(seg, &off));
    return off;
}

static unsigned decode_bbit1(BYTE *seg, unsigned off, char *buf)
{
    unsigned ModRM = GetMemInc(seg, off);
    sprintf(buf,"%s,1", get_mem(ModRM, seg, &off, byte_reg, bytep));
    return off;
}

static unsigned decode_wbit1(BYTE *seg, unsigned off, char *buf)
{
    unsigned ModRM = GetMemInc(seg, off);
    sprintf(buf,"%s,1", get_mem(ModRM, seg, &off, word_reg, wordp));
    return off;
}

static unsigned decode_bbitcl(BYTE *seg, unsigned off, char *buf)
{
    unsigned ModRM = GetMemInc(seg, off);
    sprintf(buf,"%s,cl", get_mem(ModRM, seg, &off, byte_reg, bytep));
    return off;
}

static unsigned decode_wbitcl(BYTE *seg, unsigned off, char *buf)
{
    unsigned ModRM = GetMemInc(seg, off);
    sprintf(buf,"%s,cl", get_mem(ModRM, seg, &off, word_reg, wordp));
    return off;
}

static unsigned decode_disp(BYTE *seg, unsigned off, char *buf)
{
    sprintf(buf,"%04X", get_disp(seg, &off));
    return off;
}

static unsigned decode_escape(BYTE *seg, unsigned off, char *buf)
{
    unsigned ModRM  = GetMemInc(seg, off);
    sprintf(buf,"%d,%s", instruction_byte & 0x7,
            get_mem(ModRM, seg, &off, nul_reg, blank));
    return off;
}

static unsigned decode_adjust(BYTE *seg, unsigned off, char *buf)
{
    unsigned num = GetMemInc(seg, off);

    if (num != 10)
        sprintf(buf, "%02X", num);
    return off;
}

static unsigned decode_d8al(BYTE *seg, unsigned off, char *buf)
{
    sprintf(buf, "%02X,al", get_d8(seg, &off));
    return off;
}

static unsigned decode_d8ax(BYTE *seg, unsigned off, char *buf)
{
    sprintf(buf, "%02X,ax", get_d8(seg, &off));
    return off;
}

static unsigned decode_axd8(BYTE *seg, unsigned off, char *buf)
{
    sprintf(buf, "ax,%02X", get_d8(seg, &off));
    return off;
}

static unsigned decode_far_ind(BYTE *seg, unsigned off, char *buf)
{
    unsigned ModRM = GetMemInc(seg, off);
    sprintf(buf, "far %s", get_mem(ModRM, seg, &off, word_reg, blank));
    return off;
}

static unsigned decode_portdx(BYTE *seg, unsigned off, char *buf)
{
    switch (instruction_byte)
    {
    case 0xec:
        strcpy(buf,"al,dx"); break;
    case 0xed:
        strcpy(buf,"ax,dx"); break;
    case 0xee:
        strcpy(buf,"dx,al"); break;
    case 0xef:
        strcpy(buf,"dx,ax"); break;
    }
     
    return off;
}

static unsigned decode_disp16(BYTE *seg, unsigned off, char *buf)
{
    sprintf(buf, "%04X", get_disp16(seg, &off));
    return off;
}

static unsigned decode_f6(BYTE *seg, unsigned off, char *buf)
{
    unsigned ModRM = GetMemB(seg, off);
    if ((ModRM & 0x38) == 0x00)
        return decode_bd8(seg, off, buf);

    return decode_b(seg, off, buf);
}

static unsigned decode_f7(BYTE *seg, unsigned off, char *buf)
{
    unsigned ModRM = GetMemB(seg, off);
    if ((ModRM & 0x38) == 0x00)
        return decode_wd16(seg, off, buf);

    return decode_w(seg, off, buf);
}

static unsigned decode_ff(BYTE *seg, unsigned off, char *buf)
{
    unsigned ModRM = (GetMemB(seg, off) & 0x38) >> 3;

    if (ModRM == 3 || ModRM == 5)
        return decode_far_ind(seg, off, buf);

    return decode_w(seg, off, buf);
}

static unsigned decode_bioscall(BYTE *seg, unsigned off, char *buf)
{
    unsigned addr;

    if (GetMemB(seg, off) == 0xf1)
    {
        off = (WORD)(off + 1);
        addr = GetMemInc(seg, off);
        addr += GetMemInc(seg, off) << 8;
        addr += GetMemInc(seg, off) << 16;
        addr += GetMemInc(seg, off) << 24;
        sprintf(buf, "bios   %08X",addr);
    }
    else
        sprintf(buf, "db     F1");

    return off;
}

static unsigned disasm(unsigned seg, unsigned off, char *buffer)
{
    BYTE *segp = &memory[(seg << 4)];
    struct Disasm *d;

    instruction_byte = GetMemInc(segp, off);
    d = &disasm_table[instruction_byte];

    if (d->supp != NULL)
        print_instruction(segp, off, d->supp, buffer);
    else
        sprintf(buffer, (d->flags & DF_NOSPACE) ? "%s" : "%-6s ",
                itext[d->text]);

    if (d->type != NULL)
        off = (d->type)(segp, off, &buffer[strlen(buffer)]);

    return off;
}

static unsigned disassemble(unsigned seg, unsigned off, int count)
{
    char buffer1[80];
    char buffer2[80];
    char buffer3[3];
    unsigned newoff;

    for (; !debug_abort && count > 0; count--)
    {
        do
        {
            printf("%04X:%04X ", seg, off);
            buffer1[0] = '\0';
            newoff = disasm(seg, off, buffer1);
            buffer2[0] = '\0';
            for (; off < newoff; off++)
            {
                sprintf(buffer3,"%02X", GetMemB(&memory[seg << 4], off));
                strcat(buffer2,buffer3);
            }
            printf("%-14s%s\n", buffer2,buffer1);
        } while (disasm_table[instruction_byte].flags & DF_PREFIX);
    }
    return off;
}

static unsigned hexdump(unsigned seg, unsigned off, unsigned count)
{
    char bytes[3*16+1];
    char ascii[16+1];
    char *byteptr, *asciiptr;
    unsigned startpos,i;
    BYTE *segp;
    BYTE ch;

    segp = &memory[seg << 4];

    while (!debug_abort && count > 0)
    {
        startpos = off & 0xf;

        byteptr = bytes;
        asciiptr = ascii;

        printf("%04X:%04X ", seg, off & 0xfff0);

        for (i = off & 0x0f; count>0 && i<0x10; i++, off=(WORD)(off+1),count--)
        {
            ch = GetMemB(segp, off);
            sprintf(byteptr, "%c%02X", i == 8 ? '-' : ' ', ch);
            sprintf(asciiptr, "%c", ch < 32 || ch > 126 ? '.' : ch);
            byteptr += 3;
            asciiptr++;
            if ((WORD)(off+1) < off)
            {
                debug_abort = TRUE;
                break;
            }
        }

        for (i = 0; i < startpos; i++)
            printf("   ");
        printf("%s", bytes);
        if (off & 0xf)
            for (i = 16-(off & 0xf); i > 0; i--)
                printf("   ");
        for (i = 0; i < startpos; i++)
            printf(" ");
        printf("   %s\n", ascii);

    }

    return off;
}

static int get_number(char *s)
{
    int i;
    char *endptr;
    long int num;
    
    for (i = 0; i < 8; i++)
        if (strcmp(word_reg[i],s) == 0)
            return ChangeE(wregs[i]);

    for (i = 0; i < 4; i++)
        if (strcmp(seg_reg[i],s) == 0)
            return sregs[i];

    if (strcmp("ip",s) == 0)
        return ip;

    num = strtol(s, &endptr, numbase);

    if (num > 65535 || num < -32768 || *endptr != '\0')
    {
        printf("Invalid number\n");
        return -1;
    }
    if (num < 0)
        num = (WORD)num;

    return num;
}
    

static int get_address(char *s, unsigned *seg, unsigned *off)
{
    char *offset;
    int num;

    offset = strchr(s,':');

    if (offset != NULL)
    {
        *offset = '\0';
        num = get_number(s);
        if (num >= 0)
        {
            *seg = (unsigned)num;
            num = get_number(offset+1);
            if (num >= 0)
                *off = (unsigned)num;
            else
                return -1;
        }
        else
            return -1;
    }
    else
    {
        num = get_number(s);
        if (num >= 0)
            *off = (unsigned)num;
        else
            return -1;
    }
    return 0;
}

static char *strlwr(char *s)
{
    for (; *s; s++)
        *s = mylower(*s);
        
    return s;
}

static int read_number(void)
{
    char inpbuf[80];
    char buffer[80];

    fgets(inpbuf, sizeof inpbuf, stdin);

    if (sscanf(inpbuf," %s \n", buffer) <= 0)
        return -1;

    return get_number(buffer);
}

static void change_reg(char *reg)
{
    int i;
    int num;

    for (i = 0; i < 8; i++)
        if (strcmp(word_reg[i],reg) == 0)
        {
            printf("%s = %04X\n:",word_reg[i], ChangeE(wregs[i]));
            num = read_number();
            if (num >= 0)
                wregs[i] = ChangeE(num);
            return;
        }

    for (i = 0; i < 4; i++)
        if (strcmp(seg_reg[i],reg) == 0)
        {
            printf("%s = %04X\n:",seg_reg[i], sregs[i]);
            num = read_number();
            if (num >= 0)
            {
                sregs[i] = num;
                switch(i)
                {
                case ES:
                    c_es = SegToMemPtr(ES); break;
                case CS:
                    c_cs = SegToMemPtr(CS); break;
                case SS:
                    c_ss = SegToMemPtr(SS); break;
                case DS:
                    c_ds = SegToMemPtr(DS); break;
                }
            }
            return;
        }

    if (strcmp("ip",reg) == 0)
    {
        printf("ip = %04X\n:", ip);
        num = read_number();
        if (num >= 0)
            ip = (WORD)num;
        return;
    }
    printf("Invalid register\n");
}


static void enter_bytes(unsigned seg, unsigned off)
{
    BYTE *b;
    int num;
    
    while (!debug_abort)
    {
        b = &memory[(seg << 4)+off];

        printf("%04X:%04X  %02X   ", seg, off, *b);
        num = read_number();
        if (num >= 0)
            *b = num & 0xff;

        off = (WORD)(off+1);
    }
}                


static void process_input(void)
{
    char buffer[1024];
    char command;
    char param1[1024];
    char param2[1024];
    int num;
    unsigned ucurrent_seg, ucurrent_off;
    unsigned dcurrent_seg, dcurrent_off;
    unsigned ecurrent_seg, ecurrent_off;
    unsigned next_ip;
    int count;
    unsigned temp;

    ucurrent_seg = sregs[CS];
    ucurrent_off = ip;
    ecurrent_seg = dcurrent_seg = sregs[DS];
    ecurrent_off = dcurrent_off = 0;

    print_regs();
    next_ip = disassemble(sregs[CS], ip, 1);

    for(;;)
    {

#ifdef __hpux
        sigset_t newmask, oldmask;
#endif
        fputc('-', stdout);
        fflush(stdout);
        fflush(stdin);

#ifdef __hpux
        sigfillset(&newmask);
        sigprocmask(SIG_SETMASK, &newmask, &oldmask);
#endif

        if (fgets(buffer, sizeof buffer, stdin) == NULL)
            exit_emu();

#ifdef __hpux
        sigprocmask(SIG_SETMASK, &oldmask, NULL);
#endif

        debug_abort = FALSE;
        
        strlwr(buffer);
        num = sscanf(buffer," %c %s %s \n", &command, param1, param2);
        
        if (num >= 1)
        {
            switch(command)
            {
            case 'x':
                printf("memory = %p\n", memory);
                printf("c_es = %p / %04X\n", c_es, (c_es-memory) >> 4);
                printf("c_cs = %p / %04X\n", c_cs, (c_cs-memory) >> 4);
                printf("c_ds = %p / %04X\n", c_ds, (c_ds-memory) >> 4);
                printf("c_ss = %p / %04X\n", c_ss, (c_ss-memory) >> 4);
                printf("c_stack = %p / %04X\n", c_stack, (c_stack-memory) >> 4);
                break;
            case 'q':
                exit_emu();
                break;
            case 'g':
                if (num == 1)
                {
                    running = TRUE;
                    return;
                }
                else
                {
                    unsigned seg,off;
                    seg = sregs[CS];
                    if (get_address(param1,&seg,&off) >= 0)
                    {
                        breakpoint = TRUE;
                        bpoint = &memory[(seg << 4) + off];
                        return;
                    }
                }
                break;
            case 't':
                return;
            case 'r':
                if (num == 1)
                {
                    print_regs();
                    next_ip = disassemble(sregs[CS],ip,1);
                    ucurrent_seg = sregs[CS];
                    ucurrent_off = ip;
                }
                else
                    change_reg(param1);
                break;
            case 'p':
                for (temp = ip;; temp = (WORD)(temp+1))
                {
                    num = memory[(sregs[CS] << 4) + temp];
                    if (num==0x26 || num==0x2e || num==0x36 || num==0x3e)
                        continue;
                    else
                        break;
                }
                switch(num)
                {         
                case 0xff:
                    num = memory[(sregs[CS] << 4) + (WORD)(temp+1)];
                    switch (num & 0x38)
                    {
                    case 0x10:
                    case 0x18:
                        break;
                    default:
                        return;
                    }
                    /* FALL THROUGH */
                case 0x9a:
                case 0xcc:
                case 0xcd:
                case 0xce:
                case 0xe0:
                case 0xe1:
                case 0xe2:
                case 0xe8:
                    running = FALSE;
                    breakpoint = TRUE;
                    bpoint = &c_cs[next_ip];
                    break;
                }
                return;
            case 's':
                pcemu_refresh();
                break;
            case 'u':
                count = 16;
                if (num > 1)
                {
                    ucurrent_seg = sregs[CS];
                    if (get_address(param1,&ucurrent_seg, &ucurrent_off) < 0)
                        break;
                    if (num > 2)
                    {
                        count = get_number(param2);
                        if (count < 0)
                            break;
                    }
                }
                ucurrent_off = disassemble(ucurrent_seg, ucurrent_off, count);
                break;
            case 'd':
                count = ((dcurrent_off + 16*8) & 0xfff0)-dcurrent_off;
                if (num > 1)
                {
                    dcurrent_seg = sregs[DS];
                    if (get_address(param1,&dcurrent_seg, &dcurrent_off) < 0)
                        break;
                    if (num > 2)
                    {
                        count = get_number(param2);
                        if (count < 0)
                            break;
                    }
                    else
                        count = ((dcurrent_off + 16*8) & 0xfff0)-dcurrent_off;
                }
                dcurrent_off = hexdump(dcurrent_seg, dcurrent_off, count);
                break;
            case 'e':
                if (num > 1)
                {
                    ecurrent_seg = sregs[DS];
                    if (get_address(param1,&ecurrent_seg, &ecurrent_off) < 0)
                        break;

                    enter_bytes(ecurrent_seg, ecurrent_off);
                }
                break;
            case 'b':
                if (num == 2 && (param1[0] == 'd' || param1[0] == 'h'))
                    numbase = param1[0] == 'd' ? 0 : 16;
                else
                    printf("Parameter must be either 'd' or 'h'\n");
                break;
            default:
                printf("Unrecognised command\n");
                break;
            }
        }
    }
}

int debug_breakin(int sig)
{
    signal(sig, (void *)debug_breakin);
    running = breakpoint = FALSE;
    
    if (in_debug)
        debug_abort = TRUE;
    return 0;
}

void call_debugger(int where)
{
    if (where == D_INT)
    {
/*        printf("Interrupt!\n"); */
        return;
    }

    if (running)
        return;

    if (breakpoint)
    {
        if (&c_cs[ip] != bpoint)
            return;
    }

    in_debug = TRUE;
    running = breakpoint = FALSE;
    CalcAll();
    process_input();
    in_debug = FALSE;
}

#endif



