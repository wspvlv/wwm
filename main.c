/* Protocol client libraries */
#define PCL_XLIB	0
#define PCL_XCB		1

/* Determines what Protocol client library will be used */
#ifndef USEPCL
#	define USEPCL	PCL_XLIB
#endif



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

#define run(command, ...)						\
	if (fork() == 0)							\
		execl(command, command, ##__VA_ARGS__, NULL);



static Display*	display;
static Window	root;
static List*	list;
static XEvent	event;



static void tile(List* list);



#define RETURN_NO_DISPLAY	1
int main() {
	/* Open display, return on fail */
	if ((display = XOpenDisplay(NULL)) == NULL)
		return RETURN_NO_DISPLAY;
	/* Get root window*/
	root = XDefaultRootWindow(display);
	/* Receive key release and map events from windows */
	XSelectInput(display, root, ButtonReleaseMask | SubstructureRedirectMask);
	/* Create a list for active windows */
	list = listNew(4, sizeof(Window));
	/**/
	XGrabKey(display, XKeysymToKeycode(display, XStringToKeysym("K")), AnyModifier,
            root, True, GrabModeAsync, GrabModeAsync);
	XGrabKey(display, XKeysymToKeycode(display, XStringToKeysym("Esc")), AnyModifier,
            root, True, GrabModeAsync, GrabModeAsync);

	/* Main loop */
	for (;;) {
		/* Loop through each event on each iteration, if there are any */
		XNextEvent(display, &event);
		/* Determine what is the type of current event */
		switch (event.type) {
			/* If a program tried to map a window */
			case MapRequest:
				/* Add this window to the list of tiled windows */
				listAppend(list, &event.xmaprequest.window);
				/* Retile windows */
				tile(list);
				break;
			case KeyRelease:
				switch (event.xkey.keycode) {
					case KEY_ESCAPE: goto quit;
					case KEY_K: run("/bin/konsole"); break;
					case KEY_H:
						/* XQueryTree(display, root, &unused, &unused, &window, &count);
						fprintf(file, "Amount of Windows: %x\n", count);
						for (unsigned int i = 0; i < count; i++) {
							fprintf(file, "%lx\n", window[i]);
						} */
					break;
				}
				break;
		}
	}

quit:
	XCloseDisplay(display);
	return 0;
}

static void tile(List* list) {
	uint8_t count = listCount(list);
	uint32_t width = XDisplayWidth(display, XDefaultScreen(display))/count;
	uint32_t height = XDisplayHeight(display, XDefaultScreen(display));
	Window window;

	for (uint8_t i = 0; i < count; i++) {
		window = *(Window*)listGet(list, i);
		XMoveResizeWindow(display, window, i*width, 0, width, height);
		XMapWindow(display, window);
	}
}
