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

/* This is BIOS.H  It contains definitions for the BIOS functions */


#ifndef BIOS_H
#define BIOS_H

#include "mytypes.h"

#define BOOT

void (*bios_routine[256])(void);

void init_bios(void);
void init_timer(void);
void bios_off(void);
void set_int(unsigned, BYTE *, unsigned, void (*routine)(void), BYTE *, unsigned);

void int_ems(void);
void EMS_Setup(void);

void disable(void);
void enable(void);

void put_scancode(BYTE *code, int count);

BYTE read_port60(void);

void loc(void);

char *set_boot_file(char *);
char *set_boot_type(int);

#define MAXHDISKS 2
#define MAXFDISKS 2

typedef struct
{
    char *name;
    unsigned sectors;
    unsigned cylinders;
    unsigned heads;
    int fd;
    char removable;
    char mounted;
} DiskTab;

extern DiskTab fdisk[MAXFDISKS];
extern DiskTab fdisk[MAXFDISKS];

extern char *set_num_fdrives(int);
extern char *set_num_hdrives(int);
char *set_floppy_drive(int drive, char *device, int sectors, int cylinders, int heads);
char *set_hard_drive(int drive, char *device, int sectors, int cylinders, int heads);
void open_drive(DiskTab [], int drive);
char *set_boot_drive(int);


#endif
