#include "global.h"
#include "video.h"
#include <string.h>

VIDEO_DRIVER *video_driver;

void put_cursor(unsigned x, unsigned y)
{
    video_driver->put_cursor(x, y);
}

void unput_cursor(void)
{
    video_driver->unput_cursor();
}

extern VIDEO_DRIVER curses_video;
extern VIDEO_DRIVER x_video;

static VIDEO_DRIVER *modes[] = {
    &x_video,
    &curses_video
};

#define NUM_MODES	(sizeof(modes)/sizeof(*modes))

const char *set_video_mode(const char *mode)
{
    int i;
    for (i=0; i<NUM_MODES; i++) {
	if (!strcasecmp(modes[i]->name, mode)) {
	    video_driver = modes[i];
	    return NULL;
	}
    }
    return "Invalid video mode.";
}

void init_video(void)
{
    if (video_driver == NULL)
	video_driver = modes[0];
    video_driver->init();
}

void copy(unsigned x1, unsigned y1, unsigned x2, unsigned y2, unsigned newx, unsigned newy)
{
    video_driver->copy(x1, y1, x2, y2, newx, newy);
}

void new_cursor(int st, int end)
{
    video_driver->new_cursor(st, end);
}

void window_size(unsigned width, unsigned height)
{
    video_driver->window_size(width, height);
}

void draw_line(unsigned x, unsigned y, const char *text, unsigned len, BYTE attr)
{
    video_driver->draw_line(x, y, text, len, attr);
}

void end_video(void)
{
    video_driver->end();
}

void process_events(void)
{
    video_driver->process_events();
}

void flush_video(void)
{
    video_driver->flush();
}
