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

/* This is VGAHARD.H  It contains definitions for the VGA hardware */


#ifndef VGAHARD_H
#define VGAHARD_H

#include "mytypes.h"

void new_screen(unsigned, unsigned, BYTE *);
void pcemu_refresh(void);
void update_display(void);
void displaychar(void);
void move_cursor(unsigned, unsigned);
void set_cursor_height(int, int);
void init_hardware(void);
void finish_hardware(void);
void copy_text(unsigned, unsigned, unsigned, unsigned, unsigned, unsigned);

void vga_interrupt(void);

char *set_update_rate(int);
char *set_cursor_rate(int);

#endif
