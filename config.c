#include "global.h"
#include "bios.h"
#include "vgahard.h"
#include "video.h"
#include <string.h>

void check_error(const char *msg, int line)
{
    if (msg)
        fprintf(stderr,"%s, line %d\n", msg, line);
}

void read_pcemurc(void)
{         /* This procedure is a bit of a hack :) - will be redone later */
    FILE *f1;
    char buffer[1024];  /* Maximum path length. Should really be a #define */
    char keyword[1024];
    char value[1024];
    int line = 0;
    int i;

    /* Try current directory first... */

    if ((f1 = fopen(".pcemurc","r")) == NULL)
    {           /* Try home directory */
        sprintf(buffer,"%s/.pcemurc", getenv("HOME"));
        if ((f1 = fopen(buffer,"r")) == NULL)
        {
            printf("Warning: .pcemurc not found - using compile time defaults\n");
            return;
        }
    }

    /* This bit hacked by David Given to, er, improve... */

    while (fgets(buffer,sizeof buffer,f1) != NULL)
    {
        line++;
        i = 0;				/* Strip leading spaces */
        while(buffer[i] == ' ')
            i++;
        strcpy(keyword, buffer+i);

        while(buffer[i] != ' ')		/* Skip to end of keyword */
            i++;
        while(buffer[i] == ' ')
            i++;
        strcpy(value, buffer+i);	/* Write value */

        i = 0;				/* Trim keyword to one word */
        while(keyword[i] != ' ')
            i++;
        keyword[i] = '\0';

        i = 0;				/* Strip out \n */
        while(value[i] != '\0')
        {
            if (value[i] == '\n')
                value[i] = '\0';
            i++;
        }

        if ((keyword[0] == '#') || (keyword[0] == ' ') || (keyword[0] == '\n'))
            continue;

        if (strcasecmp(keyword, "floppy") == 0)
        {
            int drive;
            char device[1024];
            int sectors;
            int cylinders;
            int heads;

            if (sscanf(value, "%d %s %d %d %d", &drive, device, &sectors, &cylinders, &heads) == 5)
                 check_error(set_floppy_drive(drive, device, sectors, cylinders, heads), line);
            else
                 check_error("Syntax error in \"floppy\" directive", line);
        }   
        else if (strcasecmp(keyword, "harddrive") == 0)
        {
            int drive;
            char device[1024];
            int sectors;
            int cylinders;
            int heads;

            if (sscanf(value, "%d %s %d %d %d", &drive, device, &sectors, &cylinders, &heads) == 5)
                 check_error(set_hard_drive(drive, device, sectors, cylinders, heads), line);
            else
                 check_error("Syntax error in \"hard\" directive", line);
        }
        else if (strcasecmp(keyword, "numharddrives") == 0)
            check_error(set_num_hdrives(strtol(value, NULL, 10)), line);
        else if (strcasecmp(keyword, "numfloppydrives") == 0)
            check_error(set_num_fdrives(strtol(value, NULL, 10)), line);
        else if (strcasecmp(keyword,"updatespeed") == 0)
            check_error(set_update_rate(strtol(value, NULL,10)), line);
        else if (strcasecmp(keyword,"cursorspeed") == 0)
            check_error(set_cursor_rate(strtol(value, NULL,10)), line);
/*	else if (strcasecmp(keyword,"keymap") == 0)
		check_error(set_keymap(buffer), line);*/
	else if (strcasecmp(keyword, "video") == 0)
	    check_error(set_video_mode(value), line);
        else
            check_error("Syntax error in .pcemu file", line);
    }
    fclose(f1);
}           
