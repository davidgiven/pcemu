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

#ifndef XSTUFF_H
#define XSTUFF_H

void start_X(void);
void end_X(void);
void process_Xevents(void);
void flush_X(void);

void copy(unsigned, unsigned, unsigned, unsigned, unsigned, unsigned);
void draw_char(unsigned, unsigned, char, BYTE);
void draw_line(unsigned, unsigned, char *, unsigned, BYTE);
void clear_screen(void);
void window_size(unsigned, unsigned);

void new_cursor(int, int);
void put_cursor(unsigned, unsigned);
void unput_cursor(void);

extern int graphics,blink;
#endif
