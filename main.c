#include <unistd.h>		/* execl, fork */
#include <stdio.h>		/* fflush, printf */
#include <stdint.h>		/* intmax_t */
#include <math.h>		/* fabs */
#include <X11/keysym.h>	/*  */
#include <X11/Xlib.h>
#include "list.h"


/**
 *	0 -- Do not optimize
 *	1 -- Optimize
 *	2 -- Optimize with complete disregard for other resources
 */
#define WWM_OPTIMIZE_MEMORY 0
#define WWM_OPTIMIZE_SPEED	1
#define WWM_OPTIMIZE_SIZE	0



#define KEY_ESCAPE	9
#define KEY_K		45
#define KEY_H		43
#define run(command, ...)								\
	if (fork() == 0)									\
		execl(command, command, ##__VA_ARGS__, NULL)



static Display*	display;
static Window	root;	/* Root window */
static Window*	list;	/* List of open windows */
static List*	meta;	/* Metadata of `list` for quick access */
static XEvent	event;
static FILE*	file;


static void tile();
static uint_fast32_t findWindow(Window window);



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
	meta = listMeta(list);
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
				meta = listMeta(list);
				/* list[meta->count] = event.xmaprequest.window;
				meta->count++; */
				tile();
				break;
			/* User released a button */
			case KeyRelease:
				/* Only check keys if Super is pressed */
				if (event.xkey.state == Mod4Mask) {
					/* Determine which button was released */
					KeySym key = XKeycodeToKeysym(display, event.xkey.keycode, 0);
					switch (key) {
						case XK_Escape:
							int index;		/* Index of list entry */
							Window focus;
							/* Here `&index` is used as a dummy pointer */
							XGetInputFocus(display, &focus, &index);
							/* Find focus window in the list */
							index = findWindow(focus);
							/* If it's not on the list, do nothing */
							if ((uint_fast32_t)index != (uint_fast32_t)-1) {
								/* Close the window and retile */
								listClear(list, index);
								XKillClient(display, focus);
								tile();
							}
							break;
						case XK_e: goto quit;
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

/* Square root */
uint_fast16_t sqrtu32(uint_fast32_t z) {
	uint_fast16_t x = 1;
	while (x*x != z)
		x = (x + z/x) / 2;
	return x;
}

/* Tile windows */
static void tile() {
	if (meta->count) {
		const int screen = XDefaultScreen(display);
		const unsigned int swidth = XDisplayWidth(display, screen);
		const unsigned int sheight = XDisplayHeight(display, screen);
		/* Resize and map */
		uint_fast8_t row = 1;
		uint_fast8_t column;
		unsigned int width, height;
		/* Here ratio means ratio of window dimensions */
		float factor; 				/* 1:1 ratio factor for currect row count */
		float bestFactor = 1; 		/* best closest to 1:1 ratio factor up until current row count */
		uint_fast8_t bestRow = 1;	/* Current best row count, which provides closest to 1:1 ratio */
		for (; row <= meta->count; row++) {
			column = meta->count / row;
			width = swidth / column;
			height = sheight / row;
			if (width > height) factor = fabs( 1-((float)height/(float)width) );
			else factor = fabs( 1-((float)width/(float)height) );
			if (factor < bestFactor) {
				bestFactor = factor;
				bestRow = row;
			}
		}
		column = meta->count / bestRow;
		width = swidth / column;
		height = sheight / bestRow;
		for (uint_fast8_t i = 0; i < bestRow; i++) {
			if (i < bestRow-1)
				for (uint_fast8_t j = 0; j < column; j++) {
					const uint_fast8_t index = i*column+j;
					XMoveResizeWindow(display, list[index], j*width, i*height, width, height);
					XMapWindow(display, list[index]);
				}
			else {
				const uint_fast8_t columnLR = column + meta->count%bestRow;
				file = fopen("homo", "w");
				fprintf(file, "row = %hhu\n", bestRow);
				fprintf(file, "column = %hhu\n", column);
				fprintf(file, "columnLR = %hhu\n", columnLR);
				fprintf(file, "width = %u\n", width);
				width = swidth / columnLR;
				fprintf(file, "widthLR = %u\n", width);
				fprintf(file, "height = %u\n", height);
				fprintf(file, "i = %hhu\n", i);
				fflush(file);
				fclose(file);
				for (uint_fast8_t j = 0; j < columnLR; j++) {
					const uint_fast8_t index = i*column+j;
					XMoveResizeWindow(display, list[index], j*width, i*height, width, height);
					XMapWindow(display, list[index]);
				}
			}
		}
	}
	/* Focus on the last opened window */
	XSetInputFocus(display, list[meta->count-1], RevertToPointerRoot, CurrentTime);
}

/* Find window in the list */
static uint_fast32_t findWindow(Window window) {
	for (uint_fast8_t i = 0; i < meta->count; i++) {
		if (list[i] == window) return i;
	}
	return -1;
}
