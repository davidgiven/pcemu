#include "global.h"

#include "vgahard.h"
#include "hardware.h"
#include "bios.h"
#include "video.h"

#include <curses.h>

extern int mono;

static WINDOW *scr;

static void curses_end(void)
{
    endwin();
}

static void curses_init(void)
{
    mono = 1;
    scr = initscr();
    cbreak();
    noecho();
    nodelay(stdscr, TRUE);
    keypad(scr, TRUE);

    on_exit((void (*)(int, void *))curses_end, NULL);
}

static void curses_flush(void)
{
    refresh();
}

static void curses_put_cursor(unsigned x, unsigned y)
{
}

static void curses_unput_cursor(void)
{
}

static void curses_copy(unsigned x1, unsigned y1, unsigned x2, unsigned y2,
          unsigned nx, unsigned ny)
{
    /* Cant figure this out.  Be evil. */
    int x, y;

    for (y=0; y<=y2-y1; y++) {
	for (x=0; x<=x2-x1; x++) {
	    int at = mvinch(y1+y, x1+x);
	    mvaddch(ny+y, nx+x, at);
	}
    }
}

static void curses_new_cursor(int st, int end)
{
}

static void curses_window_size(unsigned width, unsigned height)
{
}

static void curses_draw_line(unsigned x, unsigned y, const char *text, unsigned len, BYTE attr)
{
    mvaddnstr(y, x, text, len);
}

static BYTE scan_table1[256 - 0x20] =
{
    0x39, 0x02, 
#ifdef KBUK             /* double quotes, hash symbol */
    0x03, 0x2b,
#else
    0x28, 0x04,
#endif
    0x05, 0x06, 0x08, 0x28,
    0x0a, 0x0b, 0x09, 0x0d, 0x33, 0x0c, 0x34, 0x35,
    0x0b, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
    0x09, 0x0a, 0x27, 0x27, 0x33, 0x0d, 0x34, 0x35,
#ifdef KBUK             /* at symbol */
    0x28, 
#else
    0x03,
#endif
    0x1e, 0x30, 0x2e, 0x20, 0x12, 0x21, 0x22,
    0x23, 0x17, 0x24, 0x25, 0x26, 0x32, 0x31, 0x18,
    0x19, 0x10, 0x13, 0x1f, 0x14, 0x16, 0x2f, 0x11,
    0x2d, 0x15, 0x2c, 0x1a, 
#ifdef KBUK             /* backslash */
    0x56, 
#else
    0x2b,
#endif
    0x1b, 0x07, 0x0c,
    0x29, 0x1e, 0x30, 0x2e, 0x20, 0x12, 0x21, 0x22,
    0x23, 0x17, 0x24, 0x25, 0x26, 0x32, 0x31, 0x18,
    0x19, 0x10, 0x13, 0x1f, 0x14, 0x16, 0x2f, 0x11,
    0x2d, 0x15, 0x2c, 0x1a, 
#ifdef KBUK             /* vertical bar */
    0x56, 
#else
    0x2b,
#endif

    0x1b, 

#ifdef KBUK             /* tilde */
    0x2b,
#else
    0x29,
#endif
    0
};

static struct {
    int code;
    int scan;
} basic_keys[] = {
    { 263, 0x0e },
    { 10, 0x1c },
    { 27, 0x01 },
    { 9,  0x0f }
};

static unsigned translate(int key)
{
    int i;
    if (key >= 0x20 && key <= 0x20+sizeof(scan_table1))
        return (scan_table1[key - 0x20]);
    for (i=0; i<(sizeof(basic_keys)/sizeof(basic_keys[0])); i++) {
	if (basic_keys[i].code == key)
	    return basic_keys[i].scan;
    }
    return 0;
}

static void curses_tick(void)
{
    int got = getch();

    if (got != ERR) {
	int scan, count;
	char buffer[10];

	//	printf("Got %u\n", got);
	if ((scan = translate(got)) != 0) {
            for (count = 0; scan; count++)
            {
		buffer[count] = scan & 0xff;
                
                scan >>= 8;
            }

            if (port60_buffer_ok(count))
            {
                D(printf("Returning %d scan code bytes\n",count););
                put_scancode(buffer, count);
            }
	}
    }
}

VIDEO_DRIVER curses_video = {
    "curses",
    curses_init,
    curses_end,
    curses_flush,
    curses_put_cursor,
    curses_unput_cursor,
    curses_copy,
    curses_new_cursor,
    curses_window_size,
    curses_draw_line,
    curses_tick
};
