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

#ifndef HARDWARE_H
#define HARDWARE_H

#include "mytypes.h"

#define PIC_TIMER    1
#define PIC_KEYBOARD 2

#define TICKSPERSEC (1193180.0/65536.0)

int port60_buffer_ok(int);
void put_scancode(BYTE *, int);
void init_timer(void);

void disable(void);
void enable(void);

void starttimer(void);
void stoptimer(void);

BYTE read_port(unsigned);
void write_port(unsigned, BYTE);

#endif
