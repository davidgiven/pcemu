#############################################################################
#                                                                           #
#                            Third Year Project                             #
#                                                                           #
#                            An IBM PC Emulator                             #
#                          For Unix and X Windows                           #
#                                                                           #
#                             By David Hedley                               #
#                                                                           #
#                                                                           #
# This program is Copyrighted.  Consult the file COPYRIGHT for more details #
#                                                                           #
#############################################################################

# Valid options are as follow:
# -DINLINE_FUNCTIONS if your compiler support inline functions (most do)
# -DDEBUG            prints lots of debugging messages.
# -DDEBUGGER         compiles in the debugger
# -DKBUK             if you have a UK style 102 key keyboard
# -DBIG_ENDIAN       if your computer is big-endian (Sparc, 68000 etc)
# -DLITTLE_ENDIAN    if your computer is little-endian (80x86 etc)
# -DALIGNED_ACCESS   if your computer requires words to be on even boundaries
# -DBIGCASE          If your compiler/computer can handle 256 case switches
#
# -D_HPUX_SOURCE     If you are compiling on an HP
# -DSOLARIS          If you are using Solaris (only 2.3 has been tested)
# -DSGI              If you are using an SGI
# -DRS6000           If you are using an RS6000
#
# Note that when compiling on the RS6000 and SGI using the standard cc compiler
# specifying -O2 to optimise the code results in the emulator crashing. I
# believe this is due to compiler bugs (as the programs run OK without
# optimisation). If anyone can shed light on what the problem is I would be
# eternally grateful.
#
# Not specifying the following will mean the defaults are used. They can be
# overridden in the .pcemurc file
#
# -DBOOT360          Boot from a 360k disk image  file
# -DBOOT720          Boot from a 720k disk image file
# -DBOOT1_44         Boot from a 1.44Mb disk image file
# -DBOOT1_2          Boot from a 1.2Mb disk image file
#
# -DBOOTFILE="xyz"   Boot from disk image "xyz" (default "DriveA")
# -DCURSORSPEED=n    Set cursor flash speed to n frames per flash (default 30)
# -DUPDATESPEED=n    Set screen update speed to n frames per update (default 5)
# 
# Including -fomit-frame-pointer should increase speed a little, but it has
# been known to crash the emulator when running on certain machines (80x86
# based PCs under Linux, and HPs running HPUX). 

CC =		gcc -Wall
OPTIONS =	-DALIGNED_ACCESS -DBIGCASE -DINLINE_FUNCTIONS -DDISABLE_MFS
XROOT = 	/usr/X11R6
#CFLAGS = 	-g
CFLAGS = 	-O2 -fomit-frame-pointer -fno-strength-reduce

# You may need to add -N to the LFLAGS if you get sporadic segmentation
# faults. So far I have only needed to do this when compiling under Linux
# as Xlib seems to be mysteriously writing to its text segment

LFLAGS = 	-g

LIBRARIES =	-L$(XROOT)/lib		\
		-lXext			\
		-lX11

LIBRARIES := 	$(LIBRARIES) -lefence -lncurses

OFILES =	main.o		\
		cpu.o		\
		bios.o		\
		vga.o		\
		vgahard.o	\
		debugger.o	\
		hardware.o	\
		mfs.o		\
		ems.o		\
		video.o		\
		curses.o	\
		xstuff.o	\
		config.o

PROGNAME  =	pcemu

GLOBAL_DEP =	global.h	\
		mytypes.h

all: $(PROGNAME)

cpu.o:	$(GLOBAL_DEP) cpu.h instr.h debugger.h hardware.h
ems.o:	$(GLOBAL_DEP) cpu.h bios.h
main.o: $(GLOBAL_DEP) bios.h hardware.h video.h
bios.o: $(GLOBAL_DEP) bios.h cpu.h vga.h vgahard.h debugger.h hardware.h \
        keytabs.h mfs_link.h
vga.o:	$(GLOBAL_DEP) bios.h cpu.h vga.h vgahard.h hardware.h
vgahard.o: $(GLOBAL_DEP) vgahard.h video.h hardware.h
debugger.o: $(GLOBAL_DEP) cpu.h debugger.h disasm.h vgahard.h
xstuff.o: $(GLOBAL_DEP) vgahard.h video.h icon.h hardware.h
hardware.o: $(GLOBAL_DEP) cpu.h vgahard.h debugger.h hardware.h
mfs.o: $(GLOBAL_DEP) cpu.h mfs.h mfs_link.h
mytypes.h: autodetect.h

autodetect.h: autodetect
	./autodetect > autodetect.h

.c.o:
	$(CC) $(CFLAGS) $(OPTIONS) -c $<

$(PROGNAME): $(OFILES)
	$(CC) $(CFLAGS) -o $(PROGNAME) $(OFILES) $(LFLAGS) $(LIBRARIES)

clean:
	rm -f $(PROGNAME) autodetect *.o *~

