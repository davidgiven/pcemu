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

#include "global.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <stdio.h>

#include "icon.h"
/* #include "xstuff.h" */
#include "vgahard.h"
#include "hardware.h"
#include "bios.h"
#include "video.h"

/*
#include <sun/dkio.h>
*/
#define BITMAPDEPTH 1

void process_menu(void);
void drawmenuitem(Window, GC, int, char *, int, int);
void menu_selected(int);

static Display *display;
static int screen_num;
static XFontStruct *font_info;
static XFontStruct *menu_font1;
static XFontStruct *menu_font2;
static GC gc, cursorgc;
static Pixmap under_cursor;
static Window win;
static XSizeHints size_hints;

static int font_height;
static int font_width;
static int font_descent;

static int menu_font_height;
static int menu_font_width;
static int menu_font_descent;

static int cx, cy, cs, chgt;

int blink, graphics;
extern int mono;

struct 
{
    unsigned r,g,b;
} text_cols[] = 
{
    {0,0,0},{10,10,185},{10,195,10},{20,160,160},
    {167,10,10},{167,0,167},{165,165,40},{197,197,197},
    {100,100,100},{10,10,255},{10,255,10},{10,255,255},
    {255,10,10},{255,10,255},{255,255,0},{255,255,255}
};

unsigned col_vals[16];
unsigned blink_vals[16];

#ifdef AUTOEJECT
void eject(void);
#endif

void get_colours(void)
{
    XColor col;
    int i;

    if (mono)
    {
        col_vals[0] = BlackPixel(display, screen_num);

        for (i = 1; i < 16; i++)
            col_vals[i] = WhitePixel(display, screen_num);
    }
    else
    {
        col.flags = DoRed | DoGreen | DoBlue;
        for(i = 0; i < 16; i++)
        {
            col.red=text_cols[i].r << 8;
            col.green=text_cols[i].g << 8;
            col.blue=text_cols[i].b << 8;
            XAllocColor(display, DefaultColormap(display, screen_num),
                        &col);
            col_vals[i] = col.pixel;
        }
    }
}


static void x_put_cursor(unsigned x, unsigned y)
{
    if (chgt < 1)
        return;

    cx = x*font_width;
    cy = y*font_height+cs+font_descent;

    XCopyArea(display, win, under_cursor, cursorgc, cx, cy, font_width, 
              chgt, 0, 0);
    XFillRectangle(display, win, cursorgc, cx, cy, font_width, chgt);
}


static void x_unput_cursor(void)
{
    if (chgt < 1)
        return;

    XCopyArea(display, under_cursor, win, cursorgc, 0, 0, font_width, chgt,
              cx, cy);
}

static void x_new_cursor(int st, int end)
{
    cs = st;
    chgt = end - st + 1;
}

static void setGC(void)
{
    unsigned long valuemask = 0; /* ignore XGCvalues and use defaults */
    XGCValues values;
    unsigned int line_width = 6;
    int line_style = LineOnOffDash;
    int cap_style = CapRound;
    int join_style = JoinRound;

    gc = XCreateGC(display, win, valuemask, &values);
    cursorgc = XCreateGC(display, win, valuemask, &values);
    
    XSetFont(display, gc, font_info->fid);
    XSetForeground(display, gc, WhitePixel(display,screen_num));
    XSetBackground(display, gc, BlackPixel(display,screen_num));

    XSetLineAttributes(display, gc, line_width, line_style, 
                       cap_style, join_style);
    
    XSetForeground(display, cursorgc, WhitePixel(display,screen_num));
    XSetBackground(display, cursorgc, BlackPixel(display,screen_num));
}


static void setcursor(void)
{
    under_cursor = XCreatePixmap(display, win, font_width, font_height, 
                                 DefaultDepth(display, screen_num));
}

static void load_fonts(void)
{
    static char *fontname = "vga";
    static char *fontname2 = "fixed";
    static char *menu_font2_name = "lucidasans-bold-12";
    static char *menu_font1_name = "lucidasans-12";
    char string[] = "W";
    
    if ((font_info = XLoadQueryFont(display, fontname)) == NULL)
    {
        fprintf(stderr, "Warning: cannot locate correct VGA font\n");
        fprintf(stderr, "Reverting to standard 8x16 font\n");
        if ((font_info = XLoadQueryFont(display, fontname2)) == NULL)
        {
            fprintf(stderr, "%s: Cannot open 8x16 font\n", 
                    progname);
            exit(1);
        }
    }
    font_height = font_info->ascent + font_info->descent;
    font_width = XTextWidth(font_info, string, 1);
    font_descent = font_info->descent;

    if ((menu_font1 = XLoadQueryFont(display, menu_font1_name)) == NULL)
    {
        fprintf(stderr, "Warning: cannot locate %s font\n", menu_font1_name);
        fprintf(stderr, "Reverting to standard 8x16 font\n");
        if ((menu_font1 = XLoadQueryFont(display, fontname2)) == NULL)
        {
            fprintf(stderr, "%s: Cannot open 8x16 font\n", 
                    progname);
            exit(1);
        }
    }

    if ((menu_font2 = XLoadQueryFont(display, menu_font2_name)) == NULL)
    {
        fprintf(stderr, "Warning: cannot locate %s font\n", menu_font2_name);
        fprintf(stderr, "Reverting to standard 8x16 font\n");
        if ((menu_font2 = XLoadQueryFont(display, fontname2)) == NULL)
        {
            fprintf(stderr, "%s: Cannot open 8x16 font\n", 
                    progname);
            exit(1);
        }
    }
    menu_font_height = menu_font1->ascent + menu_font1->descent;
    menu_font_width = XTextWidth(menu_font1, string, 1);
    menu_font_descent = menu_font1->descent;
}


void clear_screen(void)
{
    XClearWindow(display, win);
}


static void x_copy(unsigned x1, unsigned y1, unsigned x2, unsigned y2,
		   unsigned nx, unsigned ny)
{
    x1 *= font_width;
    y1 *= font_height;
    x2 = (x2+1)*font_width;
    y2 = (y2+1)*font_height;
    nx *= font_width;
    ny *= font_height;

    XCopyArea(display, win, win, gc, x1, y1+font_descent, x2-x1, y2-y1, nx,
              ny+font_descent);
}
          

static void x_draw_line(unsigned x, unsigned y, const char *text, unsigned len, BYTE attr)
{
    XSetForeground(display, gc, col_vals[attr & 0xf]);
    XSetBackground(display, gc, col_vals[(attr & 0x70) >> 4]);
    XDrawImageString(display, win, gc, 
                     x*font_width, (y+1)*font_height, text, len);
}

    
void draw_char(unsigned x, unsigned y, char c, BYTE attr)
{
    XSetForeground(display, gc, col_vals[attr & 0xf]);
    XSetBackground(display, gc, col_vals[(attr & 0x70) >> 4]);
    XDrawImageString(display, win, gc, 
                     x*font_width, (y+1)*font_height, &c, 1);
}


static void x_window_size(unsigned width, unsigned height)
{
    width *= font_width;
    height = height*font_height+4;

    D(printf("Window size = %dx%d\n", width, height););
    XResizeWindow(display, win, width, height);
    
    size_hints.flags = PSize | PMinSize | PMaxSize;
    size_hints.max_width = size_hints.min_width = width;
    size_hints.max_height = size_hints.min_height = height;

    XSetWMNormalHints(display, win, &size_hints);
}


static void x_init(void)
{
    unsigned int width, height;	/* window size */
    int x, y; 	/* window position */
    unsigned int border_width = 4;	/* four pixels */
    unsigned int display_width, display_height;
    char *display_name = NULL;
    XWMHints wm_hints;
    char *window_name = "PC Emulator";
    char *icon_name = "PC";
    XClassHint class_hints;
    XTextProperty windowName, iconName;
    Pixmap icon_pixmap;

    if ((display=XOpenDisplay(display_name)) == NULL)
    {
        fprintf(stderr, "%s: cannot connect to X server %s\n", 
                progname, XDisplayName(display_name));
        exit(1);
    }
    
    screen_num = DefaultScreen(display);
    display_width = DisplayWidth(display, screen_num);
    display_height = DisplayHeight(display, screen_num);
    
    x = y = 0;
    
    load_fonts();
    
    width = 80*font_width;
    height = 25*font_height+4;
    
    win = XCreateSimpleWindow(display, RootWindow(display,screen_num), 
                              x, y, width, height, border_width,
                              WhitePixel(display, screen_num),
                              BlackPixel(display,screen_num));
    
    icon_pixmap = XCreateBitmapFromData(display, win, icon_bits, 
                                        icon_width, icon_height);
    
    if (XStringListToTextProperty(&window_name, 1, &windowName) == 0)
    {
        fprintf(stderr, "%s: structure allocation for windowName failed.\n", 
                progname);
        exit(1);
    }
    
    if (XStringListToTextProperty(&icon_name, 1, &iconName) == 0)
    {
        fprintf(stderr, "%s: structure allocation for iconName failed.\n", 
                progname);
        exit(1);
    }
    
    size_hints.flags = PSize | PMinSize | PMaxSize;
    size_hints.max_width = size_hints.min_width = width;
    size_hints.max_height = size_hints.min_height = height;

    wm_hints.initial_state = NormalState;
    wm_hints.input = True;
    wm_hints.icon_pixmap = icon_pixmap;
    wm_hints.flags = StateHint | IconPixmapHint | InputHint;
    
    class_hints.res_name = progname;
    class_hints.res_class = "PCEmulator";
    
    XSetWMProperties(display, win, &windowName, &iconName, 
                     &progname, 1, &size_hints, &wm_hints, 
                     &class_hints);
    
    XSelectInput(display, win, ExposureMask | KeyPressMask | KeyReleaseMask |
                 StructureNotifyMask | ButtonPressMask | ButtonReleaseMask);
    
    setGC();
    setcursor();

    mono = (DisplayCells(display, screen_num) < 16);
    graphics = 0; /* !(DisplayCells(display, screen_num) < 256); */

    get_colours();

    /* Display window */
    XMapWindow(display, win);
}


static void x_flush(void)
{
    XFlush(display);
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


static struct
{
    KeySym key;
    unsigned scan_code;
} scan_table2[] =
{
    { XK_BackSpace,     0x0e },
    { XK_Tab,           0x0f },
    { XK_Return,        0x1c },
    { XK_Escape,        0x01 },
    { XK_Delete,        0x53e0 },
        
    { XK_Home,          0x47e0 },
    { XK_Left,          0x4be0 },
    { XK_Up,            0x48e0 },
    { XK_Right,         0x4de0 },
    { XK_Down,          0x50e0 },
    { XK_Prior,         0x49e0 },
    { XK_Next,          0x51e0 },
    { XK_End,           0x4fe0 },
    { XK_Insert,        0x52e0 },
    { XK_Num_Lock,      0x45 },

/* This block of codes is for Sun Type 4 keyboards... */

    { XK_F27,           0x47e0 },       /* Home/R7/F27 */
    { XK_F29,           0x49e0 },       /* Prior/R9/F29 */
    { XK_F35,           0x51e0 },       /* Next/R15/F35 */
    { XK_F33,           0x4fe0 },       /* End/R13/F33 */

    { XK_F25,           0x36e0 },       /* Keypad divide/R5/F25 */
    { XK_F26,           0x37 },         /* Keypad multiply/R6/F26 */

    { XK_F23,           0x46 },         /* Scroll lock/R3/F23 */
   
    { XK_F31,           0x4c },         /* Keypad 5/R11/F31 */

/* End of Sun type 4 codes */

    { XK_KP_Enter,      0x1ce0 },
    { XK_KP_Multiply,   0x37 },
    { XK_KP_Add,        0x4e },
    { XK_KP_Subtract,   0x4a },
    { XK_KP_Divide,     0x36e0 },
    { XK_KP_Decimal,    0x53 },

    { XK_KP_0,          0x52 },
    { XK_KP_1,          0x4f },
    { XK_KP_2,          0x50 },
    { XK_KP_3,          0x51 },
    { XK_KP_4,          0x4b },
    { XK_KP_5,          0x4c },
    { XK_KP_6,          0x4d },
    { XK_KP_7,          0x47 },
    { XK_KP_8,          0x48 },
    { XK_KP_9,          0x49 },

    { XK_F1,            0x3b },
    { XK_F2,            0x3c },
    { XK_F3,            0x3d },
    { XK_F4,            0x3e },
    { XK_F5,            0x3f },
    { XK_F6,            0x40 },
    { XK_F7,            0x41 },
    { XK_F8,            0x42 },
    { XK_F9,            0x43 },
    { XK_F10,           0x44 },
    { XK_F11,           0x57 },
    { XK_F12,           0x58 },

    { XK_Shift_L,       0x2a },
    { XK_Shift_R,       0x36 },
    { XK_Control_L,     0x1d },
    { XK_Control_R,     0x1de0 },
    { XK_Meta_L,        0x38 },
    { XK_Alt_L,         0x38 },
    { XK_Meta_R,        0x38e0 },
    { XK_Alt_R,         0x38e0 },

    { XK_Scroll_Lock,   0x46 },
    { XK_Caps_Lock,     0xba3a }
};


static unsigned translate(KeySym key)
{
    int i;
    if (key >= 0x20 && key <= 0x20+sizeof(scan_table1))
        return (scan_table1[key - 0x20]);

    for (i = 0; i < sizeof(scan_table2); i++)
        if (scan_table2[i].key == key)
            return (scan_table2[i].scan_code);

    return 0;
}


int put_scan_table(BYTE code, unsigned char c)
{
	/* interface to overload scan_table1 from .pcemurc */
	if(c < ' ' || c >= ' ' + sizeof scan_table1)
		return 1;
	scan_table1[c - ' '] = code;
	return 0;
}


static void x_process_events(void)
{    
#define KEY_BUFFER_SIZE 100

    XEvent event;
    KeySym key;
    unsigned scan;
    static BYTE buffer[4];
    XEvent keyeventbuffer[KEY_BUFFER_SIZE];
    int keyptr = 0;
    int count;

    while (XPending(display) > 0)
    {
        XNextEvent(display, &event);
        switch(event.type)
        {
        case Expose:
            if (event.xexpose.count != 0)
                break;
            
            pcemu_refresh();
            break;
        case UnmapNotify:       /* Pause emulator */
            stoptimer();
            for (;;)
            {
                XNextEvent(display, &event);
                switch(event.type)
                {
                case MapNotify:
                    starttimer();
                    return;
                default:
                    break;
                }
            }
            break;
        case KeyPress:
        case KeyRelease:
            key = XLookupKeysym(&event.xkey, 0);

            D(printf("State = %X. Got keysym number: %X\n", 
                     (int)event.xkey.state, (int)key););

                /* XK_F21 is for sun type 4 keyboards.. */
            if (key == XK_Break || key == XK_F21)
                key = XK_Pause;

            if (key == XK_Pause)
            {
                D(printf("Pause pressed. State = %02X\n", event.xkey.state););
                if (event.xkey.state & ControlMask)
                    scan = 0xc6e046e0;
                else
                    scan = 0x451de1;
            }                   /* XK_F22 is sun type 4 PrtScr */
            else if (key == XK_Print || key == XK_F22)
            {
                D(printf("Print pressed. State = %02X\n", event.xkey.state););
                if (event.xkey.state & Mod1Mask)
                    scan = 0x54;
                else
                    scan = 0x37e02ae0;
            }
#ifdef KBUK
            else if ((key == XK_numbersign) && (event.xkey.state & ShiftMask))
                scan = 0x4;
#endif
            else if (key == XK_sterling)
                scan = 0x4;
            else if ((scan = translate(key)) == 0)
                break;

            for (count = 0; scan; count++)
            {
                if (key != XK_Caps_Lock && event.type == KeyRelease)                    
                    buffer[count] = 0x80 | (scan & 0xff);
                else
                    buffer[count] = scan & 0xff;
                
                scan >>= 8;
            }

            if (port60_buffer_ok(count))
            {
                D(printf("Returning %d scan code bytes\n",count););
                put_scancode(buffer, count);
            }
            else
            {
                D(printf("Port60 buffer full...\n"););

                if (keyptr < KEY_BUFFER_SIZE)
                    keyeventbuffer[keyptr++] = event;
            }
                  
            break;
        case ButtonPress:
            process_menu();
        default:
            break;
        }
    }

    for (count = keyptr; count > 0; count--)
        XPutBackEvent(display, &keyeventbuffer[count-1]);
}


static void x_end(void)
{
    XUnloadFont(display, font_info->fid);
    XUnloadFont(display, menu_font1->fid);
    XUnloadFont(display, menu_font2->fid);
    XFreeGC(display, gc);
    XFreeGC(display, cursorgc);
    XFreePixmap(display, under_cursor);
    XCloseDisplay(display);
}

void process_menu(void)
{
#define MENUITEMS 7
#define MENUGAP 4
        XSetWindowAttributes attributes;
        Window window, root, child;
        GC gc;
	int win_x, win_y;
	int root_x, root_y;
	unsigned int buttons;
	char *menu[] = {"PCEmu Controls", "-", "A", "B", "-", "Reboot", "Shutdown"};
	int greyed[] = {1, 1, 0, 0, 1, 1, 0};
	int i;
	int selection, oldselection;
	XGCValues values; /* Unused. Only here because XCreateGC demands it. */

	if (fdisk[0].removable)
		if (fdisk[0].mounted)
			menu[2] = "Dismount drive A";
		else
			menu[2] = "Mount drive A";
	else
	{
		menu[2] = "(Drive A not removable)";
		greyed[2] = 1;
	}
	if (fdisk[1].removable)
		if (fdisk[1].mounted)
			menu[3] = "Dismount drive B";
		else
			menu[3] = "Mount drive B";
	else
	{
		menu[3] = "(Drive B not removable)";
		greyed[3] = 1;
	}

	if (!XQueryPointer(display, win, &root, &child, &root_x, &root_y, &win_x, &win_y, &buttons))
		return;
        window = XCreateSimpleWindow(display, RootWindow(display, screen_num),  
                root_x-100, root_y-10, 201, MENUITEMS*(menu_font_height+MENUGAP)+10, 1, BlackPixel(display, screen_num),
		WhitePixel(display, screen_num));
        attributes.override_redirect = True;
        XChangeWindowAttributes(display, window, CWOverrideRedirect,
                &attributes);
        XMapWindow(display, window);
	gc = XCreateGC(display, window, 0, &values);
	XSetForeground(display, gc, BlackPixel(display, screen_num));
	XDrawRectangle(display, window, gc, 2, 2, 196, MENUITEMS*(menu_font_height+MENUGAP)+5);
	oldselection = -1;

	for(i=0; i<MENUITEMS; i++)
		drawmenuitem(window, gc, (i+1)*(menu_font_height+MENUGAP)+2, menu[i], greyed[i], 0);
	do {
		XQueryPointer(display, window, &root, &child, &root_x, &root_y, &win_x, &win_y, &buttons);
		selection = (win_y-2)/(menu_font_height+4);
		if ((selection != oldselection) && (selection>=0) && (selection<MENUITEMS))
		{
			if (oldselection != -1)
				drawmenuitem(window, gc, (oldselection+1)*(menu_font_height+MENUGAP)+2, menu[oldselection], greyed[oldselection], 0);
			if (greyed[selection] == 0)
				drawmenuitem(window, gc, (selection+1)*(menu_font_height+MENUGAP)+2, menu[selection], greyed[selection], 1);
			oldselection = selection;
		}	
	} while (buttons && Button2Mask);
	XDestroyWindow(display, window);
	if (!greyed[selection])
		menu_selected(selection);
}

void drawmenuitem(Drawable window, GC gc, int y, char *string, int fontnum, int reversed)
{
	int width;

	if (fontnum)
	{
		XSetFont(display, gc, menu_font1->fid);
		width = XTextWidth(menu_font1, string, strlen(string));
	}
	else
	{
		XSetFont(display, gc, menu_font2->fid);
		width = XTextWidth(menu_font2, string, strlen(string));
	}
	if (!reversed)
	{
		XSetForeground(display, gc, WhitePixel(display, screen_num));
		XSetBackground(display, gc, BlackPixel(display, screen_num));
	}
	else	
	{
		XSetBackground(display, gc, WhitePixel(display, screen_num));
		XSetForeground(display, gc, BlackPixel(display, screen_num));
	}
	XFillRectangle(display, window, gc, 5, y-menu_font_height-(MENUGAP/3), 191, menu_font_height+MENUGAP);
	if (reversed)
	{
		XSetForeground(display, gc, WhitePixel(display, screen_num));
		XSetBackground(display, gc, BlackPixel(display, screen_num));
	}
	else	
	{
		XSetBackground(display, gc, WhitePixel(display, screen_num));
		XSetForeground(display, gc, BlackPixel(display, screen_num));
	}
	if (*string == '-')
		XDrawLine(display, window, gc, 3, y-(menu_font_height/2), 197, y-(menu_font_height/2));
	else
		XDrawString(display, window, gc, 100-(width/2), y, string, strlen(string));
}

void menu_selected(int selection)
{
	switch(selection)
	{
		case 2:	/* Dismount/mount drive A */
			if (fdisk[0].mounted)
			{
				fdisk[0].mounted = 0;
				close(fdisk[0].fd);
#ifdef AUTOEJECT
				eject();
#endif
			}
			else
				open_drive(fdisk, 0);
			break;
		case 3:	/* Dismount/mount drive B */
			if (fdisk[1].mounted)
			{
				fdisk[1].mounted = 0;
				close(fdisk[1].fd);
#ifdef AUTOEJECT
				eject();
#endif
			}
		else
				open_drive(fdisk, 1);
			break;
		case 5: /* Reboot */
			/* Don't know how to do this yet. */
			break;
		case 6: /* Shutdown */
			exit_emu();
			break;
	}
}

#ifdef AUTOEJECT
void eject(void)
{
	system(AUTOEJECT);
}
#endif

VIDEO_DRIVER x_video = {
    "X",
    x_init,
    x_end,
    x_flush,
    x_put_cursor,
    x_unput_cursor,
    x_copy,
    x_new_cursor,
    x_window_size,
    x_draw_line,
    x_process_events
};

