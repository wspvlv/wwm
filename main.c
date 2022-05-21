/**
 *	X[\w]*[^(LOG)]\([^\n]+\)
 *	XLOG($&)
 */



#include <math.h>		/* fabs */
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>		/* fflush, printf */
#include <stdint.h>		/* intmax_t */
#include <string.h>
#include <unistd.h>		/* execl, fork */
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



/* #define run(command)	({							\
	const pid_t pid = fork();						\
	if (pid == 0) execl(command, command, NULL);	\
	pid;											\
}) */
static inline pid_t run(const char* restrict const command) {
	const pid_t pid = fork();
	if (pid == 0) execl(command, command, NULL);
	return pid;
}

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

/* #define merge(...) _merge(0,__VA_ARGS__,NULL)
#define XLOG(FUNC) ({														\
	fprintf(file, "@%s@%s@%u: %s\n", __FILE__, __func__, __LINE__, #FUNC);	\
	fflush(file);	\
	FUNC;																	\
}) */



/* Client type */
typedef struct Client {
	uint8_t column;
	uint8_t row;
	pid_t process;
	Window window;
} Client;

/* Keybinding */
typedef struct Keybinding {
	uint_fast8_t mod;	/* Additional modifier keys */
	KeyCode key;
} __attribute__((packed)) Keybinding;

/* Settings */
typedef struct Settings {
#if WWM_OPTIMIZE_MEMORY ==	1
	uint32_t 	borderActive:24;
	uint32_t 	borderPassive:24;
	uint8_t 	borderWidth;
	uint8_t 	mod;	/* Should we support keys that are not key masks as modifier keys? */
	Keybinding	close;
	Keybinding	down;
	Keybinding	left;
	Keybinding	right;
	Keybinding	run;
	Keybinding	quit;
	Keybinding	up;
#else
/* First alignment (Two alignments with 64-bit uint_fast32_t's) */
	uint_fast32_t	borderActive;	/* Color of active (in focus) window's border */
	uint_fast32_t	borderPassive;	/* Color of other windows' border*/
	uint_fast8_t	borderWidth;	/* in pixel */
	uint_fast8_t	mod;			/* Should we support keys that are not key masks as modifier keys? */
/* Second alignment */
	Keybinding 		close;
	Keybinding 		down;
	Keybinding 		left;
	Keybinding 		right;
/* Third alignment */
	Keybinding 		run;
	Keybinding 		quit;
	Keybinding 		up;
	Keybinding		_padding;
#endif
}
#if WWM_OPTIMIZE_MEMORY == 1
	__attribute__((packed))
#endif
Settings;



static Display*		display;
static Client*		client;
static int			screen;
static uint_fast8_t	column;
static uint_fast8_t	row;
static Settings		settings;
/* static FILE*		file; */



static void tile();
/* static char* itoa(uint_fast32_t n);
static char* _merge(const char unused, ...); */




#define RETURN_NO_DISPLAY	1
#define RETURN_NO_MEMORY	2
int main() {
	/* sizeof(Settings); */


	/********************
	 *	INITIALIZATION	*
	 ********************/
	/**
	 * Initialization of global variables 
	 */
	/* Opening display */
	display = XOpenDisplay(NULL);
	if (unlikely(display == NULL)) return RETURN_NO_DISPLAY;
	/* Creating a list for clients */
	client = listNew(4, sizeof(Client));
	if (unlikely(client == NULL)) return RETURN_NO_MEMORY;
	/* File */
	/* file = fopen("homo", "w"); */

	/**
	 * Declaration and initialization of local variables 
	 */
	/* Event data is retrieved from this variable */
	XEvent		event;
	/* Client process ID intermediate storage for a client pending window creation */
	pid_t		_pid = 0;
	/* Stores the id of the root window */
	const Window root = XDefaultRootWindow(display);
	/* Info window id */
	/* Window		info = XCreateSimpleWindow(display, root, 0, 0, 250, 250, 10, 0xFFFFFF, 0xFF0000);
	GC gc = XLOG(XCreateGC(display, info, 0, NULL));
	XLOG(XSetForeground(display, gc, 0xFFFFFF));
	XLOG(XSetBackground(display, gc, 0x0)); */
	/* Index of the client in focus */
	uint_fast8_t focus = -1;

	settings = (Settings){
		.borderActive = 0x00FF00,
		.borderPassive = 0xFF0000,
		.borderWidth = 1,
		.mod = Mod4Mask,
		.close = { .mod = 0, .key = XKeysymToKeycode(display, XK_Escape) },
		.down = { .mod = 0, .key = XKeysymToKeycode(display, XK_Down) },
		.left = { .mod = 0, .key = XKeysymToKeycode(display, XK_Left) },
		.right = { .mod = 0, .key = XKeysymToKeycode(display, XK_Right) },
		.run = { .mod = 0, .key = XKeysymToKeycode(display, XK_space) },
		.quit = { .mod = 0, .key = XKeysymToKeycode(display, XK_e) },
		.up = { .mod = 0, .key = XKeysymToKeycode(display, XK_Up) }
	};
	
	/**
	 * Event handling setup
	 */
	/* Define event-mask for root */
	XSelectInput(display, root, KeyReleaseMask | SubstructureRedirectMask | SubstructureNotifyMask | FocusChangeMask);
	/* Grab Super key */
	XGrabKey(display, /* XKeysymToKeycode(display, XK_Super_L) */AnyKey, settings.mod, root, True, GrabModeAsync, GrabModeAsync);
	XGrabKey(display, /* XKeysymToKeycode(display, XK_Super_R) */AnyKey, settings.mod, root, True, GrabModeAsync, GrabModeAsync);



	/********************
	 *	PROGRAM LOOP	*
	 ********************/
	for (;;) {
		/* Loop through all events */
		XNextEvent(display, &event);
		switch (event.type) {

#			define CHECK_KEYBINDING(KEYBINDING)	(unlikely ( ((event.xkey.state&(KEYBINDING).mod) == (KEYBINDING).mod) && (event.xkey.keycode == (KEYBINDING).key) ) )
			/* A key was released */
			case KeyRelease:
#ifndef OLD_CODE
				/* Mod key should be pressed */
				if (likely((event.xkey.state&settings.mod) == settings.mod)) {
					/* Open konsole */
					if (CHECK_KEYBINDING(settings.run))
						_pid = run("/bin/konsole");
					/* Close client */
					if (CHECK_KEYBINDING(settings.close))
						kill(client[focus].process, SIGTERM);
					/* Move window left */
					if (CHECK_KEYBINDING(settings.left) && likely(VALID_FOCUS && focus > 0)) {
						const Client buf = client[focus];
						client[focus] = client[focus-1];
						client[focus-1] = buf;
						tile();
						focus--;
					}
					/* Move window right */
					if (CHECK_KEYBINDING(settings.right) && likely(VALID_FOCUS && focus < listCount(client)-1)) {
						const Client buf = client[focus];
						client[focus] = client[focus+1];
						client[focus+1] = buf;
						tile();
						focus++;
					}
					/* Quit window manager */
					if (CHECK_KEYBINDING(settings.quit)) goto quit;
				}
#else
				/* Only handle keybindings when Super key is pressed */
				if (likely(event.xkey.state == Mod4Mask)) {
					/* Determine which button was released */
					KeySym key = XKeycodeToKeysym(display, event.xkey.keycode, 0);
					switch (key) {

						/* Win+Space = run a terminal emulator */
						case XK_space: 
							_pid = run("/bin/konsole");
						break;

						/* Win+Escape = close client */
						case XK_Escape:
 							/* XDestroyWindow(display, client[focus].window); */
							kill(client[focus].process, SIGTERM);
						break;

						case XK_Left:
							if (likely(VALID_FOCUS && focus > 0)) {
								const Client buf[2] = { client[focus-1], client[focus] };
								client[focus] = buf[0];
								client[focus-1] = buf[1];
								tile();
								focus--;
							}
						break;
						
						case XK_Right:
							if (likely(VALID_FOCUS && focus < listCount(client)-1)) {
								const Client buf[2] = { client[focus], client[focus+1] };
								client[focus] = buf[1];
								client[focus+1] = buf[0];
								tile();
								focus++;
							}
						break;

						/* Win+E = quit the WM */
						case XK_e: goto quit;
					}
				}
#endif
			break;

			/* A process requested to map a window */
			case MapRequest:
				/* Initialize a client with a process and Window IDs */
				listAppend(client, ((Client){.window = event.xmaprequest.window, .process = _pid}));
				/* Let this window detect focus changes */
				XSelectInput(display, event.xmaprequest.window, FocusChangeMask);
				XSetWindowBorderWidth(display, event.xmaprequest.window, settings.borderWidth);
				tile();
			break;

			/* When focus changes update focus index */
			case FocusIn:
				if (event.xfocus.mode != NotifyGrab) {
					if (event.xfocus.mode == NotifyUngrab && VALID_FOCUS) {
						XSetInputFocus(display, client[focus].window, RevertToParent, CurrentTime);
						break;
					}
					if (VALID_FOCUS) XSetWindowBorder(display, client[focus].window, settings.borderPassive);
					XSetWindowBorder(display, event.xfocus.window, settings.borderActive);
					focus = listFindP(client, event.xfocus.window);
					focus = (VALID_FOCUS) ? focus : listCount(client)-1;
				}
			break;

			/* When focus changes update focus index */
			case FocusOut:
				/* if (event.xfocus.mode == NotifyUngrab && VALID_FOCUS)
					XSetInputFocus(display, client[focus].window, RevertToParent, CurrentTime); */
			break;

			/* If the window is being destroyed */
			case DestroyNotify:
				/* Very likely that a window being deleted is the same as the one in focus */
				/* But in other case, find the window in client list */
				if (event.xdestroywindow.window != client[focus].window)
					focus = listFindP(client, event.xdestroywindow.window);
				/* Uninitialize the client */
				if (VALID_FOCUS) {
					listClear(client, focus);
					tile();
				}
			break;

		}
	}



	/****************
	 *	QUITTING	*
	 ****************/
quit:
	for (uint_fast8_t i = 0; i < listCount(client); i++)
		kill(client[i].process, SIGTERM);
	listDelete(client);
	XCloseDisplay(display);
	return 0;
}

/* Tile windows */
static void tile() {
	/* The value of `count` is used often here, therefore it makes sense to precompute the value, rather than calculating it's location */
	uint_fast8_t count = listMeta(client)->count;

	if (count) {
		const unsigned int swidth = XDisplayWidth(display, screen);
		const unsigned int sheight = XDisplayHeight(display, screen);
		/* Resize and map */
		unsigned int width, height;
		if (count != 3) {
			/* Here ratio means ratio of window dimensions */
			float factor; 				/* 1:1 ratio factor for currect row count */
			float bestFactor = 1; 		/* best closest to 1:1 ratio factor up until current row count */
			uint_fast8_t bestRow = 1;	/* Current best row count, which provides closest to 1:1 ratio */
			for (row = 1; row <= count; row++) {
				column = count / row;
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
		column = count / row;
		width = swidth / column;
		height = sheight / row;
		for (uint_fast8_t i = 0; i < row; i++) {
			if (i < row-1)
				for (uint_fast8_t j = 0; j < column; j++) {
					const uint_fast8_t index = i*column+j;
					client[index].row = i;
					client[index].column = j;
					XMoveResizeWindow(display, client[index].window, j*width, i*height, width-2*settings.borderWidth, height-2*settings.borderWidth);
					XMapWindow(display, client[index].window);
				}
			else {
				const uint_fast8_t columnLR = column + count%row;
				width = swidth / columnLR;
				for (uint_fast8_t j = 0; j < columnLR; j++) {
					const uint_fast8_t index = i*column+j;
					client[index].row = i;
					client[index].column = j;
					XMoveResizeWindow(display, client[index].window, j*width, i*height, width-2*settings.borderWidth, height-2*settings.borderWidth);
					XMapWindow(display, client[index].window);
				}
			}
		}
	}
}

/* #include <math.h>
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
} */

/** [Merge strings]
 *
 */
/* static char* _merge(const char unused, ...) {
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
 */
