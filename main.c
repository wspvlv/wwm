#include <unistd.h>		/* execl, fork */
#include <stdio.h>		/* fflush, printf */
#include <stdint.h>		/* intmax_t */
#include <stdlib.h>		/* system */
#include <X11/keysym.h>	/*  */
#include <X11/Xlib.h>
#include "list.h"



#define KEY_ESCAPE	9
#define KEY_K		45
#define KEY_H		43
#define run(command, ...)								\
	if (fork() == 0)									\
		execl(command, command, ##__VA_ARGS__, NULL)



static Display*	display;
static Window	root;
static Window*	list;
static XEvent	event;


static void tile();



#define RETURN_NO_DISPLAY	1
#define RETURN_NO_MEMORY	2
int main() {
	/********************
	 *	INITIALIZATION	*
	 ********************/
	/* Open display, return on fail */
	if ((display = XOpenDisplay(NULL)) == NULL) return RETURN_NO_DISPLAY;
	/* Get root window*/
	root = XDefaultRootWindow(display);
	/* Receive key release and map events from windows */
	XSelectInput(display, root, KeyReleaseMask | SubstructureRedirectMask);
	/* Create a list for active windows */
	list = listNew(4, sizeof(Window));
	if (!list) return RETURN_NO_MEMORY;
	/* Grab Super key */
	XGrabKey(display, XKeysymToKeycode(display, XK_Super_L), 0, root, False, GrabModeAsync, GrabModeAsync);
	XGrabKey(display, XKeysymToKeycode(display, XK_Super_R), 0, root, False, GrabModeAsync, GrabModeAsync);

	/********************
	 *	PROGRAM LOOP	*
	 ********************/
	for (;;) {
		/* Loop through each event on each iteration, if there are any */
		XNextEvent(display, &event);
		/* Determine what is the type of current event */
		switch (event.type) {
			/* If a program tried to map a window */
			case MapRequest:
				/* Add this window to the list of tiled windows */
				listAppend(list, event.xmaprequest.window);
				/* list[listCount(list)] = event.xmaprequest.window;
				listCount(list)++; */
				/* Retile windows */
				tile();
				break;
			/* User released a button */
			case KeyRelease:
				/* Only check keys if Super is pressed */
				if (event.xkey.state == Mod4Mask) {
					/* Determine which button was released */
					KeySym key = XKeycodeToKeysym(display, event.xkey.keycode, 0);
					switch (key) {
						case XK_Escape: goto quit;
						case XK_k: run("/bin/konsole"); break;
					}
				}
				break;
		}
	}

	/***************
	 * Termination *
	 ***************/
quit:
	listDelete(list);
	XCloseDisplay(display);
	return 0;
}

static void tile() {
	const uint_fast8_t count = listCount(list);
	if (count) {
		const int screen = XDefaultScreen(display);
		const unsigned int width = XDisplayWidth(display, screen)/count;
		const unsigned int height = XDisplayHeight(display, screen);
		for (uint_fast8_t i = 0; i < count; i++) {
			XMoveResizeWindow(display, list[i], i*width, 0, width, height);
			XMapWindow(display, list[i]);
		}
	}
}
