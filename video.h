/* Simple abstraction to the video driver.
   The video driver is already very simple.
*/
#ifndef VIDEO_INCLUDE
#define VIDEO_INCLUDE

typedef struct
{
    const char *name;
    void (*init)(void);
    void (*end)(void);
    void (*flush)(void);
    void (*put_cursor)(unsigned x, unsigned y);
    void (*unput_cursor)(void);
    void (*copy)(unsigned x1, unsigned y1, unsigned x2, unsigned y2, unsigned newx, unsigned newy);
    void (*new_cursor)(int st, int end);
    void (*window_size)(unsigned width, unsigned height);
    void (*draw_line)(unsigned x, unsigned y, const char *text, unsigned len, BYTE attr);
    void (*process_events)(void);
} VIDEO_DRIVER;

void put_cursor(unsigned x, unsigned y);
void unput_cursor(void);
void copy(unsigned x1, unsigned y1, unsigned x2, unsigned y2, unsigned newx, unsigned newy);
void new_cursor(int st, int end);
void window_size(unsigned width, unsigned height);
void draw_line(unsigned x, unsigned y, const char *text, unsigned len, BYTE attr);

extern VIDEO_DRIVER *video_driver;

void init_video(void);
void end_video(void);
const char *set_video_mode(const char *mode);
void process_events(void);
void flush_video(void);

#endif
