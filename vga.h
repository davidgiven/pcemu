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

/* This is VGA.H  It contains definitions for the VGA BIOS functions */


#ifndef VGA_H
#define VGA_H

#define VIDEOMEM    (256*1024)

extern int mono;

void init_vga(void);
void vga_off(void);

#endif
