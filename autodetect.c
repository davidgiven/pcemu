/*
 * wordsize.c
 *
 * Produces a #include file that defines INT16, INT32 etc in a (hopefully)
 * portable manner.
 */

#include <stdio.h>
#include <stdlib.h>

struct {
	char *name;
	int size;
} typearray[] = {
	{"char",	sizeof(char)},
	{"short int",	sizeof(short int)},
	{"int",		sizeof(int)},
	{"long int",	sizeof(long int)},
	{NULL,		0},
};

void dotype(int size)
{
	int i = 0;
	
	while (typearray[i].name)
	{
		if ((typearray[i].size * 8) == size)
		{
			printf("typedef signed %s INT%d;\n", typearray[i].name, size);
			printf("typedef unsigned %s UINT%d;\n", typearray[i].name, size);
			return;
		}
		i++;
	}
	
	fprintf(stderr, "Could find a type of length %d\n", size);
	exit(1);
}
			
int main(int argc, char *argv[])
{
	union {
		char c[4];
		int i;
	} endian;
	
	printf("/* Automatically created file --- do not edit */\n\n");
	
	dotype(8);
	dotype(16);
	dotype(32);
	
	endian.i = 0;
	endian.c[0] = 1;
	if (endian.i == 1)
		printf("#define PCEMU_LITTLE_ENDIAN\n");
	else
		printf("#define PCEMU_BIG_ENDIAN\n");
		
	return 0;
}
