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

/* This is MAIN.C  This is where everything begins... */


#include "global.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>

#include "bios.h"
#include "vga.h"
#include "video.h"
#include "hardware.h"
#include "vgahard.h"

BYTE *memory;
char *progname;

void exit_emu(void)
{
    disable();
    bios_off();
    vga_off();
    exit(0);
}

int main(int argc, char **argv)
{
    progname = (progname = strrchr(argv[0],'/')) ? progname : argv[0];

#ifndef BOOT    
    FILE *f1;
    if (argc != 2)
    {
        fprintf(stderr,"Format: %s testfile\n",progname);
        exit(1);
    }
#endif
    if (!(memory = (BYTE *)malloc(MEMORY_SIZE)))
    {
        fprintf(stderr,"Insufficient available memory\n");
        exit(1);
    }
#ifdef BOOT
    memset(memory,0, MEMORY_SIZE);
#else
    if (!(f1 = fopen(argv[1],"rb")))
    {
        fprintf(stderr,"Cannot open test file\n");
        exit(1);
    }


    fread(memory+0x800,1,MEMORY_SIZE-0x800,f1);
    fclose(f1);
#endif

    read_pcemurc();
    disable();

    init_video();
    init_cpu();
    init_bios();
    init_vga();
    init_timer();

    enable();

    execute();
    
    /* NOT REACHED */
    return 0;
}



