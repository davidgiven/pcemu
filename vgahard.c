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

/* This is VGA.C It contains stuff about the VGA hardware adapter */

#include "global.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>

#include "vga.h"
#include "vgahard.h"
#include "video.h"
#include "hardware.h"

#ifndef CURSORSPEED
#define CURSORSPEED   30
#endif

#ifndef UPDATESPEED
#define UPDATESPEED 5
#endif

#define BUFFER_DELAY 5

static BYTE *screen_mem;     /* pointer to screen memory */

extern int mono;             /* TRUE if on monochrome screen */

static unsigned cx,cy;       /* cursor position */
static int cs,ce;            /* cursor start/end (height) */
static int height, width;        /* screen size */

static volatile int busy = 0;

static volatile int cursoron = FALSE;

static unsigned tick_count;
static unsigned chars_printed;
static unsigned cursor_printed;

static volatile int refresh_count = 1;
static volatile int cursor_count = 1;

static int refresh_speed = UPDATESPEED;
static int cursor_speed = CURSORSPEED;

static BYTE *disp_buffer;
static BYTE *frame_buffer;


static void toggle_cursor(void)
{
    busy = TRUE;
    cursoron = !cursoron;

    if (cursoron)
        put_cursor(cx, cy);
    else
        unput_cursor();

    busy = FALSE;
}


static void blank_cursor(void)
{
    if (cursoron)
    {
        cursoron = FALSE;
        unput_cursor();
    }
}


static void draw_cursor(void)
{
    if (!cursoron)
    {
        if (cx < width && cy < height && ce > cs)
        {
            cursoron = TRUE;
            put_cursor(cx,cy);
        }
    }
}


void move_cursor(unsigned x, unsigned y)
{
    static unsigned last_count;

    busy = TRUE;
    cx = x;
    cy = y;

    if (cursor_printed >= 1 && tick_count >= last_count && 
        tick_count < last_count+BUFFER_DELAY)
    {
        last_count = tick_count;
    }
    else
    {
        last_count = tick_count;
        cursor_printed++;
        
        blank_cursor();
        draw_cursor();
    }

    busy = FALSE;
}


void copy_text(unsigned x1, unsigned y1, unsigned x2, unsigned y2,
               unsigned nx, unsigned ny)
{
    unsigned w,h, bwidth, bheight;
    BYTE *src,*dest;

    busy = TRUE;
    blank_cursor();
    copy(x1,y1,x2,y2,nx,ny);

    bwidth = x2-x1+1;
    bheight = y2-y1+1;

    if ((ny > y1 && nx >= x1) || (ny >= y1 && nx < x1))
    {
        dest = &frame_buffer[2*((ny+bheight)*width+(nx+bwidth))];
        src = &frame_buffer[2*(y2*width+x2)];

        for (h = bheight; h > 0; h--)
        {
            for (w = 2*bwidth; w > 0; w--)
                *--dest = *--src;
            
            dest -= (width-bwidth);
            src -= (width-bwidth);
        }
    }
    else
    {
        dest = &frame_buffer[2*(ny*width+nx)];
        src = &frame_buffer[2*(y1*width+x1)];

        for (h = bheight; h > 0; h--)
        {
            for (w = 2*bwidth; w > 0; w--)
                *dest++ = *src++;
            
            dest += (width-bwidth);
            src += (width-bwidth);
        }
    }       

    draw_cursor();
    busy = FALSE;
}


void set_cursor_height(int s, int e)
{
    busy = TRUE;
    cs = s;
    ce = e;
    blank_cursor();
    new_cursor(s, e);
    draw_cursor();
    busy = FALSE;
}


void new_screen(unsigned w, unsigned h, BYTE *mem)
{
    busy = TRUE;

    screen_mem = mem;
    if (width != w || height != h)
    {
        if (width != w)
        {
            if (disp_buffer)
                free(disp_buffer);
            
            width = w;
            disp_buffer = (BYTE *)malloc(width+1);
            disp_buffer[width] = '\0';
        }
        
        if (frame_buffer)
            free(frame_buffer);
        
        height = h;
        frame_buffer = (BYTE *)malloc(width*height*2+1);
        memset(frame_buffer, 0, width*height*2);
            
        window_size(width, height);
    }
    busy = FALSE;
}


void update_display(void)
{
    register BYTE *scr = frame_buffer;
    register BYTE *mem = screen_mem;
    unsigned row,col,startcol;
    BYTE *end;
    BYTE *bufptr;
    BYTE attr;

    busy = TRUE;

    end = &scr[width*height*2];
    *end = mem[width*height*2]+1;

    for(;;)
    {
        while (*scr == *mem)
        {
            scr++;
            mem++;
        }

        if (scr == end)
            break;
        
        blank_cursor();

        col = (scr-frame_buffer) % (2*width);
        row = (scr-frame_buffer) / (2*width);

        bufptr = disp_buffer;
        startcol = col >> 1;

        if (col & 1)
        {
            attr = *mem;
            col--;
            scr--;
            mem--;
        }
        else
            attr = mem[1];
        
        do
        {
            *bufptr++ = (*scr++ = *mem) ? *mem : 0x20;
            *scr++ = attr;
            mem += 2;
            col += 2;
        } while((*scr != *mem || scr[1] != mem[1]) && mem[1] == attr &&
                col < 2*width);
        
        draw_line(startcol, row,(char *)disp_buffer, bufptr-disp_buffer, attr);
    }

    if (cursor_speed <= 0)
        draw_cursor();
    busy = FALSE;
}
           

static void update_displayline(unsigned line)
{
    unsigned x, startcol;
    BYTE *scr, *buf, *mem, attr;

    mem = &screen_mem[line*width*2];
    scr = &frame_buffer[line*width*2];

    busy = TRUE;
    blank_cursor();

    x = 0;
    while (x < width)
    {
        buf = disp_buffer;
        attr = mem[1];

        startcol = x;
        do
        {
            *buf++ = (*scr++ = *mem) ? *mem : 0x20;
            mem++;
            *scr++ = *mem++;
            x++;
        } while (x < width && mem[1] == attr);

        draw_line(startcol, line, (char *)disp_buffer, buf-disp_buffer, attr);
    }   
    draw_cursor();
    busy = FALSE;
}


void pcemu_refresh(void)
{
    unsigned h;

    for (h = 0; h < height; h++)
        update_displayline(h);
}


void displaychar(void)
{
    BYTE *mem;
    static unsigned last_count;
    
    if (chars_printed >= 1 && tick_count >= last_count && 
        tick_count < last_count+BUFFER_DELAY)
    {
        last_count = tick_count;
        return;
    }

    last_count = tick_count;
    chars_printed++;
    
    mem = &screen_mem[2*(cx+cy*width)];

/*    frame_buffer[2*(cx+cy*width)] = *mem;
    frame_buffer[2*(cx+cy*width)+1] = mem[1];
*/

    busy = TRUE;
/*    blank_cursor(); */
    update_display();
/*    draw_char(cx, cy, *mem, mem[1]); */
    busy = FALSE;
}


void init_hardware(void)
{
    busy = TRUE;
    set_cursor_height(14,15);
    busy = FALSE;
}


void finish_hardware(void)
{
    busy = TRUE;
    end_video();
    busy = FALSE;
}


void vga_interrupt(void)
{
    tick_count++;
    if (!busy)
    {
        process_events();
        if (cursor_speed > 0 && --cursor_count == 0)
        {
            cursor_count = cursor_speed;
            toggle_cursor();
        }
        if (--refresh_count == 0)
        {
            refresh_count = refresh_speed;
            chars_printed = 0;
            cursor_printed = 0;
            update_display();
            flush_video();
        }
    }
}


char *set_update_rate(int frames)
{
    if (frames > 0)
    {
        refresh_speed = frames;
        return NULL;
    }

    return "Invalid update speed";
}


char *set_cursor_rate(int frames)
{
    cursor_speed = frames;
    return NULL;
}



