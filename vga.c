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

/* This is VGA.C It contains stuff about the VGA BIOS */

#include "global.h"

#include <stdio.h>

#include "vga.h"
#include "bios.h"
#include "cpu.h"
#include "vgahard.h"
#include "video.h"
#include "hardware.h"

#define MAXPAGES 8

#define VIDEOFUNCTIONALITY 0x2000

static BYTE *data_segment;   /* pointer to 0040:0000 */
static BYTE *screen_mem;     /* pointer to screen memory */

#define MAXMODE (sizeof defaults/sizeof defaults[0])

static struct
{
    int cs, ce;
    int sx, sy;
    int colours;
    int pages;
    int base;
    int text;
} defaults[] = 
{
    { 6, 7, 40, 25, 16, 8, 0xb800, TRUE },   /* Mode 0x00 */
    { 6, 7, 40, 25, 16, 8, 0xb800, TRUE },   /* Mode 0x01 */
    { 6, 7, 80, 25, 16, 8, 0xb800, TRUE },   /* Mode 0x02 */
    { 6, 7, 80, 25, 16, 8, 0xb800, TRUE },   /* Mode 0x03 */
    { 0, 0, 320, 200, 4, 4, 0xb800, FALSE }, /* Mode 0x04 */
    { 0, 0, 320, 200, 4, 4, 0xb800, FALSE }, /* Mode 0x05 */
    { 0, 0, 640, 200, 2, 4, 0xb800, FALSE }, /* Mode 0x06 */
    { 11, 12, 80, 25, 2, 8, 0xb000, TRUE },  /* Mode 0x07 */
    { 0, 0, 0, 0, 0, 0, 0, FALSE },          /* Mode 0x08 */
    { 0, 0, 0, 0, 0, 0, 0, FALSE },          /* Mode 0x09 */
    { 0, 0, 0, 0, 0, 0, 0, FALSE },          /* Mode 0x0a */
    { 0, 0, 0, 0, 0, 0, 0, FALSE },          /* Mode 0x0b */
    { 0, 0, 0, 0, 0, 0, 0, FALSE },          /* Mode 0x0c */
    { 0, 0, 320, 200, 16, 8, 0xa000, FALSE },/* Mode 0x0d */
    { 0, 0, 640, 200, 16, 4, 0xa000, FALSE },/* Mode 0x0e */
    { 0, 0, 640, 350, 2, 2, 0xa000, FALSE }, /* Mode 0x0f */
    { 0, 0, 640, 350, 16, 2, 0xa000, FALSE },/* Mode 0x10 */
    { 0, 0, 640, 480, 2, 1, 0xa000, FALSE }, /* Mode 0x11 */
    { 0, 0, 640, 480, 16, 1, 0xa000, FALSE },/* Mode 0x12 */
    { 0, 0, 320, 200, 256, 1, 0xa000, FALSE },/* Mode 0x13 */
};

int mono;                    /* TRUE if on monochrome screen */

/* Remember: screen height is stored one less than actual!!! */

#define SetMode(m)      PutMemB(data_segment, 0x49, m)
#define GetMode()       GetMemB(data_segment, 0x49)
#define SetWidth(w)     PutMemW(data_segment, 0x4a, w)
#define GetWidth()      GetMemW(data_segment, 0x4a)
#define SetHeight(h)    PutMemB(data_segment, 0x84, h)
#define GetHeight()     GetMemB(data_segment, 0x84)
#define SetSize(s)      PutMemW(data_segment, 0x4c, s)
#define GetSize()       GetMemW(data_segment, 0x4c)
#define SetOffset(o)    PutMemW(data_segment, 0x4e, o)
#define GetOffset()     GetMemW(data_segment, 0x4e)
#define SetCx(page,cx)  PutMemB(data_segment, 0x50+(page << 1), cx)
#define GetCx(page)     GetMemB(data_segment, 0x50+(page << 1))
#define SetCy(page,cy)  PutMemB(data_segment, 0x51+(page << 1), cy)
#define GetCy(page)     GetMemB(data_segment, 0x51+(page << 1))
#define SetCs(s)        PutMemB(data_segment, 0x61, s)
#define GetCs()         GetMemB(data_segment, 0x61)
#define SetCe(s)        PutMemB(data_segment, 0x60, s)
#define GetCe()         GetMemB(data_segment, 0x60)
#define SetCp(cp)       PutMemB(data_segment, 0x62, cp)
#define GetCp()         GetMemB(data_segment, 0x62)
#define GetCRT()        GetMemW(data_segment, 0x63)
#define SetCRT(c)       PutMemW(data_segment, 0x63, c)
#define SetCharHeight(c) PutMemW(data_segment, 0x85, c)
#define GetCharHeight() GetMemW(data_segment, 0x85)

static int cur_emulate = TRUE;
static int pages;            /* Maximum number of pages */

#define page_start(page) &screen_mem[GetSize()*(page)]


static BYTE functionality[] = 
{
    0xff,               /* Modes 0->7 supported */
    0x00,               /* Modes 8->0x0f not supported yet */
    0x00,               /* Modes 0x10->0x13 not supported yet */
    0,0,0,0,            /* Reserved bytes */
    0x04,               /* Only 400 scan lines supported at the moment */
    0x01,               /* Character blocks */
    0x01,               /* Number of active character blocks */
    0x04+0x08+0x16,     /* Various functions */
    0x04+0x08,          /* ditto */
    0,0,                /* Reserved */
    0x00,               /* Various saved things */
    0x00,               /* Reserved */
};


static BYTE *get_offset(int page)
{
    return &screen_mem[(GetSize() * page) + (2 * (GetCx(page) +
                                                  GetCy(page)*GetWidth()))];
}


static void clearscr(int x1,int y1,int x2,int y2,int attr)
{
    BYTE *scr = &screen_mem[GetOffset()];
    int x,y;
    unsigned width = GetWidth();

    for (y = y1; y <= y2; y++)
    {
        for (x = x1; x <= x2; x++)
        {
            scr[2*(x+y*width)] = ' ';
            scr[2*(x+y*width)+1] = (BYTE)attr;
        }
    }
    update_display();
}


static void scroll(int x1, int y1, int x2, int y2, int num, int attr)
{
    int x,y;
    unsigned width = GetWidth();
    BYTE *scr;

    if (x1 > x2 || y1 > y2)
        return;

    scr = &screen_mem[GetOffset()];

    if (num == 0 || y2-y1 < abs(num))
    {
        clearscr(x1,y1,x2,y2, attr);
        return;
    }

    if (num > 0)
    {
        disable();    /* To prevent occasional screen glitch */
        for (y = y1; y <= y2-num; y++)
            for (x = x1; x <= x2; x++)
            {
                scr[2*(x+y*width)] = scr[2*(x+(y+num)*width)];
                scr[2*(x+y*width)+1] = scr[2*(x+(y+num)*width)+1];
            }
        y = y2-num+1;
        copy_text(x1,y1+num,x2,y2,x1,y1);
        enable();
    }
    else
    {
        num = -num;
        disable();    /* To prevent occasional screen glitch */
        for (y = y2-num; y >= y1; y--)
            for (x = x1; x <= x2; x++)
            {
                scr[2*(x+(y+num)*width)] = scr[2*(x+y*width)];
                scr[2*(x+(y+num)*width)+1] = scr[2*(x+y*width)+1];
            }
        y = y1;
        copy_text(x1,y1,x2,y2-num,x1,y1+num);
        enable();     
    }
    clearscr(x1,y,x2,y+num-1,attr);
}


void set_cursor_type(unsigned start, unsigned end)
{
    if (start == 0x20)
        set_cursor_height(0, -1);
    else
    {
        start &= 0x1f;
        end &= 0x1f;

        SetCs(start);
        SetCe(end);
        if (cur_emulate && GetMode() <= 3 && start <= 8 && end <= 8)
        {
            start = (GetCharHeight()*start)/8;
            end = (GetCharHeight()*end)/8;
        }
        set_cursor_height(start, end);
    }
}


static void set_mode(int mode)
{
    if (mode < MAXMODE)
    {
        if (!mono || (defaults[mode].colours <= 2 && mono))
        {
            int i;

            if (!defaults[mode].text)
                return;
            SetMode(mode);
            SetWidth(defaults[mode].sx);
            SetHeight(defaults[mode].sy-1);
            SetSize(defaults[mode].sx * defaults[mode].sy * 2);
            SetCp(0);
            SetOffset(0);
            if (mode == 0x7)
                SetCRT(0x3b4);
            else
                SetCRT(0x3d4);
            pages = defaults[mode].pages;
            for (i = 0; i < pages; i++)
            {
                SetCx(i,0);
                SetCy(i,0);
            }
            SetCharHeight(16);
            set_cursor_type(defaults[mode].cs,defaults[mode].ce);
            move_cursor(0, 0);
            screen_mem = &memory[defaults[mode].base << 4];
            new_screen(GetWidth(), GetHeight()+1, screen_mem);
        }
    }
}


static void display_message(void)
{
    static char message1[] =
        "PC Emulator v1.01alpha (C) 1994 University of Bristol";
    static char message2[] =
        "Please report comments, bugs etc to hedley@cs.bris.ac.uk";
    BYTE *mem = get_offset(0);
    int i;

    disable();

    for (i = 0; i < sizeof message1-1; i++)
        mem[i<<1] = message1[i];

    for (i = 0; i < sizeof message2-1; i++)
        mem[(i+80)<<1] = message2[i];

    SetCx(0, 0);
    SetCy(0, 4);
    move_cursor(GetCx(0),GetCy(0));
    enable();

}


static void fill_in_functionality(void)
{
    WORD tmp = ChangeE(wregs[DI]);
    PutMemW(c_es, tmp, VIDEOFUNCTIONALITY);
    tmp += 2;
    PutMemW(c_es, tmp, 0xf000);
    tmp += 2;
    PutMemB(c_es, tmp, GetMode());
    tmp++;
    PutMemW(c_es, tmp, GetWidth());
    tmp += 2;
    PutMemW(c_es, tmp, GetSize());
    tmp += 2;
    PutMemW(c_es, tmp, GetSize() * GetCp());
    tmp += 2;
    
    PutMemB(c_es, tmp, GetCy(0));
    tmp++;
    PutMemB(c_es, tmp, GetCx(0));
    tmp++;
    PutMemB(c_es, tmp, GetCy(1));
    tmp++;
    PutMemB(c_es, tmp, GetCx(1));
    tmp++;
    PutMemB(c_es, tmp, GetCy(2));
    tmp++;
    PutMemB(c_es, tmp, GetCx(2));
    tmp++;
    PutMemB(c_es, tmp, GetCy(3));
    tmp++;
    PutMemB(c_es, tmp, GetCx(3));
    tmp++;
    PutMemB(c_es, tmp, GetCy(4));
    tmp++;
    PutMemB(c_es, tmp, GetCx(4));
    tmp++;
    PutMemB(c_es, tmp, GetCy(5));
    tmp++;
    PutMemB(c_es, tmp, GetCx(5));
    tmp++;
    PutMemB(c_es, tmp, GetCy(6));
    tmp++;
    PutMemB(c_es, tmp, GetCx(6));
    tmp++;
    PutMemB(c_es, tmp, GetCy(7));
    tmp++;
    PutMemB(c_es, tmp, GetCx(7));
    tmp++;
    
    PutMemB(c_es, tmp, GetCs());
    tmp++;
    PutMemB(c_es, tmp, GetCe());
    tmp++;
    PutMemB(c_es, tmp, GetCp());
    tmp++;
    
    PutMemW(c_es, tmp, GetCRT());
    tmp += 2;
    
    PutMemB(c_es, tmp, 0);
    tmp++;
    PutMemB(c_es, tmp, 0);
    tmp++;
 
    PutMemB(c_es, tmp, GetHeight()+1);
    tmp++;
    PutMemW(c_es, tmp, GetCharHeight());
    tmp += 2;
    PutMemB(c_es, tmp, mono ? 7 : 8);
    tmp++;
    PutMemB(c_es, tmp, 0);
    tmp++;
    PutMemW(c_es, tmp, mono ? 0 : 256);
    tmp += 2;
    PutMemB(c_es, tmp, defaults[GetMode()].pages);
    tmp++;
    PutMemB(c_es, tmp, 0x02);
    tmp++;
    PutMemB(c_es, tmp, 0);
    tmp++;
    PutMemB(c_es, tmp, 0);
    tmp++;
    PutMemB(c_es, tmp, mono ? 4 : 0);
    tmp++;
    PutMemW(c_es, tmp, 0);
    tmp += 2;
    PutMemB(c_es, tmp, 0);
    tmp++;
    PutMemB(c_es, tmp, 3);
    tmp++;
    PutMemB(c_es, tmp, 1);
}


void int_video(void)
{
    unsigned page, al;
    unsigned height, width;
    
    D(printf("In Video. Function = %02X\n", *bregs[AH]););

    CalcAll();
    page = *bregs[BH];
    al = *bregs[AL];
    height = GetHeight();
    width = GetWidth()-1;

    switch(*bregs[AH])
    {
    case 0:     /* Set mode */
        set_mode(al & 0x7f);
        if ((al & 0x80) == 0)
            clearscr(0,0,width, height, 0x07);
        break;
    case 1:     /* Set cursor type */
        set_cursor_type(*bregs[CH], *bregs[CL]);
        break;
    case 2:     /* Move cursor position */
        if (page < pages)
        {
            SetCx(page, *bregs[DL]);
            SetCy(page, *bregs[DH]);
            move_cursor(GetCx(page),GetCy(page));
        }
        break;
    case 3:     /* Get cursor position */
        if (page < pages)
        {
            *bregs[CH] = GetCs();
            *bregs[CL] = GetCe();
            *bregs[DL] = GetCx(page);
            *bregs[DH] = GetCy(page);
        }
        break;
    case 4:     /* Light pen (!) */
        *bregs[AH] = 0;
        wregs[BX] = wregs[CX] = wregs[DX] = 0;
        break;
    case 5:     /* Set display page */
        if (al < pages)
        {
            SetCp(al);
            SetOffset(al*GetSize());
            new_screen(width+1, height+1, page_start(al));
            update_display();
        }
        break;
    case 6:     /* Scroll down/clear window */
        scroll(*bregs[CL], *bregs[CH], *bregs[DL], *bregs[DH], al, *bregs[BH]);
        break;
    case 7:     /* Scroll up/clear window */
        scroll(*bregs[CL], *bregs[CH], *bregs[DL], *bregs[DH], -al,*bregs[BH]);
        break;
    case 8:     /* Read character and attribute at cursor */
        if (page < pages)
        {
            BYTE *pos = get_offset(page);
            
            *bregs[AL] = *pos++;
            *bregs[AH] = *pos;
        }
        break;
    case 9:     /* Write character(s) and attribute(s) at cursor */
        if (page < pages)
        {
            BYTE *pos = get_offset(page);
            unsigned i, count;
            
            count = ChangeE(wregs[CX]);
            for (i = count; i > 0; i--)
            {
                *pos++ = al;
                *pos++ = *bregs[BL];
            }
            if (count > 1)
                update_display();
            else
                if (count == 1)
                    displaychar();
        }
        break;
    case 0x0a:  /* Write character at cursor */
        if (page < pages)
        {
            BYTE *pos = get_offset(page);
            unsigned i, count;

            count = ChangeE(wregs[CX]);
            for (i = count; i > 0; i--)
                *pos++ = al;

            if (count > 1)
                update_display();
            else
                if (count == 1)
                    displaychar();
        }
        break;

    case 0x0b:  /* Set palette/background/border */
        break;
    case 0x0e:  /* Write character in teletype mode */
        if (page < pages)
        {
            BYTE *pos = get_offset(page);
            int x,y;

            x = GetCx(page);
            y = GetCy(page);
            switch(al)
            {
            case 0x0a:  /* Line feed */
                y++;
                break;
            case 0x0d:  /* Return */
                x = 0;
                break;
            case 0x8:   /* Backspace */
                x--;
                break;
            case 0x7:   /* Bell */
                fprintf(stderr,"\a");
                fflush(stderr);
                break;
            default:
                *pos = al;
                x++;
                displaychar();
            }
            if (x > width)
            {
                x = 0;
                y++;
            }
            if (x < 0)
            {
                x = width;
                y--;
            }
            if (y > height)
            {
                BYTE attr;
                y = height;
                attr = screen_mem[GetSize()*(page+1) -1];
                scroll(0,0,width,y,1,attr);
            }
            if (y < 0)
            {
                y = 0;
                scroll(0,0,width,height,-1,7);
            }
            SetCx(page,x);
            SetCy(page,y);
            move_cursor(x,y);
        }
        break;
    case 0x0f:  /* Return information */
        *bregs[AL] = GetMode();
        *bregs[AH] = width+1;
        *bregs[BH] = GetCp();
        break;
    case 0x10:  /* Colour/palette stuff... */
        break;
    case 0x11:  /* Font stuff... */
        switch(*bregs[AL])
        {
        case 0x02:      /* Load rom 8x8 font (set 50/42 lines) */
        case 0x12:
            height = 49;
            SetHeight(height);
            SetSize((width+1) * (height+1) * 2);
            new_screen(width+1, height+1, screen_mem);
            clearscr(0,0,width, height, 0x07);
            break;
        case 0x30:      /* Get font information */
            switch(*bregs[BH])
            {
            default:
                wregs[CX] = ChangeE(16);
                *bregs[DL] = height;
                sregs[ES] = 0xf000;
                c_es = SegToMemPtr(ES);
                wregs[BP] = ChangeE(0);
            }
            break;
        default:
            printf("Unimplemented int 0x10 function 0x11 sub-function %02X\n",*bregs[AL]);
#ifdef DEBUG
            loc();
            exit_emu();
#endif
            break;
        }
        break;
    case 0x12:  /* Display information */
        switch(*bregs[BL])
        {
        case 0x10:      /* Get config info */
            *bregs[BH] = mono;
            *bregs[BL] = 3;
            wregs[CX] = ChangeE(0);
            break;
        case 0x30:      /* Set scan lines */
            *bregs[AL] = 0x12;
            break;
        case 0x34:      /* Enable/disable cursor emulation */
            cur_emulate = *bregs[AL] == 0;
            *bregs[AL] = 0x12;
            break;
        default:
            printf("Unimplemented int 10 function 0x12 sub-function 0x%02X\n",*bregs[BL]);
#ifdef DEBUG
            loc();
            exit_emu();
#endif
            break;
        }
        break;
    case 0x1a:  /* Get/Set display combination */
        if (*bregs[AL] == 0)
        {
            *bregs[BL] = mono ? 7 : 8; 
            *bregs[BH] = 0;
            *bregs[AL] = 0x1a;
        }
        break;
    case 0x1b:  /* Get functionality/state information etc */
        fill_in_functionality();
        *bregs[AL] = 0x1b;
        break;
    case 0x30:  /* ??? Called by MSD.EXE */
    case 0x4f:  /* VESA functions... */
    case 0x6f:  /* ??? Called by borland C  */
    case 0xcc:  /* ??? Called by Norton's disk edit */
    case 0xef:  /* ??? Called by QBASIC.EXE */
    case 0xfa:  /* ??? Called by MSD.EXE (Microsoft Diagonstics) */
    case 0xfe:  /* ??? - but several programs call it. Appears to do nothing */
    case 0xff:  /* ??? Called by WordPerfect 5.1 */
        break;
    default:
        printf("Unimplemented int 10 function: %02X. AL = %02X  BL = %02X\n",
               *bregs[AH], *bregs[AL], *bregs[BL]);
#ifdef DEBUG
        loc();
        exit_emu();
#endif
        break;
    }
}

void init_vga(void)
{
    int mode = mono ? 7 : 3;
    static BYTE afterint[] =
    {
        0xfb, 0xca, 0x02, 0x00
    };

    init_hardware();

    screen_mem = &memory[defaults[mode].base << 4];
    data_segment = &memory[0x00400];

    memcpy(&memory[0xf0000+VIDEOFUNCTIONALITY], &functionality, sizeof functionality);

    set_mode(mode);
    clearscr(0,0,GetWidth()-1, GetHeight(), 0x07);
    set_int(0x10, NULL, 0, int_video, afterint, sizeof afterint);
    display_message();
}

void vga_off(void)
{
    finish_hardware();
}
