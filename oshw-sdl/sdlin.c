/* sdlin.c: Reading the keyboard.
 * 
 * Copyright (C) 2001-2006 by Brian Raiter, under the GNU General Public
 * License. No warranty. See COPYING for details.
 */

#include	<string.h>
#include	"SDL.h"
#include	"sdlgen.h"
#include	"../defs.h"

/* Structure describing a mapping of a key event to a game command.
 */
typedef	struct keycmdmap {
    int		scancode;	/* the key's scan code */
    int		shift;		/* the shift key's state */
    int		ctl;		/* the ctrl key's state */
    int		alt;		/* the alt keys' state */
    int		cmd;		/* the command */
    int		hold;		/* TRUE for repeating joystick-mode keys */
} keycmdmap;

/* Structure describing mouse activity.
 */
typedef struct mouseaction {
    int		state;		/* state of mouse action (KS_*) */
    int		x, y;		/* position of the mouse */
    int		button;		/* which button generated the event */
} mouseaction;

/* The possible states of keys.
 */
enum { KS_OFF = 0,		/* key is not currently pressed */
       KS_ON = 1,		/* key is down (shift-type keys only) */
       KS_DOWN,			/* key is being held down */
       KS_STRUCK,		/* key was pressed and released in one tick */
       KS_PRESSED,		/* key was pressed in this tick */
       KS_DOWNBUTOFF1,		/* key has been down since the previous tick */
       KS_DOWNBUTOFF2,		/* key has been down since two ticks ago */
       KS_DOWNBUTOFF3,		/* key has been down since three ticks ago */
       KS_REPEATING,		/* key is down and is now repeating */
       KS_count
};

/* The complete array of key states.
 */
static char		keystates[SDL_NUM_SCANCODES];

/* The last mouse action.
 */
static mouseaction	mouseinfo;

/* TRUE if direction keys are to be treated as always repeating.
 */
static int		joystickstyle = FALSE;

/* The complete list of key commands recognized by the game while
 * playing. hold is TRUE for keys that are to be forced to repeat.
 * shift, ctl and alt are positive if the key must be down, zero if
 * the key must be up, or negative if it doesn't matter.
 */
static keycmdmap const gamekeycmds[] = {        
    { SDL_SCANCODE_UP,                    0,  0,  0,   CmdNorth,              TRUE },
    { SDL_SCANCODE_LEFT,                  0,  0,  0,   CmdWest,               TRUE },
    { SDL_SCANCODE_DOWN,                  0,  0,  0,   CmdSouth,              TRUE },
    { SDL_SCANCODE_RIGHT,                 0,  0,  0,   CmdEast,               TRUE },
    { SDL_SCANCODE_8,                   0,  0,  0,   CmdNorth,              TRUE },
    { SDL_SCANCODE_4,                   0,  0,  0,   CmdWest,               TRUE },
    { SDL_SCANCODE_2,                   0,  0,  0,   CmdSouth,              TRUE },
    { SDL_SCANCODE_6,                   0,  0,  0,   CmdEast,               TRUE },
    { SDL_SCANCODE_Q,                        0,  0,  0,   CmdQuitLevel,          FALSE },
    { SDL_SCANCODE_P,                        0, +1,  0,   CmdPrevLevel,          FALSE },
    { SDL_SCANCODE_R,                        0, +1,  0,   CmdSameLevel,          FALSE },
    { SDL_SCANCODE_N,                        0, +1,  0,   CmdNextLevel,          FALSE },
    { SDL_SCANCODE_G,                        0, -1,  0,   CmdGotoLevel,          FALSE },
    { SDL_SCANCODE_Q,                       +1,  0,  0,   CmdQuit,               FALSE },
    { SDL_SCANCODE_PAGEUP,               -1, -1,  0,   CmdPrev10,             FALSE },
    { SDL_SCANCODE_P,                        0,  0,  0,   CmdPrev,               FALSE },
    { SDL_SCANCODE_R,                        0,  0,  0,   CmdSame,               FALSE },
    { SDL_SCANCODE_N,                        0,  0,  0,   CmdNext,               FALSE },
    { SDL_SCANCODE_PAGEDOWN,             -1, -1,  0,   CmdNext10,             FALSE },
    { SDL_SCANCODE_BACKSPACE,                      -1, -1,  0,   CmdPauseGame,          FALSE },
    { SDL_SCANCODE_SLASH,			 -1, -1,  0,   CmdHelp,               FALSE },
    { SDL_SCANCODE_F1,                   -1, -1,  0,   CmdHelp,               FALSE },
    { SDL_SCANCODE_O,			  0,  0,  0,   CmdStepping,           FALSE },
    { SDL_SCANCODE_O,			 +1,  0,  0,   CmdSubStepping,        FALSE },
    { SDL_SCANCODE_F,			  0,  0,  0,   CmdRndSlideDir,        FALSE },
    { SDL_SCANCODE_TAB,                       0, -1,  0,   CmdPlayback,           FALSE },
    { SDL_SCANCODE_TAB,                      +1, -1,  0,   CmdCheckSolution,      FALSE },
    { SDL_SCANCODE_X,                        0, +1,  0,   CmdReplSolution,       FALSE },
    { SDL_SCANCODE_X,                       +1, +1,  0,   CmdKillSolution,       FALSE },
    { SDL_SCANCODE_S,                        0,  0,  0,   CmdSeeScores,          FALSE },
    { SDL_SCANCODE_S,			  0, +1,  0,   CmdSeeSolutionFiles,   FALSE },
    { SDL_SCANCODE_V,                       +1,  0,  0,   CmdVolumeUp,           FALSE },
    { SDL_SCANCODE_V,                        0,  0,  0,   CmdVolumeDown,         FALSE },
    { SDL_SCANCODE_RETURN,               -1, -1,  0,   CmdProceed,            FALSE },
    { SDL_SCANCODE_RETURN,             -1, -1,  0,   CmdProceed,            FALSE },
    { SDL_SCANCODE_SPACE,                       -1, -1,  0,   CmdProceed,            FALSE },
    { SDL_SCANCODE_D,                        0,  0,  0,   CmdDebugCmd1,          FALSE },
    { SDL_SCANCODE_D,                       +1,  0,  0,   CmdDebugCmd2,          FALSE },
    { SDL_SCANCODE_UP,                   +1,  0,  0,   CmdCheatNorth,         TRUE },
    { SDL_SCANCODE_LEFT,                 +1,  0,  0,   CmdCheatWest,          TRUE },
    { SDL_SCANCODE_DOWN,                 +1,  0,  0,   CmdCheatSouth,         TRUE },
    { SDL_SCANCODE_RIGHT,                +1,  0,  0,   CmdCheatEast,          TRUE },
    { SDL_SCANCODE_HOME,                 +1,  0,  0,   CmdCheatHome,          FALSE },
    { SDL_SCANCODE_F10,                   0,  0,  0,   CmdCheatStuff,         FALSE },
    { '\003',                    -1, -1,  0,   CmdQuit,               FALSE },
    { SDL_SCANCODE_F4,                    0,  0, +1,   CmdQuit,               FALSE },
    { 0, 0, 0, 0, 0, 0 }
};

/* The list of key commands recognized when the program is obtaining
 * input from the user.
 */
static keycmdmap const inputkeycmds[] = {
    { SDL_SCANCODE_UP,                   -1, -1,  0,   CmdNorth,              FALSE },
    { SDL_SCANCODE_LEFT,                 -1, -1,  0,   CmdWest,               FALSE },
    { SDL_SCANCODE_DOWN,                 -1, -1,  0,   CmdSouth,              FALSE },
    { SDL_SCANCODE_RIGHT,                -1, -1,  0,   CmdEast,               FALSE },
    { SDL_SCANCODE_BACKSPACE,                      -1, -1,  0,   CmdWest,               FALSE },
    { SDL_SCANCODE_SPACE,                       -1, -1,  0,   CmdEast,               FALSE },
    { SDL_SCANCODE_RETURN,               -1, -1,  0,   CmdProceed,            FALSE },
    { SDL_SCANCODE_RETURN,             -1, -1,  0,   CmdProceed,            FALSE },
    { SDL_SCANCODE_ESCAPE,               -1, -1,  0,   CmdQuitLevel,          FALSE },
    { SDL_SCANCODE_A,                       -1,  0,  0,   'a',                   FALSE },
    { SDL_SCANCODE_B,                       -1,  0,  0,   'b',                   FALSE },
    { SDL_SCANCODE_C,                       -1,  0,  0,   'c',                   FALSE },
    { SDL_SCANCODE_D,                       -1,  0,  0,   'd',                   FALSE },
    { SDL_SCANCODE_E,                       -1,  0,  0,   'e',                   FALSE },
    { SDL_SCANCODE_F,                       -1,  0,  0,   'f',                   FALSE },
    { SDL_SCANCODE_G,                       -1,  0,  0,   'g',                   FALSE },
    { SDL_SCANCODE_H,                       -1,  0,  0,   'h',                   FALSE },
    { SDL_SCANCODE_I,                       -1,  0,  0,   'i',                   FALSE },
    { SDL_SCANCODE_J,                       -1,  0,  0,   'j',                   FALSE },
    { SDL_SCANCODE_K,                       -1,  0,  0,   'k',                   FALSE },
    { SDL_SCANCODE_L,                       -1,  0,  0,   'l',                   FALSE },
    { SDL_SCANCODE_M,                       -1,  0,  0,   'm',                   FALSE },
    { SDL_SCANCODE_N,                       -1,  0,  0,   'n',                   FALSE },
    { SDL_SCANCODE_O,                       -1,  0,  0,   'o',                   FALSE },
    { SDL_SCANCODE_P,                       -1,  0,  0,   'p',                   FALSE },
    { SDL_SCANCODE_Q,                       -1,  0,  0,   'q',                   FALSE },
    { SDL_SCANCODE_R,                       -1,  0,  0,   'r',                   FALSE },
    { SDL_SCANCODE_S,                       -1,  0,  0,   's',                   FALSE },
    { SDL_SCANCODE_T,                       -1,  0,  0,   't',                   FALSE },
    { SDL_SCANCODE_U,                       -1,  0,  0,   'u',                   FALSE },
    { SDL_SCANCODE_V,                       -1,  0,  0,   'v',                   FALSE },
    { SDL_SCANCODE_W,                       -1,  0,  0,   'w',                   FALSE },
    { SDL_SCANCODE_X,                       -1,  0,  0,   'x',                   FALSE },
    { SDL_SCANCODE_Y,                       -1,  0,  0,   'y',                   FALSE },
    { SDL_SCANCODE_Z,                       -1,  0,  0,   'z',                   FALSE },
    { '\003',                    -1, -1,  0,   CmdQuit,               FALSE },
    { SDL_SCANCODE_F4,                    0,  0, +1,   CmdQuit,               FALSE },
    { 0, 0, 0, 0, 0, 0 }
};

/* The current map of key commands.
 */
static keycmdmap const *keycmds = gamekeycmds;

/* A map of keys that can be held down simultaneously to produce
 * multiple commands.
 */
static int mergeable[CmdKeyMoveLast + 1];

/*
 * Running the keyboard's state machine.
 */

/* This callback is called whenever the state of any keyboard key
 * changes. It records this change in the keystates array. The key can
 * be recorded as being struck, pressed, repeating, held down, or down
 * but ignored, as appropriate to when they were first pressed and the
 * current behavior settings. Shift-type keys are always either on or
 * off.
 */
static void _keyeventcallback(int scancode, int down)
{
    switch (scancode) {
      case SDL_SCANCODE_LSHIFT:
      case SDL_SCANCODE_RSHIFT:
      case SDL_SCANCODE_LCTRL:
      case SDL_SCANCODE_RCTRL:
      case SDL_SCANCODE_LALT:
      case SDL_SCANCODE_RALT:
      case SDL_SCANCODE_LGUI:
      case SDL_SCANCODE_RGUI:
      case SDL_SCANCODE_NUMLOCKCLEAR:
      case SDL_SCANCODE_CAPSLOCK:
      case SDL_SCANCODE_MODE:
	keystates[scancode] = down ? KS_ON : KS_OFF;
	break;
      default:
	if (scancode < SDL_NUM_SCANCODES) {
	    if (down) {
		keystates[scancode] = keystates[scancode] == KS_OFF ?
						KS_PRESSED : KS_REPEATING;
	    } else {
		keystates[scancode] = keystates[scancode] == KS_PRESSED ?
						KS_STRUCK : KS_OFF;
	    }
	}
	break;
    }
}

/* Initialize (or re-initialize) all key states.
 */
static void restartkeystates(void)
{
    const Uint8      *keyboard;
    int		count, n;

    memset(keystates, KS_OFF, sizeof keystates);
    keyboard = SDL_GetKeyboardState(&count);
    if (count > SDL_NUM_SCANCODES)
	count = SDL_NUM_SCANCODES;
    for (n = 0 ; n < count ; ++n)
	if (keyboard[n])
	    _keyeventcallback(n, TRUE);
}

/* Update the key states. This is done at the start of each polling
 * cycle. The state changes that occur depend on the current behavior
 * settings.
 */
static void resetkeystates(void)
{
    /* The transition table for keys in joystick behavior mode.
     */
    static char const joystick_trans[KS_count] = {
	/* KS_OFF         => */	KS_OFF,
	/* KS_ON          => */	KS_ON,
	/* KS_DOWN        => */	KS_DOWN,
	/* KS_STRUCK      => */	KS_OFF,
	/* KS_PRESSED     => */	KS_DOWN,
	/* KS_DOWNBUTOFF1 => */	KS_DOWN,
	/* KS_DOWNBUTOFF2 => */	KS_DOWN,
	/* KS_DOWNBUTOFF3 => */	KS_DOWN,
	/* KS_REPEATING   => */	KS_DOWN
    };
    /* The transition table for keys in keyboard behavior mode.
     */
    static char const keyboard_trans[KS_count] = {
	/* KS_OFF         => */	KS_OFF,
	/* KS_ON          => */	KS_ON,
	/* KS_DOWN        => */	KS_DOWN,
	/* KS_STRUCK      => */	KS_OFF,
	/* KS_PRESSED     => */	KS_DOWNBUTOFF1,
	/* KS_DOWNBUTOFF1 => */	KS_DOWNBUTOFF2,
	/* KS_DOWNBUTOFF2 => */	KS_DOWN,
	/* KS_DOWNBUTOFF3 => */	KS_DOWN,
	/* KS_REPEATING   => */	KS_DOWN
    };

    char const *newstate;
    int		n;

    newstate = joystickstyle ? joystick_trans : keyboard_trans;
    for (n = 0 ; n < SDL_NUM_SCANCODES ; ++n)
	keystates[n] = newstate[(int)keystates[n]];
}

/*
 * Mouse event functions.
 */

/* This callback is called whenever there is a state change in the
 * mouse buttons. Up events are ignored. Down events are stored to
 * be examined later.
 */
static void _mouseeventcallback(int xpos, int ypos, int button, int down)
{
    if (down) {
	mouseinfo.state = KS_PRESSED;
	mouseinfo.x = xpos;
	mouseinfo.y = ypos;
	mouseinfo.button = button;
    }
}

/* Return the command appropriate to the most recent mouse activity.
 */
static int retrievemousecommand(void)
{
    int	n;

    switch (mouseinfo.state) {
      case KS_PRESSED:
	mouseinfo.state = KS_OFF;
	// if (mouseinfo.button == SDL_BUTTON_WHEELDOWN)
	//     return CmdNext;
	// if (mouseinfo.button == SDL_BUTTON_WHEELUP)
	//     return CmdPrev;
	if (mouseinfo.button == SDL_BUTTON_LEFT) {
	    n = windowmappos(mouseinfo.x, mouseinfo.y);
	    if (n >= 0) {
		mouseinfo.state = KS_DOWNBUTOFF1;
		return CmdAbsMouseMoveFirst + n;
	    }
	}
	break;
      case KS_DOWNBUTOFF1:
	mouseinfo.state = KS_DOWNBUTOFF2;
	return CmdPreserve;
      case KS_DOWNBUTOFF2:
	mouseinfo.state = KS_DOWNBUTOFF3;
	return CmdPreserve;
      case KS_DOWNBUTOFF3:
	mouseinfo.state = KS_OFF;
	return CmdPreserve;
    }
    return 0;
}

/*
 * Exported functions.
 */

/* Wait for any non-shift key to be pressed down, ignoring any keys
 * that may be down at the time the function is called. Return FALSE
 * if the key pressed is suggestive of a desire to quit.
 */
int anykey(void)
{
    int	n;

    resetkeystates();
    eventupdate(FALSE);
    for (;;) {
	resetkeystates();
	eventupdate(TRUE);
	for (n = 0 ; n < SDL_NUM_SCANCODES ; ++n)
	    if (keystates[n] == KS_STRUCK || keystates[n] == KS_PRESSED
					  || keystates[n] == KS_REPEATING)
		return n != 'q' && n != SDL_SCANCODE_ESCAPE;
    }
}

/* Poll the keyboard and return the command associated with the
 * selected key, if any. If no key is selected and wait is TRUE, block
 * until a key with an associated command is selected. In keyboard behavior
 * mode, the function can return CmdPreserve, indicating that if the key
 * command from the previous poll has not been processed, it should still
 * be considered active. If two mergeable keys are selected, the return
 * value will be the bitwise-or of their command values.
 */
int input(int wait)
{
    keycmdmap const    *kc;
    int			lingerflag = FALSE;
    int			cmd1, cmd, n;

    for (;;) {
	resetkeystates();
	eventupdate(wait);

	cmd1 = cmd = 0;
	for (kc = keycmds ; kc->scancode ; ++kc) {
	    n = keystates[kc->scancode];
	    if (!n)
		continue;
	    if (kc->shift != -1)
		if (kc->shift !=
			(keystates[SDL_SCANCODE_LSHIFT] || keystates[SDL_SCANCODE_RSHIFT]))
		    continue;
	    if (kc->ctl != -1)
		if (kc->ctl !=
			(keystates[SDL_SCANCODE_LCTRL] || keystates[SDL_SCANCODE_RCTRL]))
		    continue;
	    if (kc->alt != -1)
		if (kc->alt != (keystates[SDL_SCANCODE_LALT] || keystates[SDL_SCANCODE_RALT]))
		    continue;

	    if (n == KS_PRESSED || (kc->hold && n == KS_DOWN)) {
		if (!cmd1) {
		    cmd1 = kc->cmd;
		    if (!joystickstyle || cmd1 > CmdKeyMoveLast
				       || !mergeable[cmd1])
			return cmd1;
		} else {
		    if (cmd1 <= CmdKeyMoveLast
				&& (mergeable[cmd1] & kc->cmd) == kc->cmd)
			return cmd1 | kc->cmd;
		}
	    } else if (n == KS_STRUCK || n == KS_REPEATING) {
		cmd = kc->cmd;
	    } else if (n == KS_DOWNBUTOFF1 || n == KS_DOWNBUTOFF2) {
		lingerflag = TRUE;
	    }
	}
	if (cmd1)
	    return cmd1;
	if (cmd)
	    return cmd;
	cmd = retrievemousecommand();
	if (cmd)
	    return cmd;
	if (!wait)
	    break;
    }
    if (!cmd && lingerflag)
	cmd = CmdPreserve;
    return cmd;
}

/* Turn key-repeating on and off.
 */
int setkeyboardrepeat(int enable)
{
    sdlg.keyboardRepeatEnabled = enable;
 //    if (enable)
	// return SDL_EnableKeyRepeat(500, 75) == 0;
 //    else
	// return SDL_EnableKeyRepeat(0, 0) == 0;
    return TRUE;
}

/* Turn joystick behavior mode on or off. In joystick-behavior mode,
 * the arrow keys are always returned from input() if they are down at
 * the time of the polling cycle. Other keys are only returned if they
 * are pressed during a polling cycle (or if they repeat, if keyboard
 * repeating is on). In keyboard-behavior mode, the arrow keys have a
 * special repeating behavior that is kept synchronized with the
 * polling cycle.
 */
int setkeyboardarrowsrepeat(int enable)
{
    joystickstyle = enable;
    restartkeystates();
    return TRUE;
}

/* Turn input mode on or off. When input mode is on, the input key
 * command map is used instead of the game key command map.
 */
int setkeyboardinputmode(int enable)
{
    keycmds = enable ? inputkeycmds : gamekeycmds;
    return TRUE;
}

/* Initialization.
 */
int _sdlinputinitialize(void)
{
    sdlg.keyeventcallbackfunc = _keyeventcallback;
    sdlg.mouseeventcallbackfunc = _mouseeventcallback;

    mergeable[CmdNorth] = mergeable[CmdSouth] = CmdWest | CmdEast;
    mergeable[CmdWest] = mergeable[CmdEast] = CmdNorth | CmdSouth;

    setkeyboardrepeat(TRUE);
    // SDL_EnableUNICODE(TRUE);
    return TRUE;
}

/* Online help texts for the keyboard commands.
 */
tablespec const *keyboardhelp(int which)
{
    static char *ingame_items[] = {
	"1-arrows", "1-move Chip",
	"1-2 4 6 8 (keypad)", "1-also move Chip",
	"1-Q", "1-quit the current game",
	"1-Bkspc", "1-pause the game",
	"1-Ctrl-R", "1-restart the current level",
	"1-Ctrl-P", "1-jump to the previous level",
	"1-Ctrl-N", "1-jump to the next level",
	"1-V", "1-decrease volume",
	"1-Shift-V", "1-increase volume",
	"1-Ctrl-C", "1-exit the program",
	"1-Alt-F4", "1-exit the program"
    };
    static tablespec const keyhelp_ingame = { 11, 2, 4, 1, ingame_items };

    static char *twixtgame_items[] = {
	"1-P", "1-jump to the previous level",
	"1-N", "1-jump to the next level",
	"1-PgUp", "1-skip back ten levels",
	"1-PgDn", "1-skip ahead ten levels",
	"1-G", "1-go to a level using a password",
	"1-S", "1-see the scores for each level",
	"1-Tab", "1-playback saved solution",
	"1-Shift-Tab", "1-verify saved solution",
	"1-Ctrl-X", "1-replace existing solution",
	"1-Shift-Ctrl-X", "1-delete existing solution",
	"1-Ctrl-S", "1-see the available solution files",
	"1-O", "1-toggle between even-step and odd-step offset",
	"1-Shift-O", "1-increment stepping offset (Lynx only)",
	"1-V", "1-decrease volume",
	"1-Shift-V", "1-increase volume",
	"1-Q", "1-return to the file list",
	"1-Ctrl-C", "1-exit the program",
	"1-Alt-F4", "1-exit the program"
    };
    static tablespec const keyhelp_twixtgame = { 18, 2, 2, 1,
						 twixtgame_items };

    static char *scorelist_items[] = {
	"1-up down", "1-move selection",
	"1-PgUp PgDn", "1-scroll selection",
	"1-Enter Space", "1-select level",
	"1-Ctrl-S", "1-change solution file",
	"1-Q", "1-return to the last level",
	"1-Ctrl-C", "1-exit the program",
	"1-Alt-F4", "1-exit the program"
    };
    static tablespec const keyhelp_scorelist = { 7, 2, 2, 1, scorelist_items };

    static char *scroll_items[] = {
	"1-up down", "1-move selection",
	"1-PgUp PgDn", "1-scroll selection",
	"1-Enter Space", "1-select",
	"1-Q", "1-cancel",
	"1-Ctrl-C", "1-exit the program",
	"1-Alt-F4", "1-exit the program"
    };
    static tablespec const keyhelp_scroll = { 6, 2, 2, 1, scroll_items };

    switch (which) {
      case KEYHELP_INGAME:	return &keyhelp_ingame;
      case KEYHELP_TWIXTGAMES:	return &keyhelp_twixtgame;
      case KEYHELP_SCORELIST:	return &keyhelp_scorelist;
      case KEYHELP_FILELIST:	return &keyhelp_scroll;
    }

    return NULL;
}
