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

/* This is HARDWARE.C It contains stuff about the PC hardware (chips etc) */

#include "global.h"

#include <stdio.h>
#include <sys/time.h>
#include <signal.h>

#include "hardware.h"
#include "cpu.h"
#include "vgahard.h"

#ifdef DEBUGGER
#    include "debugger.h"
#endif

static struct itimerval timer;    

static volatile int disable_int;
static volatile int timer_blocked = 1;

#define PORT60SIZE 5

static BYTE port60buffer[PORT60SIZE];
static int port60start, port60end;

static unsigned PIC_inservice;
static unsigned PIC_irq;
static unsigned PIC_mask = ~(PIC_TIMER|PIC_KEYBOARD);

static unsigned timer_tmp;

static unsigned VGA_status;

static void PIC_interrupt(void)
{
    if (PIC_inservice)
    {
        D2(printf("PIC still blocked\n"););
        return;
    }

    if (PIC_irq & ~PIC_mask & PIC_TIMER)
    {
        D2(printf("INTR: timer\n"););
        PIC_inservice = PIC_TIMER;
        PIC_irq &= ~PIC_TIMER;
        if (IF)
            int_pending = 8;
        else
        {
            D2(printf("INTR blocked: IF disabled\n"););
            int_blocked = 8;
        }
    }
    else if (PIC_irq & ~PIC_mask & PIC_KEYBOARD)
    {
        D2(printf("INTR: keyboard\n"););
        PIC_inservice = PIC_KEYBOARD;
        PIC_irq &= ~PIC_KEYBOARD;
        if (IF)
            int_pending = 9;
        else
        {
            D2(printf("INTR blocked: IF disabled\n"););
            int_blocked = 9;
        }
    }
}


static void PIC_flagint(unsigned interrupt)
{
    disable();
    D2(printf("IRQ %02X\n", interrupt););
    PIC_irq |= interrupt;
    enable();
}


void PIC_EOI(void)
{
    disable();

    if (PIC_inservice & PIC_KEYBOARD)
    {
        if (++port60start >= PORT60SIZE)
            port60start = 0;

        if (port60start != port60end)
            PIC_irq |= PIC_KEYBOARD;
    }


    PIC_inservice = 0;
	PIC_interrupt();
    enable();
}


int port60_buffer_ok(int count)
{
    int tmp = port60end;

    for (; count > 0; count--)
    {
        if (++tmp >= PORT60SIZE)
            tmp = 0;

        if (tmp == port60start)
            return FALSE;
    }

    return TRUE;
}


void put_scancode(BYTE *code, int count)
{
    for (; count > 0; count--)
    {
        port60buffer[port60end] = *code++;

        if (++port60end >= PORT60SIZE)
            port60end = 0;
    }

    PIC_flagint(PIC_KEYBOARD);

    D2(
      {
          int tmp;

          printf("INT9 signalled\n");
          printf("Port60 buffer: ");

          tmp = port60start;
          while (tmp != port60end)
          {
              printf("%02X ", port60buffer[tmp]);
              if (++tmp >= PORT60SIZE)
                  tmp = 0;
          }
          printf("\n");
      }
    )
}


static BYTE read_port60(void)
{
    BYTE ret;
    static BYTE lastread = 0;

    disable();

    if (port60start == port60end)
        ret = lastread;
    else
        lastread = ret = port60buffer[port60start];
        
    enable();

    return ret;
}

#define TIMER_MULT 4

static volatile int int8_counter = TIMER_MULT;

static int timer_handler(int sig)
{

#ifdef DEBUGGER
    if (in_debug)
        return 0;
#endif

    if (disable_int)
    {
        D2(printf("Timer called when disabled\n"););
        timer_blocked++;
        return 0;
    }

    disable();

    vga_interrupt();

    timer_tmp++;

    if (--int8_counter == 0)
    {
        PIC_flagint(PIC_TIMER);
        int8_counter = TIMER_MULT;
    }

    PIC_interrupt();

    enable();
    return 0;
}


#ifndef SA_RESTART
#define SA_RESTART 0
#endif

void init_timer(void)
{
    struct sigaction sa;

    sa.sa_handler = (void *)timer_handler;
    sa.sa_flags = SA_RESTART;
    sigemptyset(&sa.sa_mask);

    sigaction(SIGALRM, &sa, NULL);

    timer.it_interval.tv_sec = timer.it_value.tv_sec = 0;
    timer.it_interval.tv_usec = timer.it_value.tv_usec =  
        (long)(1000000.0 /((float)TIMER_MULT*TICKSPERSEC));

    D2(printf("Set timer to %ld microseconds\n", timer.it_interval.tv_usec););
    setitimer(ITIMER_REAL, &timer, NULL);
}


void stoptimer(void)
{
    timer.it_interval.tv_sec = timer.it_value.tv_sec = 
    timer.it_interval.tv_usec = timer.it_value.tv_usec =  0;
    setitimer(ITIMER_REAL, &timer, NULL);
    signal(SIGALRM, SIG_IGN);
}


void starttimer(void)
{
    init_timer();
}


void disable(void)
{
    if (disable_int == 0)
        timer_blocked = 0;
    disable_int++;
}


void enable(void)
{
    if (disable_int > 0)
        disable_int--;

    if (disable_int == 0)
        while (timer_blocked > 0)
        {
            timer_blocked--;
            timer_handler(SIGALRM);
        }
}


static int PIT_toggle = 1;

BYTE read_port(unsigned port)
{
    BYTE val;
    static unsigned timer_tmp;

    switch(port)
    {
    case 0x21:
        val = PIC_mask;
        break;
    case 0x40:
    case 0x41:
    case 0x42:
        disable();
        if ((PIT_toggle = !PIT_toggle) == 0)
            val = timer_tmp & 0xff;
        else
        {
            val = (timer_tmp & 0xff00) >> 8;
            timer_tmp++;
        }
        enable();
        break;
    case 0x60:
        val = read_port60();
        break;
    case 0x61:
        val = 0xd0;
        break;
    case 0x3da:
        val = VGA_status;
        VGA_status ^= 0x9;
        break;
    case 0x2fd:
	val = 0xff;
	break;
    default:
        val = 0;
        break;
    }
    
    D(printf("Reading 0x%02X from port 0x%04X\n", val, port););
    return val;
}


void write_port(unsigned port, BYTE value)
{
    switch(port)
    {
    case 0x20:
        if (value == 0x20)
        {
            PIC_EOI();
            return;
        }
        break;
    case 0x21:
        disable();
        PIC_mask = value;
        enable();
        break;
    case 0x43:
        PIT_toggle = 1;
        break;
    default:
        break;
    }
    D(printf("Writing 0x%02X to port 0x%04X\n", value, port););

}


