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

/* This is BIOS.C It contains stuff about the ROM BIOS (at 0xf000:xxxx) */

#include "global.h"

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <sys/stat.h>

#include "cpu.h"
#include "vga.h"
#include "bios.h"
#include "debugger.h"
#include "vgahard.h"
#include "hardware.h"

#define BIOS

#include "mfs_link.h"

void (*bios_routine[256])(void);

static BYTE *BIOS_base;
static BYTE *data_segment;

#define SYSTEMID 0xFB

#define BIOSDATE "06/09/94"     /* Damned American date format :) */
#define BIOSCOPYRIGHT "(c) 1994 University of Bristol"

#define INT_ROUTINE_START 0x1000

#define KB_1 (data_segment[0x17])
#define KB_2 (data_segment[0x18])

unsigned modifiers = 0;

#define SHIFT_R 1
#define SHIFT_L 2
#define CTRL    4
#define ALT     8
#define SCROLL  16
#define NUMLOCK 32
#define CAPS    64
#define INSERT  128

#define CTRL_L  1
#define ALT_L   2
#define SYSREQ  4
#define HOLD    8

#define CTRL_R  4
#define ALT_R   8

#define update_flag(flag, key, state) \
       if (state) flag &= ~(key); else flag |= key;

void open_drive(DiskTab [], int);

int numhdisks = 0;
int numfdisks = 2;

DiskTab fdisk[MAXFDISKS] = {
	{"/dev/fd0", 18, 80, 2, 1, 0},
	{"/dev/fd1", 18, 80, 2, 1, 0}
};

DiskTab hdisk[MAXHDISKS] = {
	{"/dev/zero", 0, 0, 0, 0, 0},
	{"/dev/zero", 0, 0, 0, 0, 0}
};

int bootdisk = 0x0;
static unsigned pos = INT_ROUTINE_START;

#define diskerror data_segment[0x41]

#define GetCurKeyBufStart()     GetMemW(data_segment, 0x1a)
#define GetCurKeyBufEnd()       GetMemW(data_segment, 0x1c)
#define GetKeyBufStart()        GetMemW(data_segment, 0x80)
#define GetKeyBufEnd()          GetMemW(data_segment, 0x82)

#define SetCurKeyBufStart(addr) PutMemW(data_segment, 0x1a, addr)
#define SetCurKeyBufEnd(addr)   PutMemW(data_segment, 0x1c, addr)
#define SetKeyBufStart(addr)    PutMemW(data_segment, 0x80, addr)
#define SetKeyBufEnd(addr)      PutMemW(data_segment, 0x82, addr)

static unsigned num = 0;

struct vm86_struct vm86s;
char tmpdir[] = "/tmp";

#include "keytabs.h"


void loc(void)
{
    unsigned count = 200;
    unsigned sp, seg, off;

    for (sp = ChangeE(wregs[SP]); count > 0 ; sp+=2, count--)
    {
        seg = GetMemW(c_stack, (WORD)(sp+2));
        off = (WORD)(GetMemW(c_stack, sp) - 2);
        if (memory[(seg << 4) + off] == 0xcd)
            break;
    }
    if (count == 0)
        printf("Sorry - no information as to the location of the problem\n");
    else
        printf("Routine called from cs:ip = %04X:%04X\n", seg, off);
}


static void put_ticks(UINT32 ticks)
{
    PutMemB(data_segment, 0x6c, (BYTE)ticks);
    ticks >>= 8;
    PutMemB(data_segment, 0x6d, (BYTE)ticks);
    ticks >>= 8;
    PutMemB(data_segment, 0x6e, (BYTE)ticks);
    ticks >>= 8;
    PutMemB(data_segment, 0x6f, (BYTE)ticks);
}


void set_int(unsigned intno, BYTE *before, unsigned bcount,
             void (*routine)(void), BYTE *after, unsigned acount)
{
    PutMemW(memory,intno*4,pos);
    PutMemW(memory,intno*4+2,0xf000);

    if (before && bcount > 0)
    {
        memcpy(&BIOS_base[pos], before, bcount);
        pos += bcount;
    }
    
    if (routine)
    {
        bios_routine[intno] = routine;
        BIOS_base[pos++] = 0xf1; /* bios .... */
        BIOS_base[pos++] = 0xf1;
        BIOS_base[pos++] = intno;
        BIOS_base[pos++] = 0;
        BIOS_base[pos++] = 0;
        BIOS_base[pos++] = 0;
    }

    if (after && acount > 0)
    {
        memcpy(&BIOS_base[pos], after, acount);
        pos += acount;
    }
}


void set_vector(unsigned intno, unsigned seg, unsigned off)
{
    PutMemW(memory,4*intno,off);
    PutMemW(memory,4*intno+2,seg);
}


static void int_serial(void)
{
    D(printf("In serial. Function = 0x%02X\n", *bregs[AH]););

    CalcAll();
    switch(*bregs[AH])
    {
    default:
        *bregs[AH] = 0x80;
        D(printf("unsupported function\n"););
        break;
    }
}


static void int_printer(void)
{
    D(printf("In printer. Function = 0x%02X\n", *bregs[AH]););

    CalcAll();
    switch(*bregs[AH])
    {
    default:
        *bregs[AH] = 1;
        D(printf("unsupported function\n"););
        break;
    }
}


int code_to_num(unsigned code)
{
    unsigned ascii = scan_unshifted[code];

    if (ascii >= '0' && ascii <= '9')
        return ascii-'0';

    return -1;
}


void raw_to_BIOS(unsigned code, unsigned e0, unsigned *ascii, unsigned *scan)
{
    BYTE *tab;
    int tmp;

    if (KB_1 & ALT)
    {
        *ascii = (code == 0x39) ? 0x20 : 0;

        if (!e0 && (tmp = code_to_num(code)) >= 0)
        {
            num = num*10 + tmp;
            *scan = 0;
            return;
        }

        if (code == 0x1c && e0)
            *scan = 0xa6;
        else
            *scan  = scan_alt[code];
        return;
    }

    if (KB_1 & CTRL)
    {
        if (code == 0x39)
        {
            *ascii = 0x20;
            *scan = code;
        }
        else if (code >= 0x36 && code <= 0x58)
        {
            *ascii = 0;
            *scan = scan_ctrl_high[code - 0x36];
        }
        else if (code == 0x03)
        {
            *ascii = 0;
            *scan = code;
        }
        else
        {
            *ascii = scan_ctrl[code];
            if (e0 && code == 0x1c)
                *scan = 0xe0;
            else
                *scan = *ascii ? code : 0;
        }

        return;
    }

    tab = scan_unshifted;

    if (KB_1 & (SHIFT_L | SHIFT_R))
    {
        if (code >= 0x3b && code <= 0x44)
        {
            *ascii = 0;
            *scan = code+25;
            return;
        }
        if (code == 0x57 || code == 0x58)
        {
            *ascii = 0;
            *scan = code+0x30;
            return;
        }
        tab = scan_shift;
    }

    if (e0 && code == 0x36)
        *ascii = '/';
    else if (e0 && code == 0x1c)
    {
        *ascii = 0x0d;
        *scan = 0xe0;
        return;
    }
    else if (e0 || (code >= 0x47 && code <= 0x53 && !(KB_1 & NUMLOCK)))
        *ascii = 0;
    else
    {
        *ascii = tab[code];
        if ((KB_1 & CAPS) && *ascii >= 'a' && *ascii <= 'z')
            *ascii &= 0xdf;
    }
    
    if (code == 0x57 || code == 0x58)
        *scan = code+0x2e;
    else
        *scan = code;
}


static void put_key_in_buffer(unsigned ascii, unsigned scan)
{
    unsigned cstart, cend, bufstart, bufend, tmp;
    
    if (KB_2 & HOLD)
    {
        KB_2 &= ~HOLD;
        return;
    }
    
    cstart = GetCurKeyBufStart();
    cend = GetCurKeyBufEnd();
    bufstart = GetKeyBufStart();
    bufend = GetKeyBufEnd();
    
    tmp = cend;
    cend += 2;
    
    if (cend >= bufend)
        cend = bufstart;
    
    if (cend == cstart)
    {
        fprintf(stderr,"\a");
                    fflush(stderr);
    }
    else
    {
        D(printf("Writing ascii %02X scan %02X\n",ascii,scan););
        PutMemB(data_segment, tmp, ascii);
        PutMemB(data_segment, tmp+1, scan);
        SetCurKeyBufEnd(cend);
    }
}
 
   
void int9(void)
{
    unsigned code, state, ascii, scan;
    static unsigned e0_code = FALSE;

    CalcAll();

    code = *bregs[AL];

    *bregs[AH] = 0;

    D(printf("Read: %02X\n", code););
    state = code & 0x80;
    
    if ((code & 0xe0) == 0xe0)
    {
        e0_code = TRUE;
        return;
    
    }
    code &= 0x7f;

    switch (code)
    {
    case 0x2a:
        update_flag(KB_1, SHIFT_L, state);
        break;
    case 0x3a:
        if (!state)
            KB_1 ^= CAPS;
        break;
    case 0x1d:
        if (e0_code)
        {
            update_flag(modifiers, CTRL_R, state);
        }
        else
        {
            update_flag(KB_2, CTRL_L, state);
            update_flag(modifiers, CTRL_L, state);
        }
        update_flag(KB_1, CTRL, !(modifiers & (CTRL_R | CTRL_L)));
        break;
    case 0x38:
        if (e0_code)
        {
            update_flag(modifiers, ALT_R, state);
        }
        else
        {
            update_flag(KB_2, ALT_L, state);
            update_flag(modifiers, ALT_L, state);
        }
        update_flag(KB_1, ALT, !(modifiers & (ALT_R | ALT_L)));
        if (state)
        {
            num &= 0xff;
            if (num)
                put_key_in_buffer(num, 0);
        }
        else
            num = 0;
        break;
    case 0x36:
        if (!e0_code)
        {
            update_flag(KB_1, SHIFT_R, state);
            break;
        }

        /* else fall through.. */
    default:
        if (code == 0x52)
        {
            update_flag(KB_2, INSERT, state);
            if (state)
                KB_1 &= ~INSERT;
        }

        if (!state)
        {
            if (code == 0x46 && (KB_1 & CTRL))
            {
                unsigned cstart;

                D(printf("Control-Break!\n"););
                *bregs[AH] = 2;
                PutMemB(data_segment, 0x71, 1);
                cstart = GetCurKeyBufStart();   /* Clear keyboard buffer */
                SetCurKeyBufEnd(cstart);
                break;
            }

            if (code == 0x45 && (KB_1 & CTRL))
            {
                if (!(KB_2 & HOLD))
                {
                    D(printf("Pause!\n"););
                    *bregs[AH] = 1;
                    KB_2 |= HOLD;
                }
                break;
            }

            if (code == 0x53 && (KB_1 & CTRL) && (KB_1 & ALT))
            {
                D(printf("Got exit request!\n"););
                exit_emu();
                /* Not reached */
            }

            raw_to_BIOS(code, e0_code, &ascii, &scan);

            D(printf("%02X/%02X\n", ascii, scan););
            if (ascii != 0 || scan != 0)
            {
                if (!(KB_1 & ALT) && ascii == 0 && e0_code)
                    ascii = 0xe0;

                put_key_in_buffer(ascii, scan);
            }
        }
        break;
    }
    e0_code = FALSE;
    D(printf("Exiting key interrupt\n"););
}


static void int_keyboard(void)
{
    unsigned cstart, cend, bufstart, bufend;
    unsigned func = *bregs[AH];
    unsigned tmp;

    CalcAll();
    switch(func)
    {
    case 0x00:  /* Get non-extended key */
    case 0x10:  /* Get extended key */
        cstart = GetCurKeyBufStart();
        cend = GetCurKeyBufEnd();
        
        if (cstart != cend)
        {
            bufstart = GetKeyBufStart();
            bufend = GetKeyBufEnd();
            
            *bregs[AL] = GetMemB(data_segment, cstart);
            *bregs[AH] = GetMemB(data_segment, cstart+1);
            
            if (func == 0x00) {
                if (*bregs[AH] == 0xe0) {
                    *bregs[AH] = 0x1c;
		}
                else {
                    if (*bregs[AL] == 0xe0) {
                        *bregs[AL] = 0x00;
		    }
		}
	    }

            cstart += 2;
            
            if (cstart >= bufend)
                cstart = bufstart;
            
            SetCurKeyBufStart(cstart);
            
            D(printf("Cleared key %02X\n", *bregs[AL]););

            *bregs[CL] = 1;
            break;
        }
        else
            *bregs[CL] = 0;
        break;
    case 0x01:  /* Get non-extended status */
    case 0x11:  /* Get extended status */
        cstart = GetCurKeyBufStart();
        cend = GetCurKeyBufEnd();

        if (cstart == cend)
            ZF = 1;
        else
        {
            *bregs[AL] = GetMemB(data_segment, cstart);
            *bregs[AH] = GetMemB(data_segment, cstart+1);
            
            if (func == 0x01) {
                if (*bregs[AH] == 0xe0) {
                    *bregs[AH] = 0x1c;
		}
                else {
                    if (*bregs[AL] == 0xe0) {
                        *bregs[AL] = 0x00;
		    }
		}
	    }
            
            ZF = 0;
            D(printf("Returning key %02X from INT 16 1/11\n", *bregs[AL]););
        }

        break;
    case 0x02:  /* Get keyboard flags */
        *bregs[AL] = KB_1;
        break;
    case 0x03:  /* Set repeat rate */
    case 0x04:  /* Set key click */
        break;
    case 0x05:  /* Push scan code */
        cstart = GetCurKeyBufStart();
        cend = GetCurKeyBufEnd();
        bufstart = GetKeyBufStart();
        bufend = GetKeyBufEnd();
        
        tmp = cend;
        cend += 2;
        
        if (cend >= bufend)
            cend = bufstart;
        
        if (cend == cstart)
        {
            CF = 1;
            *bregs[AL] = 1;
        }
        else
        {
            PutMemB(data_segment, tmp, *bregs[CL]);
            PutMemB(data_segment, tmp+1, *bregs[CH]);
            SetCurKeyBufEnd(cend);
            CF = 0;
            *bregs[AL] = 0;
        }
        break;
    case 0x12:  /* Get enhanced keyboard flags */
        *bregs[AL] = KB_1;
        *bregs[AH] = (!(!(modifiers & CTRL_L)) +
                      (!(!(modifiers & ALT_L)) << 1) +
                      (!(!(modifiers & CTRL_R)) << 2 ) +
                      (!(!(modifiers & ALT_R)) << 3) +
                      (!(!(KB_2 & SCROLL)) << 4) +
                      (!(!(KB_2 & NUMLOCK)) << 5) +
                      (!(!(KB_2 & CAPS)) << 6) +
                      (!(!(KB_2 & SYSREQ)) << 7));
        break;
    default:
        D(printf("Warning: unimplemented INT 16 function %02X\n",func););
        CF = 1;
        break;
    }
}


static void int_extended(void)
{
    D(printf("In INT 0x15. Function = 0x%02X\n", *bregs[AH]););

    CalcAll();
    CF = 1;
    switch(*bregs[AH])
    {
    case 0x4f:
        break;
    case 0x85:
        CF = 0;
        break;
    case 0x10:
    case 0x41:
    case 0x64:
    case 0xc0:
    case 0xc1:
    case 0x84:
    case 0x87:
    case 0x88:
    case 0x89:
    case 0xd8:
        *bregs[AH] = 0x80;
        break;
    case 0x90:
    case 0x91:
        *bregs[AH] = 0;
        CF = 0;
        break;
    case 0xc2:
        *bregs[AH] = 1;
        break;
    default:
        printf("unimplement INT 15h function %02X\n",*bregs[AH]);
#ifdef DEBUG
        loc();
        exit_emu();
#else
        *bregs[AH] = 0x80;
#endif
        break;
    }
}


static void int_basic(void)
{
    CalcAll();
    D(printf("In BASIC\n"););
}


static void int_reboot(void)
{
    CalcAll();
    D(printf("In reboot\n"););
    exit_emu();
}


static void int_equipment(void)
{
    CalcAll();
    D(printf("In equipment\n"););
    *bregs[AL] = data_segment[0x10];
    *bregs[AH] = data_segment[0x11];
}


static void int_memory(void)
{
    CalcAll();
    D(printf("In memory\n"););
    *bregs[AL] = data_segment[0x13];
    *bregs[AH] = data_segment[0x14];
}


static UINT32 get_ticks_since_midnight(void)
{
    time_t curtime;
    struct tm *local;
    unsigned seconds;

    curtime = time(NULL);
    local = localtime(&curtime);

    seconds = (local->tm_hour * 3600) + (local->tm_min * 60) + local->tm_sec;
    return (TICKSPERSEC*seconds);
}


static void int_time(void)
{
    time_t curtime;
    struct tm *local;

    D(printf("In time. Function = 0x%02X\n", *bregs[AH]););

    CalcAll();
    switch(*bregs[AH])
    {
    case 0:     /* Get ticks */
        put_ticks(get_ticks_since_midnight());

        *bregs[DL] = GetMemB(data_segment, 0x6c);
        *bregs[DH] = GetMemB(data_segment, 0x6d);
        *bregs[CL] = GetMemB(data_segment, 0x6e);
        *bregs[CH] = GetMemB(data_segment, 0x6f);
        *bregs[AL] = GetMemB(data_segment, 0x70);

        CF = 0;

/*        D(printf("Returning %02X%02X%02X%02X\n", *bregs[CL], *bregs[CH],
                 *bregs[DH], *bregs[DL]);); */
        break;
    case 1:     /* Set ticks */

        PutMemB(data_segment, 0x6c, *bregs[DL]);
        PutMemB(data_segment, 0x6d, *bregs[DH]);
        PutMemB(data_segment, 0x6e, *bregs[CL]);
        PutMemB(data_segment, 0x6f, *bregs[CH]);
        break;
    case 2:     /* Get time */
        curtime = time(NULL);
        local = localtime(&curtime);
        
        *bregs[CH] = (local->tm_hour % 10) | ((local->tm_hour / 10) << 4);
        *bregs[CL] = (local->tm_min % 10) | ((local->tm_min / 10) << 4);
        *bregs[DH] = (local->tm_sec % 10) | ((local->tm_sec / 10) << 4);
        *bregs[DL] = local->tm_isdst > 0;
        CF = 0;

        put_ticks(get_ticks_since_midnight());
    case 3:     /* Set time */
        break;
    case 4:     /* Get date */
    {
        unsigned century, year, month;

        curtime = time(NULL);
        local = localtime(&curtime);

        century = 19 + (local->tm_year / 100);
        year = local->tm_year % 100;
        month = local->tm_mon+1;
        *bregs[CH] = (century % 10) | (century/10 << 4);
        *bregs[CL] = (year % 10) | (year/10 << 4);
        *bregs[DH] = (month % 10) | (month/10 << 4);
        *bregs[DL] = (local->tm_mday % 10) | (local->tm_mday/10 << 4);
        CF = 0;
    }
        break;
    case 0x5:
    case 0x6:
    case 0x7:
    case 0x08:
    case 0x09:
    case 0x0a:
    case 0x0b:
    case 0x35:  /* ??? Called by word perfect ??? */
    case 0x36:  /* ditto */
        CF = 1;
        break;
    default:
        printf("unimplemented INT 1Ah function %02X\n", *bregs[AH]);
#ifdef DEBUG
        loc();
        exit_emu();
#endif
        break;
    }
}


static DiskTab *get_disk_tab(int num)
{
	if (num < numfdisks)
		return &fdisk[num];
	
	num -= 0x80;
	if ((num < numhdisks) && (num >= 0))
		return &hdisk[num];
    
	return NULL;
}


static int disk_seek(DiskTab *disk, int cylinder, int head, int sector)
{
    unsigned pos;

    if (head > disk->heads || cylinder > disk->cylinders ||
        sector > disk->sectors)
    {
        CF = 1;
        *bregs[AH] = diskerror = 0x4;  /* Sector not found */
        return -1;
    }
    pos = ((cylinder*disk->heads + head)*disk->sectors + sector) << 9;
    if (pos != lseek(disk->fd, pos, 0))
    {
        CF = 1;
        *bregs[AH] = diskerror = 0x4;  /* Sector not found */
        return -1;
    }
    return 0;
}


static void int_disk(void)
{
    int head, sector, cylinder, res, num;
    BYTE *buffer;
    DiskTab *disk;

    res = 0;

    CalcAll();

    switch(*bregs[AH])
    {
    case 0:
        D(printf("Initialise disk 0x%02X\n",*bregs[DL]););
        CF = 0;
        break;
    case 1:  /* Get last error */
        D(printf("Get last disk error\n"););
        *bregs[AH] = 0;
        *bregs[AL] = diskerror;
        break;
    case 2:  /* Read sector */
        disk = get_disk_tab(*bregs[DL]);
        if (!disk)
        {
            CF = 1;
            *bregs[AH] = diskerror = 0xaa;  /* Drive not ready... */
            break;
        }
        head = *bregs[DH];
        cylinder = *bregs[CH] + ((*bregs[CL] & 0xc0) << 2);
        sector = (*bregs[CL] & 0x3f) -1;
        buffer = &c_es[ChangeE(wregs[BX])];
        D(printf("DISK 0x%02X (%s) read [h%d,s%d,t%d](%d)->%04X:%04X\n",
                 *bregs[DL], disk->name, head, sector, cylinder, *bregs[AL],
                 sregs[ES], ChangeE(wregs[BX])););
        if (disk_seek(disk, cylinder, head, sector))
            break;

        num = *bregs[AL] << 9;
        res = read(disk->fd, buffer, num);
        if (res & 0x1ff)
        {
            CF = 1;
            *bregs[AH] = diskerror = 0x2;  /* Sector corrupt */
            break;
        }
        *bregs[AH] = diskerror = 0;
        *bregs[AL] = res >> 9;
        CF = 0;
        break;
    case 3:  /* Write sector */
        disk = get_disk_tab(*bregs[DL]);
        if (!disk)
        {
            CF = 1;
            *bregs[AH] = diskerror = 0xaa;  /* Drive not ready... */
            break;
        }
        head = *bregs[DH];
        cylinder = *bregs[CH] + ((*bregs[CL] & 0xc0) << 8);
        sector = (*bregs[CL] & 0x3f) -1;
        buffer = &c_es[ChangeE(wregs[BX])];
        D(printf("DISK 0x%02X (%s) read [h%d,s%d,t%d](%d)->%04X:%04X\n",
                 *bregs[DL], disk->name, head, sector, cylinder, *bregs[AL],
                 sregs[ES], ChangeE(wregs[BX])););
        if (disk_seek(disk, cylinder, head, sector))
            break;

        num = *bregs[AL] << 9;
        res = write(disk->fd, buffer, num);
        if (res & 0x1ff)
        {
            CF = 1;
            *bregs[AH] = diskerror = 0x2;  /* Sector corrupt */
            break;
        }
        *bregs[AH] = diskerror = 0;
        *bregs[AL] = res >> 9;
        CF = 0;
        break;
    case 4: /* Test disk */
        D(printf("Testing disk 0x%02X\n",*bregs[DL]););
        disk = get_disk_tab(*bregs[DL]);
        if (!disk)
        {
            CF = 1;
            *bregs[AH] = diskerror = 0xaa;  /* Drive not ready... */
            break;
        }
        head = *bregs[DH];
        cylinder = *bregs[CH] + ((*bregs[CL] & 0xc0) << 8);
        sector = (*bregs[CL] & 0x3f) -1;
        buffer = &c_es[ChangeE(wregs[BX])];
        if (disk_seek(disk, cylinder, head, sector))
            break;

        *bregs[AH] = diskerror = 0;
        *bregs[AL] = res >> 9;
        CF = 0;
        break;
    case 5: /* Format disc track */
    case 6: /* Format and set bad sector flags */
    case 7: /* Format drive starting from given track */
        CF = 0;
        break;
    case 8: /* Get disk params */
        D(printf("Get disk params 0x%02X\n",*bregs[DL]););
        disk = get_disk_tab(*bregs[DL]);
        if (disk)
        {
            if (*bregs[DL] < 0x80)
            	switch(disk->sectors)
            	{
            	case 9:
                	*bregs[BL] = (disk->cylinders == 80) ? 3 : 1;
                	break;
            	case 15:
                	*bregs[BL] = 2;
                	break;
            	case 18:
                	*bregs[BL] = 4;
                	break;
            	}
            else
            	*bregs[BL] = 0; /* Is this right? */

            *bregs[CH] = (disk->cylinders - 1) & 0xff;
            *bregs[CL] = (disk->sectors - 1) | (((disk->cylinders - 1)
                                                 & 0x300) >> 2);
            *bregs[DH] = disk->heads - 1;
            *bregs[DL] = (*bregs[DL] < 0x80) ? numfdisks : numhdisks;
            *bregs[AL] = 0;
            CF = 0;
        }
        else
        {
            CF = 1;
            *bregs[AH] = diskerror = 0x1;
            wregs[DX] = 0;
            wregs[CX] = 0;
        }
        break;
    case 0x15:  /* Get disk type */
        D(printf("Get disk type 0x%02X\n",*bregs[DL]););
        disk = get_disk_tab(*bregs[DL]);
        if (disk)
        {
            CF = 0;
            if (*bregs[DL] >= 0x80)
            {
                num = disk->cylinders * disk->sectors * disk->heads;
                *bregs[DL] = num & 0xff;
                *bregs[DH] = (num & 0xff00) >> 8;
                *bregs[CL] = (num & 0xff0000) >> 16;
                *bregs[CH] = (num & 0xff000000) >> 24;
                *bregs[AH] = 3;
            }
            else
                *bregs[AH] = 1;
        }
        else
        {
            CF = 1;
            *bregs[AH] = 0;
        }
        break;
    default:
        printf("Unimplemented INT 13h function %02X\n",*bregs[AH]);
#ifdef DEBUG
        loc();
        exit_emu();
#endif
        break;
        }

    D(if (CF) printf("Operation failed\n"););
}

#if !defined(DISABLE_MFS)
static int call_mfs(int (*routine)(void))
{
    int ret;

    vm86s.regs.eax = ChangeE(wregs[AX]);
    vm86s.regs.ebx = ChangeE(wregs[BX]);
    vm86s.regs.ecx = ChangeE(wregs[CX]);
    vm86s.regs.edx = ChangeE(wregs[DX]);
    vm86s.regs.esi = ChangeE(wregs[SI]);
    vm86s.regs.edi = ChangeE(wregs[DI]);
    vm86s.regs.ebp = ChangeE(wregs[BP]);
    vm86s.regs.esp = ChangeE(wregs[SP]);
    vm86s.regs.cs  = sregs[CS];
    vm86s.regs.ds  = sregs[DS];
    vm86s.regs.es  = sregs[ES];
    vm86s.regs.ss  = sregs[SS];
    vm86s.regs.eip = ip;
    vm86s.regs.eflags = CompressFlags();

#if 0
printf("--------------------\n");
print_regs();
printf("eflags = %X\n", vm86s.regs.eflags);
printf("\n");
fflush(stdout);
#endif
	
    ret = routine();


    wregs[AX] = ChangeE(vm86s.regs.eax);
    wregs[BX] = ChangeE(vm86s.regs.ebx);
    wregs[CX] = ChangeE(vm86s.regs.ecx);
    wregs[DX] = ChangeE(vm86s.regs.edx);
    wregs[SI] = ChangeE(vm86s.regs.esi);
    wregs[DI] = ChangeE(vm86s.regs.edi);
    wregs[BP] = ChangeE(vm86s.regs.ebp);
    wregs[SP] = ChangeE(vm86s.regs.esp);
    
    sregs[CS] = (WORD)vm86s.regs.cs;
    c_cs = SegToMemPtr(CS);
    sregs[DS] = (WORD)vm86s.regs.ds;
    c_ds = SegToMemPtr(DS);
    sregs[SS] = (WORD)vm86s.regs.ss;
    c_stack = c_ss = SegToMemPtr(SS);
    sregs[ES] = (WORD)vm86s.regs.es;
    c_es = SegToMemPtr(ES);

    ip = (WORD)vm86s.regs.eip;
    ExpandFlags(vm86s.regs.eflags);

#if 0
print_regs();
printf("eflags = %X\n", vm86s.regs.eflags);
fflush(stdout);
#endif

    return ret;
}
#endif

static void int_doshelper(void)
{
    CalcAll();
    printf("doshelper AX=%X BX=%x\n", ChangeE(wregs[AX]), ChangeE(wregs[BX]));fflush(stdout);
    fflush(stdout);
    switch(*bregs[AL])
    {
    case 0x00: /* Check for Linux DOSEMU. We pretend to be V1.0 */
        wregs[AX] = ChangeE(0xAA55);
        wregs[BX] = ChangeE(0x0100);
        break;
#if !defined(DISABLE_MFS)
    case 0x20: /* MFS entry */
        call_mfs(mfs_intentry);
        break;
#endif
    default:
        CF = 1;
    }
}


static void int_2f(void)
{
#if !defined(DISABLE_MFS)
    unsigned tmp,tmp2;

    D(printf("In INT 0xe8 AH = 0x%02X  AL = 0x%02X\n",*bregs[AH],*bregs[AL]););

    CalcAll();
    switch(*bregs[AH])
    {
    case 0x11:
        tmp = ChangeE(wregs[SP]);
        tmp2 = tmp + 6;
        wregs[SP] = ChangeE(tmp2);
        ZF = call_mfs(mfs_redirector) ? 0 : 1;
        wregs[SP] = ChangeE(tmp);
        break;
    default:
        break;
    }
#endif
}


static void setup(void)
{
    static BYTE int8code[] =
    {
        30,80,82,184,64,0,142,216,161,108,0,139,22,110,0,5,
        1,0,131,210,0,131,250,24,114,14,61,176,0,114,9,49,
        192,49,210,198,6,112,0,1,163,108,0,137,22,110,0,176,
        32,230,32,251,205,28,90,88,31,207
    };
    static BYTE afterint[] = { 0xfb, 0xca, 0x02, 0x00 };
    static BYTE before_int16[] = { 0x51, 0xb9, 0x01, 0x00, 0xfb };
    static BYTE after_int16[] = { 0xe3, 0xf8, 0x59, 0xca, 0x02, 0x00 };
    static BYTE int9bcode[] =
    {
        30,80,180,79,228,96,205,21,114,7,176,32,230,32,251,235,57
    };
    static BYTE int9acode[] =
    {
        176,32,230,32,251,8,228,116,42,254,204,117,14,184,64,0,142,
        216,246,6,24,0,8,117,249,235,24,254,204,117,4,205,27,235,16,
        254,204,117,4,205,5,235,8,254,204,117,4,180,133,205,21,88,31,207
    };
    static BYTE inte7code[] =
    {
        0x06,0x57,0x50,0xb8,0x0c,0x12,0xcd,0x2f,0x58,0x5f,0x07,0xcf
    };

    unsigned equip = 0;
    int i;

    memory[0xf0000+0xfffe] = SYSTEMID;

    EMS_Setup();
    
    if (numfdisks > 0)
        equip |= (1 | ((numfdisks-1) << 6));
    equip |= mono ? 0x30 : 0x20;


    SetCurKeyBufStart(0x001e);
    SetCurKeyBufEnd(0x001e);
    SetKeyBufStart(0x001e);
    SetKeyBufEnd(0x003e);

    PutMemW(data_segment, 0x10, equip);
    PutMemW(data_segment, 0x13, 640);
    PutMemB(data_segment, 0x3e, 0xff);
    PutMemB(data_segment, 0x3f, 0);
    PutMemB(data_segment, 0x40, 0);
    PutMemB(data_segment, 0x41, 0);
    PutMemW(data_segment, 0x72, 0x1234);
    PutMemW(data_segment, 0x75, numhdisks);
    PutMemW(data_segment, 0x96, 16);            /* 101/102 keyboard */
    PutMemB(data_segment, 0x17, NUMLOCK);

    PutMemB(BIOS_base, 0x00, 0xcf);  /* IRET */

    PutMemB(BIOS_base, 0xfff0, 0xcd);  /* INT 19 */
    PutMemB(BIOS_base, 0xfff1, 0x19);
    PutMemB(BIOS_base, 0xfff2, 0xeb);  /* jmp $-2 */
    PutMemB(BIOS_base, 0xfff3, 0xfe);

    for (i = 0; i < 256; i++)           /* Reset all int vectors */
    {
        if (i >= 0x60 && i <= 0x6f)
            continue;
        else
        {
            PutMemW(memory, i*4, 0);
            PutMemW(memory, i*4+2, 0xf000);
        }
    }

#define INSTALL_INT(intno, before, handler, after) \
	set_int(intno, before, before?(sizeof before):0, handler, after, \
		after?(sizeof after):0)

    /*         Int_no Code_before       Handler         Code_after */
    
    INSTALL_INT(0x08, int8code,		NULL,		NULL);
    INSTALL_INT(0x09, int9bcode,	int9,		int9acode);
    INSTALL_INT(0x11, NULL,		int_equipment,	afterint);
    INSTALL_INT(0x12, NULL,		int_memory,	afterint);
    INSTALL_INT(0x13, NULL,		int_disk,	afterint);
    INSTALL_INT(0x14, NULL,		int_serial,	afterint);
    INSTALL_INT(0x15, NULL,		int_extended,	afterint);
    INSTALL_INT(0x16, before_int16,	int_keyboard,	after_int16);
    INSTALL_INT(0x17, NULL,		int_printer,	afterint);
    INSTALL_INT(0x18, NULL,		int_basic,	afterint);
    INSTALL_INT(0x19, NULL,		int_reboot,	afterint);
    INSTALL_INT(0x1a, NULL,		int_time,	afterint);
    
    INSTALL_INT(0x4b, NULL,		int_ems,	afterint);

    INSTALL_INT(0xe6, NULL,		int_doshelper,	afterint);
    INSTALL_INT(0xe7, inte7code,	NULL,		NULL);
    INSTALL_INT(0xe8, NULL,		int_2f,		afterint);
   
#undef INSTALL_INT

    put_ticks(get_ticks_since_midnight());

    memcpy(BIOS_base+0xfff5,BIOSDATE,sizeof BIOSDATE);
    memcpy(BIOS_base+0xe000,BIOSCOPYRIGHT, sizeof BIOSCOPYRIGHT);
}


void init_bios(void)
{
    int i;
#ifdef BOOT
    DiskTab *boot;
#endif

    if (numhdisks)
        for (i = 0; i < numhdisks; i++)
            open_drive(hdisk, i);

    for (i = 0; i < numfdisks; i++)
        open_drive(fdisk, i);

    BIOS_base = &memory[0xf0000];
    data_segment = &memory[0x400];

    setup();

#ifdef BOOT
    boot = get_disk_tab(bootdisk);

    if (boot == NULL || read(boot->fd, &memory[0x7c00], 512) != 512)
    {
        fprintf(stderr, "Cannot boot from %s :",boot->name);
        if (boot)
            perror(NULL);
        exit(1);
    }
    
    sregs[CS] = sregs[ES] = sregs[DS] = sregs[SS] = 0;
    *bregs[DL] = bootdisk;
    ip = 0x7c00;
    IF = 1;
#endif

#ifdef DEBUGGER
    signal(SIGINT, (void *)debug_breakin);
#else
    signal(SIGINT, (void *)exit_emu);
#endif
    signal(SIGTERM, (void *)exit_emu);

}

void open_drive(DiskTab disktab[], int drive)
{
    disktab[drive].mounted = 1;
    if ((disktab[drive].fd = open(disktab[drive].name, O_RDWR)) < 0)
    {
	if ((disktab[drive].fd = open(disktab[drive].name, O_RDONLY)) < 0)
	{
	    printf("Unable to open %s at all\n", disktab[drive].name);
	    disktab[drive].mounted = 0;
	    disktab[drive].fd = open("/dev/zero", O_RDWR);
	}
    }
}

char *set_drive(DiskTab disktab[], int drive, char *device, int sectors, int cylinders, int heads)
{
    if ((disktab[drive].name = strdup(device)) == NULL)
    {
        fprintf(stderr, "Insufficient available memory\n");
        exit(1);
    }
    disktab[drive].sectors = sectors;
    disktab[drive].cylinders = cylinders;
    disktab[drive].heads = heads;
    
    return NULL;
}

char *set_floppy_drive(int drive, char *device, int sectors, int cylinders, int heads)
{
    char s[8];
    char *status;
    
    status = set_drive(fdisk, drive, device, sectors, cylinders, heads);
    if (status)
    	return(status);
    strncpy(s, fdisk[drive].name, 5);
    s[5] = '\0';
    if (strcmp(s, "/dev/")==0)
        fdisk[drive].removable = 1;
    else
        fdisk[drive].removable = 0;
        
    return NULL;
}

char *set_hard_drive(int drive, char *device, int sectors, int cylinders, int heads)
{
    char *status;
    
    status = set_drive(hdisk, drive, device, sectors, cylinders, heads);
    if (status)
    	return(status);
    hdisk[drive].removable = 0;
    
    return NULL;
}

char *set_num_fdrives(int num)
{
	if ((num<0) || (num>2))
		return "Can't have this many floppy drives";
	numfdisks = num;
	return NULL;
}

char *set_num_hdrives(int num)
{
	if ((num<0) || (num>2))
		return "Can't have this many hard drives";
	numhdisks = num;
	return NULL;
}

void bios_off(void)
{
    int i;
    
    for (i = 0; i < numfdisks; i++)
        close(fdisk[i].fd);

	if (numhdisks)
		for (i =0; i < numhdisks; i++)
			close(hdisk[i].fd);
}

