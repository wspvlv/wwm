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



#define run(command)	({							\
	const pid_t pid = fork();						\
	if (pid == 0) execl(command, command, NULL);	\
	pid;											\
})

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



/* Client type */
typedef struct Client {
	pid_t	process;
	Window	window;
	char*	name;
	size_t	nameLen;
} Client;

/* Keybinding */
typedef struct Keybinding {
	uint_fast8_t mod;	/* Additional modifier keys */
	KeyCode key;
} __attribute__((packed)) Keybinding;

/* Settings */
typedef struct Settings {
	/* Aesthetics */
	uint32_t	borderActive:24;	/* Color of active (in focus) window's border */
	uint32_t	borderPassive:24;	/* Color of other windows' border*/
	uint8_t		borderWidth;		/* Border width in pixels */
	uint8_t		gap;				/* Gap width in pixels */
	uint8_t		barWidth;			/* Window bar width*/
	/* Behavior */
	_Bool		loopH:1;			/* Whether moving windows left from first position on the row or right from the last position will move them to the opposite end of the row */
	_Bool		loopV:1;			/* Whether moving windows up from first row or down from last row will move them to to last or first row respectively */
	_Bool		shiftH:1;			/* Whether moving windows left from first position on the row or right from the last position will move them to the opposite end of the row but also shift them one row up or down respectively */
	_Bool		shiftV:1;			/* Whether moving windows up from first row or down from last row will move them to to last or first row respectively but also shift them one column left or right respectively */
	/* If loop and shift are both on, shifting will be done generally. However moving windows left or up from the first row and column, or right or down from the last row and column would loop them over */
	/* Commands */
	char*		term;				/* Terminal emulator command */
	char*		menu;				/* Menu command */
	/* Keybindings */
	KeyCode		mod;				/* Modifier key */
	uint8_t		mod_mask;			/* Modifier key but as a key mask (e.g. Mod5Mask) */
	Keybinding	kClose;				/* Close window action keybinding */
	Keybinding	kDown;				/* Move window down action keybinding */
	Keybinding	kLeft;				/* Move window left action keybinding */
	Keybinding	kRight;				/* Move window right action keybinding */
	Keybinding	kMenu;				/* Menu action keybinding */
	Keybinding	kTerm;				/* Open terminal */
	Keybinding 	kQuit;
	Keybinding 	kUp;
} __attribute__((packed)) Settings;



static Client*		client;
static uint_fast8_t	column;
static Display*		display;
static GC			gcPassive;
static GC			gcActive;
static Window		root;
static uint_fast8_t	row;
static int			screen;
static Settings		settings;
static unsigned int swidth;
static unsigned int sheight;
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
#	ifdef XSYNCHRONIZE
		XSynchronize(display, True);
#	endif
	/* Creating a list for clients */
	client = listNew(4, sizeof(Client));
	if (unlikely(client == NULL)) return RETURN_NO_MEMORY;
	/* Screen width and height */
	swidth = XDisplayWidth(display, screen);
	sheight = XDisplayHeight(display, screen);
	/* Stores the id of the root window */
	root = XDefaultRootWindow(display);
	/* GCs */
	gcActive = XCreateGC(display, root, 0, NULL);
	XSetForeground(display, gcActive, 0xFFFFFF);
	gcPassive = XCreateGC(display, root, 0, NULL);
	XSetForeground(display, gcPassive, 0x000000);
	/* Configuration value initialization */
	settings = (Settings){
		.borderActive = 0xFFFFFF,
		.borderPassive = 0x800080,
		.borderWidth = 4,
		.barWidth = 16,
		.gap = 8,
		.shiftH = False,
		.shiftV = False,
		.term = "/bin/konsole",
		.menu = "/bin/dmenu_run",
		.mod = XKeysymToKeycode(display, XK_Super_L),
		.kClose = { .mod = 0, .key = XKeysymToKeycode(display, XK_Escape) },
		.kDown = { .mod = 0, .key = XKeysymToKeycode(display, XK_Down) },
		.kLeft = { .mod = 0, .key = XKeysymToKeycode(display, XK_Left) },
		.kRight = { .mod = 0, .key = XKeysymToKeycode(display, XK_Right) },
		.kMenu = { .mod = 0, .key = XKeysymToKeycode(display, XK_Super_L) },
		.kTerm = { .mod = 0, .key = XKeysymToKeycode(display, XK_Return) },
		.kQuit = { .mod = 0, .key = XKeysymToKeycode(display, XK_e) },
		.kUp = { .mod = 0, .key = XKeysymToKeycode(display, XK_Up) }
	};
	/* Compute key mask from our modifier key (branchless switch basically) */
	settings.mod_mask = 
		(settings.mod == XKeysymToKeycode(display, XK_Shift_L) || settings.mod == XKeysymToKeycode(display, XK_Shift_R)) * ShiftMask +
		(settings.mod == XKeysymToKeycode(display, XK_Caps_Lock)) * LockMask +
		(settings.mod == XKeysymToKeycode(display, XK_Control_L) || settings.mod == XKeysymToKeycode(display, XK_Control_R)) * ControlMask +
		(settings.mod == XKeysymToKeycode(display, XK_Alt_L) || settings.mod == XKeysymToKeycode(display, XK_Alt_R)) * Mod1Mask +
		(settings.mod == XKeysymToKeycode(display, XK_Super_L) || settings.mod == XKeysymToKeycode(display, XK_Super_R)) * Mod4Mask +
		(settings.mod == XKeysymToKeycode(display, XK_Num_Lock)) * Mod2Mask;
	/**
	 * Declaration and initialization of local variables 
	 */
	/* Event data is retrieved from this variable */
	XEvent		event;
	/* Client process ID intermediate storage for a client pending window creation */
	pid_t		_pid = 0;
	/* Focus index */
	uint_fast8_t focus = -1;
	/* Focus index intermediate storage */
	uint_fast8_t _focus = -1;
	/* Intermediate buffer for a currently hold key combination */
	Keybinding	key = { .key = 0, .mod = 0 };

	GC gc = XCreateGC(display, root, 0, NULL);
	
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
	/* Ungrab any previous grabs */
	XUngrabKeyboard(display, CurrentTime);
	/* Grab Super key */
	XGrabKey(display, settings.mod, AnyModifier, root, True, GrabModeAsync, GrabModeAsync);
#endif

	XSetForeground(display, gc, 0x80FF80);
	XFillRectangle(display, root, gc, 0, 0, swidth, settings.barWidth);

#ifndef REGION_PROGRAM_LOOP
	for (;;) {
		/* Loop through all events */
		XNextEvent(display, &event);
		switch (event.type) {

			case KeyRelease:
				if (state.mod && event.xkey.keycode == settings.mod) {
					if (key.key == 0) {
						key.key = settings.mod;
						goto surpass;
					}
				}
			break;

			case KeyPress:
				if (state.mod) {
					if (event.xkey.keycode != settings.mod) {
						key.mod = event.xkey.state & (~settings.mod_mask);
						key.key = event.xkey.keycode;
surpass:
						/* Open menu */
						if (unlikely(key.key == settings.kMenu.key && key.mod == settings.kMenu.mod))
							run(settings.menu);

						/* Open terminal */
						if (unlikely(key.key == settings.kTerm.key && key.mod == settings.kTerm.mod))
							_pid = run(settings.term);
						
						/* Close window */
						if (unlikely(key.key == settings.kClose.key && key.mod == settings.kClose.mod))
							kill(client[focus].process, SIGTERM);

#define winRow(index)		(index/column)
#define winColumn(index)	(index%column)

						/* Move window left */
						if (unlikely(key.key == settings.kLeft.key && key.mod == settings.kLeft.mod && focus > 0)) {
							if (likely(settings.shiftH || winColumn(focus) != 0)) {
								const Client buf = client[focus];
								client[focus] = client[focus-1];
								client[focus-=1] = buf;
								tile();
							}
						}

						/* Move window right */
						if (unlikely(key.key == settings.kRight.key && key.mod == settings.kRight.mod && focus < listCount(client)-1)) {
							if (likely(settings.shiftH || (winRow(focus) != row-1 ? winColumn(focus) != column-1 : focus != listCount(client)-1))) {
								const Client buf = client[focus];
								client[focus] = client[focus+1];
								client[focus+=1] = buf;
								tile();
							}
						}

						/* Move window up */
						if (unlikely(key.key == settings.kUp.key && key.mod == settings.kUp.mod)) {
							const Client buf = client[focus];
							if (focus < column) {
								if (settings.shiftV && winColumn(focus) > 0) {
									const uint_fast8_t x = column*(row-1)+(winColumn(focus)-1);
									client[focus] = client[x];
									client[focus=x] = buf;
									tile();
								}
							}
							else {
								client[focus] = client[focus-column];
								client[focus-=column] = buf;
								tile();
							}
						}

						/* Move window down */
						if (unlikely(key.key == settings.kDown.key && key.mod == settings.kDown.mod)) {
							const Client buf = client[focus];
							if (focus > listCount(client)-column) {
								if (settings.shiftV && winColumn(focus) < column-1) {
									const uint_fast8_t x = winColumn(focus)+1;
									client[focus] = client[x];
									client[focus=x] = buf;
									tile();
								}
							}
							else {
								client[focus] = client[focus+column];
								client[focus+=column] = buf;
								tile();
							}
						}

						/* Quit WM */
						if(unlikely(key.key == settings.kQuit.key && key.mod == settings.kQuit.mod))
							goto quit;

					}
				}
				else if (event.xkey.keycode == settings.mod)
					state.mod = 1;
			break;

			/* A process requested to map a window */
			case MapRequest:
				/* Initialize a client with a process and Window IDs */
				listAppend(client, ((Client){
					.window = event.xmaprequest.window, 
					.process = _pid,
				}));
				XFetchName(display, client[listCount(client)-1].window,
				&client[listCount(client)-1].name);
				client[listCount(client)-1].nameLen = strlen(client[listCount(client)-1].name);
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
						if (likely(VALID_FOCUS)) {
							XSetWindowBorder(display, client[focus].window, settings.borderPassive);
							XFillRectangle(display, root, gcPassive, swidth/listCount(client)*focus, 0, swidth/listCount(client), settings.barWidth);
							XDrawString(display, root, gcActive, swidth/listCount(client)*focus, 8, client[focus].name, client[focus].nameLen);
						}
						focus = _focus;
						goto highlight;
					}
					if (unlikely(!VALID_FOCUS && focus != (uint_fast8_t)-1 && listCount(client))) {
						focus = listCount(client)-1;
						goto highlight;
					}
				}
			break;
highlight:
			XSetWindowBorder(display, client[focus].window, settings.borderActive);
			XFillRectangle(display, root, gcActive, swidth/listCount(client)*focus, 0, swidth/listCount(client), settings.barWidth);
			XDrawString(display, root, gcPassive, swidth/listCount(client)*focus, 8, client[focus].name, client[focus].nameLen);
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
				if (_focus < listCount(client)) {
					listClear(client, _focus);
					_focus = likely(_focus!=0) ? _focus-1 : _focus;
					if (listCount(client)) {
						XSetInputFocus(display, client[_focus].window, RevertToPointerRoot, CurrentTime);
						tile();
					}
				}
			break;
		}
	}
#endif

#ifndef REGION_QUIT
quit:
	for (uint_fast8_t i = 0; i < listCount(client); i++) {
		kill(client[i].process, SIGTERM);
		XFree(client[i].name);
	}
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
					XMoveResizeWindow(
						display, 
						client[index].window, 
						j*width + settings.gap, 
						i*height + settings.gap + (i==0)*settings.barWidth, 
						width - 2*(settings.borderWidth+settings.gap), 
						height - 2*(settings.borderWidth+settings.gap) - (i==0)*settings.barWidth
					);
					XMapWindow(display, client[index].window);
					XFillRectangle(display, root, gcPassive, swidth/count*index, 0, swidth/count, settings.barWidth);
					XDrawString(display, root, gcActive, swidth/count*index, 0, client[index].name, client[index].nameLen);
				}
			}
			else {
				const uint_fast8_t columnLR = column + count%row;
				width = swidth / columnLR;
				for (uint_fast8_t j = 0; j < columnLR; j++) {
					const uint_fast8_t index = i*column+j;
					XMoveResizeWindow(
						display, 
						client[index].window, 
						j*width + settings.gap, 
						i*height + settings.gap + (i==0)*settings.barWidth, 
						width - 2*(settings.borderWidth+settings.gap), 
						height - 2*(settings.borderWidth+settings.gap) - (i==0)*settings.barWidth
					);
					XMapWindow(display, client[index].window);
					XFillRectangle(display, root, gcPassive, swidth/count*index, 0, swidth/count, settings.barWidth);
					XDrawString(display, root, gcActive, swidth/count*index, 0, client[index].name, client[index].nameLen);
				}
			}
		}
	}
}
