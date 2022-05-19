/**
 *	X[\w]*[^(LOG)]\([^\n]+\)
 *	XLOG($&)
 */



#include <unistd.h>		/* execl, fork */
#include <stdio.h>		/* fflush, printf */
#include <stdint.h>		/* intmax_t */
#include <math.h>		/* fabs */
#include <string.h>
#include <stdarg.h>
#include <X11/keysym.h>	/*  */
#include <X11/Xlib.h>
#include "list.h"


/**
 *	0 -- Do not optimize
 *	1 -- Optimize
 *	2 -- Optimize with complete disregard for other resources
 */
#ifndef WWM_OPTIMIZE_MEMORY
#	define WWM_OPTIMIZE_MEMORY 0
#endif
#ifndef	WWM_OPTIMIZE_SPEED
#	define WWM_OPTIMIZE_SPEED 1
#endif
#ifndef	WWM_OPTIMIZE_SIZE
#	define WWM_OPTIMIZE_SIZE 0
#endif

#define VALID_FOCUS	(focus != (uint_fast8_t)-1)

#if defined(WWM_INLINE_RUN)
static inline void run(const char* command) {
	if (fork() == 0) execl(command, command, NULL);
}
#elif defined(WWM_FUNC_RUN)
static void run(const char* command) {
	if (fork() == 0) execl(command, command, NULL);
}
#else	/* Optimal behavior */
#define run(command)	\
	if (fork() == 0) execl(command, command, NULL);
#endif

/* Branch Prediction */
#ifndef likely
#	if __GNUC__
#		define likely __glibc_likely
#	else
#		define likely(cond)	(cond)
#	endif
#endif

#ifndef unlikely
#	if __GNUC__
#		define unlikely __glibc_unlikely
#	else
#		define unlikely(cond)	(cond)
#	endif
#endif

#define DrawStr(x, y, str) ({							\
	const char* c = (str);								\
	XLOG(XDrawString(display, info, gc, x, y, c, strlen(c)));\
})

#define merge(...) _merge(0,__VA_ARGS__,NULL)
#define XLOG(FUNC) ({														\
	fprintf(file, "@%s@%s@%u: %s\n", __FILE__, __func__, __LINE__, #FUNC);	\
	fflush(file);	\
	FUNC;																	\
})



static Display*		display;
static List*		meta;	/* Metadata of `list` for quick access */
static Window*		list;	/* List of open windows */
static int			screen;
static uint_fast8_t	column;
static uint_fast8_t	row;
static FILE*		file;



static void tile();
static char* itoa(uint_fast32_t n);
/* static char* merge2(const char* a, const char* b); */
static char* _merge(const char unused, ...);




#define RETURN_NO_DISPLAY	1
#define RETURN_NO_MEMORY	2
int main() {
	/********************
	 *	INITIALIZATION	*
	 ********************/
	XEvent		event;
	Window		root;
	Window		info;
	uint_fast8_t focus;	/* Index of the window in focus */
	file = fopen("homo", "w");
	/* Display */
	display = XLOG(XOpenDisplay(NULL));
	if (unlikely(display == NULL)) return RETURN_NO_DISPLAY;
	/* Root Window */
	root = XLOG(XDefaultRootWindow(display));
	/* Screen */
	screen = XLOG(XDefaultScreen(display));
	/* List & Meta */
	list = listNew(4, sizeof(Window));
	if (unlikely(list == NULL)) return RETURN_NO_MEMORY;
	/* Focus */
	focus = (uint_fast8_t)-1;
	/* Receive key release and map events from windows */
	XLOG(XSelectInput(display, root, KeyReleaseMask | SubstructureRedirectMask | SubstructureNotifyMask | FocusChangeMask));
	/* Grab Super key */
	XLOG(XGrabKey(display, XKeysymToKeycode(display, XK_Super_L), 0, root, True, GrabModeAsync, GrabModeAsync));
	XLOG(XGrabKey(display, XKeysymToKeycode(display, XK_Super_R), 0, root, True, GrabModeAsync, GrabModeAsync));


	/* Info window */
	info = XLOG(XCreateSimpleWindow(display, root, 0, 0, 250, 250, 10, 0xFFFFFF, 0xFF0000));
	GC gc = XLOG(XCreateGC(display, info, 0, NULL));
	XLOG(XSetForeground(display, gc, 0xFFFFFF));
	XLOG(XSetBackground(display, gc, 0x0));


	/********************
	 *	PROGRAM LOOP	*
	 ********************/
	for (;;) {
		/* Loop through each event on each iteration, if there are any */
		XLOG(XNextEvent(display, &event));
		/* Determine what is the type of current event */
		switch (event.type) {
			/* If a program tried to map a window */
			case MapRequest:
				fprintf(file, "\tMapRequest\n");
				if (likely(event.xmaprequest.window != info)) {
					/* Add this window to the list of tiled windows */
					listAppend(list, event.xmaprequest.window);
					meta = listMeta(list);	/* Address is subject to change due to expansion */
					/* Tile 'em */
					tile();
					XLOG(XSelectInput(display, event.xmaprequest.window, FocusChangeMask));
				}
				break;
			/* If a program unmapped a window */
			case UnmapNotify:
				break;
			/* Always know what window is in focus */
			case FocusIn:
				fprintf(file, "\tFocusIn\n");
				/* Acquire index in the list for the window in focus */
				if (event.xfocus.window != root && event.xfocus.mode != NotifyPointer) {
						for (focus = 0; focus < meta->count; focus++)
							if (unlikely(event.xfocus.window == list[focus])) goto found;
					focus = (uint_fast8_t)-1;
				}
found:			
				if (event.xfocus.window != root && event.xfocus.mode != NotifyPointer) {
					XLOG(XClearWindow(display, info));
					XLOG(XMapRaised(display, info));
					DrawStr(0, 10, merge("root = ", itoa(root)));
					DrawStr(0, 20, merge("focus = ", itoa(focus)));
					DrawStr(0, 30, merge("event.xfocus.window = ", itoa(event.xfocus.window)));
					DrawStr(0, 40, merge("event.xfocus.detail = ", itoa(event.xfocus.detail)));
					DrawStr(0, 50, merge("list[focus] = ", itoa(list[focus])));
				}
				break;
			/* User released a button */
			case KeyRelease:
				fprintf(file, "\tKeyRelease\n");
				/* Only check keys if Super is pressed */
				if (likely(event.xkey.state == Mod4Mask)) {
					/* Determine which button was released */
					KeySym key = XLOG(XKeycodeToKeysym(display, event.xkey.keycode, 0));
					switch (key) {
						case XK_Escape:
							fprintf(file, "\t\tXK_Escape\n");
							if (likely(VALID_FOCUS)) {
								XLOG(XKillClient(display, list[focus]));
								listClear(list, focus);
								meta = listMeta(list);
								tile();
							}
							break;
						case XK_e: 
							fprintf(file, "\t\tXK_e\n");
							goto quit;
						case XK_r: 
							fprintf(file, "\t\tXK_r\n");
							run("/bin/dmenu_run");
							break;
						case XK_i:
							fprintf(file, "\t\tXK_i\n");
							XLOG(XUnmapWindow(display, info));
							break;
						case XK_f:
							fprintf(file, "\t\tXK_f\n");
							XLOG(XSetInputFocus(display, list[meta->count-1], RevertToNone, CurrentTime));
							break;
						case XK_space:
							fprintf(file, "\t\tXK_space\n");
							run("/bin/konsole");
							break;
						case XK_Left:
							fprintf(file, "\t\tXK_Left\n");
							if (likely(VALID_FOCUS && focus > 0)) {
								Window buf[2] = { list[focus-1], list[focus] };
								list[focus] = buf[0];
								list[focus-1] = buf[1];
								tile();
							}
							break;
						case XK_Right:
							fprintf(file, "\t\tXK_Right\n");
							if (likely(VALID_FOCUS && focus < meta->count-1)) {
								Window buf[2] = { list[focus], list[focus+1] };
								list[focus] = buf[1];
								list[focus+1] = buf[0];
								tile();
							}
							break;
					}
				}
				break;
			}
		}
quit:
	for (uint_fast8_t i = 0; i < meta->count; i++)
		XLOG(XKillClient(display, list[i]));
	listDelete(list);
	XLOG(XCloseDisplay(display));
	fclose(file);
	return 0;
}

/* Tile windows */
static void tile() {
	if (meta->count) {
		const unsigned int swidth = XLOG(XDisplayWidth(display, screen));
		const unsigned int sheight = XLOG(XDisplayHeight(display, screen));
		/* Resize and map */
		unsigned int width, height;
		if (meta->count != 3) {
			/* Here ratio means ratio of window dimensions */
			float factor; 				/* 1:1 ratio factor for currect row count */
			float bestFactor = 1; 		/* best closest to 1:1 ratio factor up until current row count */
			uint_fast8_t bestRow = 1;	/* Current best row count, which provides closest to 1:1 ratio */
			for (row = 1; row <= meta->count; row++) {
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
			row = bestRow;
		}
		else {
			row = 2;
		}
		column = meta->count / row;
		width = swidth / column;
		height = sheight / row;
		for (uint_fast8_t i = 0; i < row; i++) {
			if (i < row-1)
				for (uint_fast8_t j = 0; j < column; j++) {
					const uint_fast8_t index = i*column+j;
					XLOG(XMoveResizeWindow(display, list[index], j*width, i*height, width, height));
					XLOG(XMapWindow(display, list[index]));
				}
			else {
				const uint_fast8_t columnLR = column + meta->count%row;
/* 				file = fopen("homo", "w");
				fprintf(file, "row = %hhu\n", bestRow);
				fprintf(file, "column = %hhu\n", column);
				fprintf(file, "columnLR = %hhu\n", columnLR);
				fprintf(file, "width = %u\n", width); */
				width = swidth / columnLR;
/* 				fprintf(file, "widthLR = %u\n", width);
				fprintf(file, "height = %u\n", height);
				fprintf(file, "i = %hhu\n", i);
				fflush(file);
				fclose(file); */
				for (uint_fast8_t j = 0; j < columnLR; j++) {
					const uint_fast8_t index = i*column+j;
					XLOG(XMoveResizeWindow(display, list[index], j*width, i*height, width, height));
					XLOG(XMapWindow(display, list[index]));
				}
			}
		}
		/* Focus on the last opened window */
		XLOG(XSetInputFocus(display, list[meta->count-1], RevertToPointerRoot, CurrentTime));
	}
}

#include <math.h>
static char* itoa(uint_fast32_t n) {
	if (likely(n > 0)) {
		const uint_fast8_t len = (uint_fast8_t)log10f(n);
		char* const str = malloc(len+2);
		for (uint_fast8_t i = len; n > 0; i--) {
			str[i] = n%10 + '0';
			n /= 10;
		}
		str[len+1] = '\0';
		return str;
	}
	return "0";
}

/* static char* merge2(const char* a, const char* b) {
	const size_t la = strlen(a);
	const size_t lb = strlen(b);
	char* const str = malloc(la+lb+1);
	memcpy(str, a, la);
	memcpy(str+la, b, lb);
	str[la+lb] = '\0';
	free(a);
	free(b);
	return str;
} */

/** [Merge strings]
 *
 */
static char* _merge(const char unused, ...) {
	char** _list = listNew(2, sizeof(char*));

	va_list arg;
	va_start(arg, unused);

	char* str = va_arg(arg, char*);
	while (str != NULL) {
		listAppend(_list, str);
		str = va_arg(arg, char*);
	}

	va_end(arg);

	uint_fast8_t count = listCount(_list);
	size_t len[count];
	size_t lensum = 0;
	char* r;

	for (uint_fast8_t i = 0; i < count; i++) {
		lensum += (len[i] = strlen(_list[i]));
	}
	
	r = malloc(lensum+1);

	uint_fast8_t i;
	uint_fast16_t o;
	for (i = 0, o = 0; i < count; i++) {
		memcpy(r+o, _list[i], len[i]);
		o += len[i];
	}

	r[lensum] = '\0';
	listDelete(_list);
	return r;
}
