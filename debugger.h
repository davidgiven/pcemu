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

#ifndef DEBUGGER_H
#define DEBUGGER_H

#define D_INT 0
#define D_TRACE 1

volatile int in_debug;
void call_debugger(int);
int debug_breakin(int);
void print_regs(void);

extern volatile int breakpoint;
extern volatile int running;
#endif

#endif
