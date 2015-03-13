#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XInput2.h>
#include <X11/Xft/Xft.h>
#include <assert.h>

#include "gesture.h"


/*
 * Searches the input hierarchy for a direct-touch device (e.g. a touchscreen,
 * but not most touchpads).  The id parameter gives either a specific device ID
 * to check or one of the special values XIAllDevices or XIAllMasterDevices.
 */
int init_touch_device(struct kbd_state *state, int id)
{
	// Get list of input devices and parameters
	XIDeviceInfo *di;
	int ndev;
	di = XIQueryDevice(state->dpy, id, &ndev);
	if (!di) {
		fprintf(stderr, "Failed to query devices\n");
		return 1;
	}

	// Find the first direct-touch device
	int i;
	for (i = 0; i < ndev; i++) {
		int j;
		for (j = 0; j < di[i].num_classes; j++) {
			XITouchClassInfo *tci =
				(XITouchClassInfo *) di[i].classes[j];
			if (tci->type == XITouchClass &&
					tci->mode == XIDirectTouch) {
				state->input_dev = di[i].deviceid;
				state->ntouches = tci->num_touches;
				goto done;
			}
		}
	}

done:
	XIFreeDeviceInfo(di);
	if (i >= ndev) {
		fprintf(stderr, "No touch device found\n");
		return 1;
	}

	// Allocate space for keeping track of currently held touches and the
	// corresponding XInput touch event IDs
	state->touches = calloc(state->ntouches, sizeof(state->touches[0]));

	if (!state->touches) {
		fprintf(stderr, "Failed to allocate touches\n");
		free(state->touches);
		return 1;
	}

	// Mark as unused to start
	for (i = 0; i < state->ntouches; i++)
		state->touches[i].id = -1;

	return 0;
}

/*
 * Clean up resources allocated for touch device
 */
void destroy_touch_device(struct kbd_state *state)
{
	free(state->touches);
}

/*
 * Establishes an active grab on the touch device
 */
int grab_touches(struct kbd_state *state)
{
	// Set up event mask for touch events
	unsigned char mask[XIMaskLen(XI_LASTEVENT)];
	memset(mask, 0, sizeof(mask));
	XISetMask(mask, XI_TouchBegin);
	XISetMask(mask, XI_TouchUpdate);
	XISetMask(mask, XI_TouchEnd);

	XIEventMask em = {
		.mask = mask,
		.mask_len = sizeof(mask),
		.deviceid = state->input_dev,
	};

	// Grab the touch device
	return XIGrabDevice(state->dpy, state->input_dev,
			DefaultRootWindow(state->dpy), CurrentTime, None,
			XIGrabModeAsync, XIGrabModeAsync, False, &em);
}

/*
 * Releases grab for the touch device
 */
void ungrab_touches(struct kbd_state *state)
{
	XIUngrabDevice(state->dpy, state->input_dev, CurrentTime);
}

/*
 * Creates the main window for the GKOS keyboard
 */
int create_window(struct kbd_state *state)
{
	// Set up the class hint for the viewer window
	XClassHint *class = XAllocClassHint();
	if (!class) {
		fprintf(stderr, "Failed to allocate class hint\n");
		return 1;
	}
	class->res_name = class->res_class = "gestureview";

	// Create the main fullscreen window
	Screen *scr = DefaultScreenOfDisplay(state->dpy);
	int swidth = WidthOfScreen(scr);
	int sheight = HeightOfScreen(scr);
	XSetWindowAttributes attrs = {
		.background_pixel = BACKGROUND_COLOR,
		.border_pixel = BACKGROUND_COLOR,
		.override_redirect = True,
		.colormap = state->cmap,
	};
	state->win = XCreateWindow(state->dpy, DefaultRootWindow(state->dpy),
			0, 0, swidth, sheight, 0,
			state->xvi.depth, InputOutput, state->xvi.visual,
			CWBackPixel | CWBorderPixel | CWOverrideRedirect | CWColormap, &attrs);
	XSetClassHint(state->dpy, state->win, class);
	XSelectInput(state->dpy, state->win, StructureNotifyMask);

	// Free the class hint
	XFree(class);

	// Grab touch events for the new window
	if (grab_touches(state)) {
		fprintf(stderr, "Failed to grab touch event\n");
		return 1;
	}

	return 0;
}

/*
 * Map the main window and wait for confirmation
 */
void map_window(struct kbd_state *state)
{
	// Map everything and wait for the notify event
	XMapWindow(state->dpy, state->win);

	XEvent ev;
	while (XMaskEvent(state->dpy, StructureNotifyMask, &ev) == Success &&
			(ev.type != MapNotify || ev.xmap.event != state->win))
		;
}

/*
 * Tear down everything created in create_window
 */
void destroy_window(struct kbd_state *state)
{
	ungrab_touches(state);
	XDestroyWindow(state->dpy, state->win);
}

/*
 * Initializes drawing context
 */
int setup_draw(struct kbd_state *state)
{
	XRenderColor xrc;

	state->gc = XCreateGC(state->dpy, state->win, 0, NULL);
	state->draw = XftDrawCreate(state->dpy, state->win, state->xvi.visual,
			state->cmap);
	if (!state->draw) {
		fprintf(stderr, "Couldn't create Xft draw context\n");
		goto err_free_gc;
	}

	/*
	xrc.alpha = 0xffff;
	xrc.red = 0xeeee;
	xrc.green = 0xeeee;
	xrc.blue = 0xecec;
	*/
	xrc = TEXT_COLOR;
	if (!XftColorAllocValue(state->dpy, state->xvi.visual, state->cmap,
				&xrc, &state->textclr)) {
		fprintf(stderr, "Couldn't allocate Xft color\n");
		goto err_destroy_draw;
	}

	state->font = XftFontOpenName(state->dpy, DefaultScreen(state->dpy),
			TEXT_FONT);
	if (!state->font) {
		fprintf(stderr, "Couldn't load Xft font\n");
		goto err_free_color;
	}
	return 0;


err_free_color:
	XftColorFree(state->dpy, state->xvi.visual, state->cmap, &state->textclr);
err_destroy_draw:
	XftDrawDestroy(state->draw);
err_free_gc:
	XFreeGC(state->dpy, state->gc);
	return 1;
}

/*
 * Tears down drawing context
 */
void cleanup_draw(struct kbd_state *state)
{
	XftColorFree(state->dpy, state->xvi.visual, state->cmap, &state->textclr);
	XftDrawDestroy(state->draw);
	XFreeGC(state->dpy, state->gc);
}

/*
 * Gets the centroid of the touch points
 */
void get_centroid(struct kbd_state *state, double *cx, double *cy)
{
	int i;
	int n;
	double tx, ty;

	n = 0;
	tx = ty = 0;
	for (i = 0; i < state->ntouches; i++) {
		if (state->touches[i].id < 0)
			continue;
		tx += state->touches[i].x;
		ty += state->touches[i].y;
		n++;
	}
	*cx = tx / n;
	*cy = ty / n;
}

/*
 * Gets the center of the bounding box of the touch points
 */
void get_bbox_center(struct kbd_state *state, double *cx, double *cy)
{
	int i;
	double xmin, xmax, ymin, ymax;

	xmin = ymin = 9999999;
	xmax = ymax = 0;
	for (i = 0; i < state->ntouches; i++) {
		if (state->touches[i].id < 0)
			continue;

		if (state->touches[i].x < xmin)
			xmin = state->touches[i].x;
		if (state->touches[i].x > xmax)
			xmax = state->touches[i].x;
		if (state->touches[i].y < ymin)
			ymin = state->touches[i].y;
		if (state->touches[i].y > ymax)
			ymax = state->touches[i].y;
	}

	*cx = (xmin + xmax) / 2;
	*cy = (ymin + ymax) / 2;
}

/*
 * Draws the window
 */
void update_display(struct kbd_state *state)
{
	int i;
	double cx, cy;
	int touches;
	char str[256];
	Screen *scr = DefaultScreenOfDisplay(state->dpy);
	int sheight = HeightOfScreen(scr);

	XClearWindow(state->dpy, state->win);

	// Draw touches
	XSetForeground(state->dpy, state->gc, TOUCH_COLOR);
	touches = 0;
	for (i = 0; i < state->ntouches; i++) {
		if (state->touches[i].id < 0)
			continue;
		touches++;
		XFillArc(state->dpy, state->win, state->gc,
				state->touches[i].x - TOUCH_RADIUS,
				state->touches[i].y - TOUCH_RADIUS,
				2 * TOUCH_RADIUS, 2 * TOUCH_RADIUS,
				0, 360 * 64);
	}

	// Print calculated data
	i = snprintf(str, 256, "Touches: %d", touches);
	XftDrawString8(state->draw, &state->textclr, state->font, 0, sheight - 10,
			(XftChar8 *) str, i);

	if (!touches)
		return;

	get_centroid(state, &cx, &cy);

	XSetForeground(state->dpy, state->gc, ANALYSIS_COLOR);
	XFillRectangle(state->dpy, state->win, state->gc,
			cx - CENTER_RADIUS, cy - CENTER_RADIUS,
			2 * CENTER_RADIUS, 2 * CENTER_RADIUS);

	i = snprintf(str, 256, "C: (%.1f, %.1f)", cx / touches, cy / touches);
	XftDrawString8(state->draw, &state->textclr, state->font, 0, sheight - 60,
			(XftChar8 *) str, i);
}

/*
 * Find the index of a given touch ID in the internal array
 */
int get_touch_index(struct kbd_state *state, int id)
{
	int i;
	for (i = 0; i < state->ntouches; i++)
		if (state->touches[i].id == id)
			return i;
	return -1;
}

/*
 * Records a touch and its info
 */
int add_touch(struct kbd_state *state, int id, double x, double y)
{
	// Find an empty slot
	int i = get_touch_index(state, -1);
	// Should always have allocated enough slots for device max
	assert(i >= 0);

	// Fill it out
	state->touches[i].id = id;
	state->touches[i].x = x;
	state->touches[i].y = y;
	return 0;
}

/*
 * Removes a touch record
 */
void remove_touch(struct kbd_state *state, int idx)
{
	state->touches[idx].id = -1;
}

/*
 * Event handling for XInput generic events
 */
int handle_xi_event(struct kbd_state *state, XIDeviceEvent *ev)
{
	int idx;

	switch (ev->evtype) {
		case XI_TouchBegin:
			// Bring window to top if it isn't
			XRaiseWindow(state->dpy, state->win);

			// Claim the touch event
			XIAllowTouchEvents(state->dpy, state->input_dev,
					ev->detail, ev->event, XIAcceptTouch);

			// Find and record which button was touched
			if (add_touch(state, ev->detail, ev->event_x, ev->event_y))
				return 1;
			break;

		case XI_TouchEnd:
			// Find which touch was released
			idx = get_touch_index(state, ev->detail);
			// Should always have recorded this touch
			assert(idx >= 0);

			// Update touch tracking
			remove_touch(state, idx);
			break;

		case XI_TouchUpdate:
			idx = get_touch_index(state, ev->detail);
			// Should always have recorded this touch
			assert(idx >= 0);

			// Update touch position
			state->touches[idx].x = ev->event_x;
			state->touches[idx].y = ev->event_y;
			break;

		default:
			fprintf(stderr, "other event %d\n", ev->evtype);
			break;
	}
	update_display(state);
	return 0;
}

/*
 * Main event handling loop
 */
int event_loop(struct kbd_state *state)
{
	XEvent ev;
	XGenericEventCookie *cookie = &ev.xcookie;

	while (!state->shutdown && XNextEvent(state->dpy, &ev) == Success) {
		if (ev.type == GenericEvent &&
				cookie->extension == state->xi_opcode &&
				XGetEventData(state->dpy, cookie)) {
			// GenericEvent from XInput
			handle_xi_event(state, cookie->data);
			XFreeEventData(state->dpy, cookie);
		} else {
			// Regular event type
			switch (ev.type) {
				case MappingNotify:
					XRefreshKeyboardMapping(&ev.xmapping);
					break;
				default:
					fprintf(stderr, "regular event %d\n", ev.type);
			}
		}
	}

	return 0;
}

/*
 * Program entry point
 */
int main(int argc, char **argv)
{
	int ret = 0;

	struct kbd_state state;
	state.shutdown = 0;

	// Open display
	state.dpy = XOpenDisplay(NULL);
	if (!state.dpy) {
		fprintf(stderr, "Could not open display\n");
		return 1;
	}

	// Ensure we have XInput...
	int event, error;
	if (!XQueryExtension(state.dpy, "XInputExtension", &state.xi_opcode,
				&event, &error)) {
		ret = 1;
		fprintf(stderr, "Server does not support XInput\n");
		goto out_close;
	}

	// ... in particular, XInput version 2.2
	int major = 2, minor = 2;
	XIQueryVersion(state.dpy, &major, &minor);
	if (major * 1000 + minor < 2002) {
		ret = 1;
		fprintf(stderr, "Server does not support XInput 2.2\n");
		goto out_close;
	}

	// Get a specific device if given, otherwise find anything capable of
	// direct-style touch input
	int id = (argc > 1) ? atoi(argv[1]) : XIAllDevices;
	ret = init_touch_device(&state, id);
	if (ret)
		goto out_close;

	// Get visual and colormap for transparent windows
	ret = !XMatchVisualInfo(state.dpy, DefaultScreen(state.dpy),
				32, TrueColor, &state.xvi);
	if (ret) {
		fprintf(stderr, "Couldn't find 32-bit visual\n");
		goto out_destroy_touch;
	}

	state.cmap = XCreateColormap(state.dpy, DefaultRootWindow(state.dpy),
			state.xvi.visual, AllocNone);

	// Create main window and keyboard buttons
	ret = create_window(&state);
	if (ret) {
		fprintf(stderr, "Failed to create windows\n");
		goto out_free_cmap;
	}

	// Set up a GC and Xft stuff
	ret = setup_draw(&state);
	if (ret)
		goto out_destroy_window;

	// Display the window
	map_window(&state);
	update_display(&state);

	ret = event_loop(&state);

	// Clean everything up
out_destroy_window:
	destroy_window(&state);
out_free_cmap:
	XFreeColormap(state.dpy, state.cmap);
out_destroy_touch:
	destroy_touch_device(&state);
out_close:
	XCloseDisplay(state.dpy);

	return ret;
}
