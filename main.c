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
				/* Retile windows */
				tile();
				break;
			/* User released a button*/
			case ButtonRelease:
				switch (event.xkey.keycode) {
					case KEY_ESCAPE: goto quit;
					case KEY_K: run("/bin/konsole"); break;
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
	uint8_t count = listCount(list);
	uint32_t width = XDisplayWidth(display, XDefaultScreen(display))/count;
	uint32_t height = XDisplayHeight(display, XDefaultScreen(display));
	Window window;

	for (uint8_t i = 0; i < count; i++) {
		window = list[i];
		XMoveResizeWindow(display, window, i*width, 0, width, height);
		XMapWindow(display, window);
	}
}
