
/* Program to convert disk into disk image file */
/* To run, give the source drive as a parameter */
/* The output file will be called drivea (which will have to renamed to
	DriveA when transferred to Unix */
/* By D. Hedley 18/6/94 */

#include <dos.h>
#include <stdio.h>
#include <stdlib.h>

#define OUTPUT "drivea"

int drive, drivetype;

struct
{
	int tracks, sectors, heads;
	char *descr;
} drivetypes[] =
{
	{ 40,  9, 2, "360k 5¬\"" },
	{ 80, 15, 2, "1.2MB 5¬\"" },
	{ 80,  9, 2, "720k 3«\"" },
	{ 80, 18, 2, "1.44MB 3«\"" },
};

void reset_drive(void)
{
	struct REGPACK regs;

	regs.r_ax = 0;
	regs.r_dx = drive;
	intr(0x13, &regs);
}

int read_sector(int sector, int head, int track, char *buffer)
{
	struct REGPACK regs;

	regs.r_ax = 0x0201;
	regs.r_cx = (track << 8) | sector;
	regs.r_dx = (head << 8) | drive;
	regs.r_es = _DS;
	regs.r_bx = (int)buffer;

	intr(0x13, &regs);

	return regs.r_ax;
}

void copy_sector(int sector, int head, int track, FILE *out)
{
	int len;
	int count;
	char buffer[512];

	printf("Copying track %d, sector %d, head %d   %n",track,sector,head,&len);
	for (; len > 0; len--)
		printf("\b");

	for (count = 0; count < 3; count++)
	{
		if (read_sector(sector,head,track,buffer) == 1)
		{
			if (fwrite(buffer,512,1,out) != 1)
			{
				perror("writing output");
				exit(1);
			}
			return;
		}

		reset_drive();
	}

	fprintf(stderr,"Sorry - copy failed\n");
	exit(1);
}

void main(int argc, char **argv)
{
	struct REGPACK regs;
	FILE *f;
	int tmp;
	int tracks, sectors, heads;

	if (argc != 2 || (argv[1][0] != 'A' && argv[1][0] != 'B'))
	{
		fprintf(stderr,"Format: dumpdisk A|B\n");
		exit(1);
	}

	if ((f = fopen(OUTPUT,"wb")) == NULL)
	{
		perror("Opening output file");
		exit(1);
	}

	drive = argv[1][0] - 'A';

	reset_drive();

	regs.r_ax = 0x0800;
	regs.r_dx = drive;
	intr(0x13,&regs);
	tmp = _BX & 0xff;
	if (tmp < 1 || tmp > 4)
	{
		fprintf(stderr,"Error reading drive %c\n", drive+'A');
		exit(1);
	}

	drivetype = tmp-1;

	printf("Drive %c is %s\n", drive+'A', drivetypes[drivetype].descr);

	for (tracks = 0; tracks < drivetypes[drivetype].tracks; tracks++)
		for (heads = 0; heads < drivetypes[drivetype].heads; heads++)
			for (sectors = 1; sectors <= drivetypes[drivetype].sectors; sectors++)
				copy_sector(sectors, heads, tracks, f);

	fclose(f);
}





