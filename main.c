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

#define VALID_FOCUS	(focus < listCount(client))



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

#define CHECK_KEYBINDING(KEYBINDING)	(unlikely ( ((event.xkey.state&(~settings.mod)) == (KEYBINDING).mod) && (event.xkey.keycode == (KEYBINDING).key) ) )

/* #define merge(...) _merge(0,__VA_ARGS__,NULL)
#define XLOG(FUNC) ({														\
	fprintf(file, "@%s@%s@%u: %s\n", __FILE__, __func__, __LINE__, #FUNC);	\
	fflush(file);	\
	FUNC;																	\
}) */



/* Client type */
typedef struct Client {
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
	uint32_t	borderActive:24;	/* Color of active (in focus) window's border */
	uint32_t	borderPassive:24;	/* Color of other windows' border*/
	uint8_t		borderWidth;		/* Border width in pixel */
	uint8_t		mod;				/* Modifier key */
	char*		term;				/* Terminal emulator command */
	char*		menu;				/* Menu command */
	Keybinding	kClose;				/* Close window action keybinding */
	Keybinding	kDown;				/* Move window down action keybinding */
	Keybinding	kLeft;				/* Move window left action keybinding */
	Keybinding	kRight;				/* Move window right action keybinding */
	Keybinding	kMenu;				/* Menu action keybinding */
	Keybinding	kTerm;

	Keybinding 	kQuit;
	Keybinding 	kUp;
} __attribute__((packed)) Settings;



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
#ifndef REGION_INITIALIZATION
	/**
	 * Initialization of global variables 
	 */
	/* Opening display */
	display = XOpenDisplay(NULL);
	if (unlikely(display == NULL)) return RETURN_NO_DISPLAY;
#	if XSYNCHRONIZE
		XSynchronize(display, True);
#	endif
	/* Creating a list for clients */
	client = listNew(4, sizeof(Client));
	if (unlikely(client == NULL)) return RETURN_NO_MEMORY;
	settings = (Settings){
		.borderActive = 0xFFFFFF,
		.borderPassive = 0x000000,
		.borderWidth = 15,
		.mod = XKeysymToKeycode(display, XK_Super_L),
		.term = "/bin/konsole",
		.menu = "/bin/dmenu_run",
		.kClose = { .mod = 0, .key = XKeysymToKeycode(display, XK_Escape) },
		.kDown = { .mod = 0, .key = XKeysymToKeycode(display, XK_Down) },
		.kLeft = { .mod = 0, .key = XKeysymToKeycode(display, XK_Left) },
		.kRight = { .mod = 0, .key = XKeysymToKeycode(display, XK_Right) },
		.kMenu = { .mod = 0, .key = 133 },
		.kTerm = { .mod = 0, .key = XKeysymToKeycode(display, XK_Return) },
		.kQuit = { .mod = 0, .key = XKeysymToKeycode(display, XK_e) },
		.kUp = { .mod = 0, .key = XKeysymToKeycode(display, XK_Up) }
	};

	/**
	 * Declaration and initialization of local variables 
	 */
	/* Event data is retrieved from this variable */
	XEvent		event;
	/* Client process ID intermediate storage for a client pending window creation */
	pid_t		_pid = 0;
	/* Stores the id of the root window */
	const Window root = XDefaultRootWindow(display);
	uint_fast8_t focus = -1;
	uint_fast8_t _focus = -1;
	KeyCode	key = 0;
	
	struct {
		uint8_t mod:1;
		uint8_t modGrub:1;
	} __attribute__((packed)) state = {
		.mod = 0,
		.modGrub = 0
	};

	
	/**
	 * Event handling setup
	 */
	/* Define event-mask for root */
	XSelectInput(display, root, KeyPressMask | KeyReleaseMask | SubstructureRedirectMask | SubstructureNotifyMask | FocusChangeMask);
	/* Grab Super key */
	XGrabKey(display, settings.mod, 0, root, True, GrabModeAsync, GrabModeAsync);
#endif

#ifndef REGION_PROGRAM_LOOP
	for (;;) {
		/* Loop through all events */
		XNextEvent(display, &event);
		switch (event.type) {
			case KeyRelease:
				if (state.mod && event.xkey.keycode == 133) {
					if (!key) run(settings.menu);
					key = 0;
					state.mod = 0;
				}
				/* if (event.xkey.state&settings.mod && key == 0 && mod_lock) {
					if (event.xkey.keycode!=133) key = event.xkey.keycode;
					else key = key ? key : 255;
				} */
				/* else {
					if (event.xkey.keycode==133) key = 0;
				} */
			break;

			case KeyPress:
				if (state.mod) {
					if (event.xkey.keycode != 133) {
						key = event.xkey.keycode;

						if (unlikely(key == settings.kTerm.key))
							_pid = run(settings.term);
						
						if (unlikely(key == settings.kClose.key)) {
							XUnmapWindow(display, client[focus].window);
							kill(client[focus].process, SIGTERM);
							/* listClear(client, focus);
							XSetInputFocus(display, client[focus-1].window, RevertToPointerRoot, CurrentTime);
							tile(); */
						}

						if (unlikely(key == settings.kLeft.key) && focus > 0) {
							const Client buf = client[focus];
							client[focus] = client[focus-1];
							client[focus-=1] = buf;
							tile();
							/* focus--; */
						}

						if (unlikely(key == settings.kRight.key) && focus < listCount(client)-1) {
							const Client buf = client[focus];
							client[focus] = client[focus+1];
							client[focus+=1] = buf;
							tile();
							/* focus++; */
						}

						if (unlikely(key == settings.kUp.key) && focus >= column) {
							const Client buf = client[focus];
							client[focus] = client[focus-column];
							client[focus-=column] = buf;
							tile();
						}

						if (unlikely(key == settings.kDown.key) && focus <= listCount(client)-column-1) {
							const Client buf = client[focus];
							client[focus] = client[focus+column];
							client[focus+=column] = buf;
							tile();
						}

						if(unlikely(key == settings.kQuit.key))
							goto quit;

					}
				}
				else if (event.xkey.keycode == 133)
					state.mod = 1;
			break;

			/* A process requested to map a window */
			case MapRequest:
				/* Initialize a client with a process and Window IDs */
				listAppend(client, ((Client){.window = event.xmaprequest.window, .process = _pid}));
				/* Let this window detect focus changes */
				XSelectInput(display, event.xmaprequest.window, FocusChangeMask);
				XSetWindowBorderWidth(display, event.xmaprequest.window, settings.borderWidth);
				tile();
				XSetInputFocus(display, event.xmaprequest.window, RevertToPointerRoot, CurrentTime);
			break;

			/* When focus changes update focus index */
			case FocusIn:
				if (state.modGrub || event.xfocus.mode==NotifyGrab) {
					if (likely(VALID_FOCUS))
						XSetInputFocus(display, client[focus].window, RevertToPointerRoot, CurrentTime);
					state.modGrub = 0;
				}
				else {
					_focus = listFindP(client, event.xfocus.window);
					if (_focus != (uint_fast8_t)-1) {
						if (likely(VALID_FOCUS)) 
							XSetWindowBorder(display, client[focus].window, settings.borderPassive);
						focus = _focus;
						XSetWindowBorder(display, client[focus].window, settings.borderActive);
					}
					if (unlikely(!VALID_FOCUS && focus != (uint_fast8_t)-1)) {
						focus = listCount(client)-1;
						XSetWindowBorder(display, client[focus].window, settings.borderActive);
					}
				}
			break;

			/* When focus changes update focus index */
			case FocusOut:
				/* if (event.xfocus.mode==NotifyUngrab) state.modGrub = 1; */
			break;

			case UnmapNotify:
				/* Very likely that a window being deleted is the same as the one in focus */
				/* But in other case, find the window in client list */
				if (unlikely(event.xdestroywindow.window != client[focus].window))
					_focus = listFindP(client, event.xdestroywindow.window);
				/* Uninitialize the client */
				if (_focus != (uint_fast8_t)-1) {
					focus = _focus;
					listClear(client, focus);
					XSetInputFocus(display, client[focus-1].window, RevertToPointerRoot, CurrentTime);
					tile();
				}
			break;
		}
	}
#endif

#ifndef REGION_QUIT
quit:
	for (uint_fast8_t i = 0; i < listCount(client); i++)
		kill(client[i].process, SIGTERM);
	listDelete(client);
	XCloseDisplay(display);
	return 0;
#endif

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
			if (i < row-1) {
				for (uint_fast8_t j = 0; j < column; j++) {
					const uint_fast8_t index = i*column+j;
					XMoveResizeWindow(display, client[index].window, j*width, i*height, width-2*settings.borderWidth, height-2*settings.borderWidth);
					XMapWindow(display, client[index].window);
				}
			}
			else {
				const uint_fast8_t columnLR = column + count%row;
				width = swidth / columnLR;
				for (uint_fast8_t j = 0; j < columnLR; j++) {
					const uint_fast8_t index = i*column+j;
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
