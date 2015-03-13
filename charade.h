#ifndef CHARADE_H_
#define CHARADE_H_

#include <stdint.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xft/Xft.h>

#define TOUCH_RADIUS 50
#define CENTER_RADIUS 30

#define TRANSPARENT 0
#define BACKGROUND_COLOR 0x60000000
#define TOUCH_COLOR 0xd0204a87
#define ANALYSIS_COLOR 0xd0888a85
#define TEXT_COLOR ((XRenderColor) {\
		.red = 0xeeee, \
		.blue = 0xeeee, \
		.green = 0xecec, \
		.alpha = 0xffff, \
	})

#define TEXT_FONT "Consolas:pixelsize=50"

struct touch {
	int id;
	double x, y;
};

/*
 * Main application state structure
 */
struct kbd_state {
	Display *dpy;
	XVisualInfo xvi;
	Colormap cmap;
	Window win;
	GC gc;
	XftFont *font;
	XftDraw *draw;
	XftColor textclr;
	int xi_opcode;
	int input_dev;
	int ntouches;
	int shutdown;
	struct touch *touches;
};


#endif
