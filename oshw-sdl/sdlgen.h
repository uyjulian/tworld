/* sdlgen.h: The internal shared definitions of the SDL OS/hardware layer.
 * 
 * Copyright (C) 2001-2006 by Brian Raiter, under the GNU General Public
 * License. No warranty. See COPYING for details.
 */

#ifndef	_sdlgen_h_
#define	_sdlgen_h_

#include	"SDL.h"
#include	"../gen.h"
#include	"../oshw.h"

/* Structure to hold the definition of a font.
 */
typedef	struct fontinfo {
    signed char		h;		/* height of each character */
    signed char		w[256];		/* width of each character */
    void	       *memory;		/* memory allocated for the font */
    unsigned char      *bits[256];	/* pointers to each glyph */
} fontinfo;

/* Structure to hold a font's colors.
 */
typedef	struct fontcolors { Uint32 c[3]; } fontcolors;

#define	bkgndcolor(fc)	((fc).c[0])	/* the background color */
#define	halfcolor(fc)	((fc).c[1])	/* the antialiasing color */
#define	textcolor(fc)	((fc).c[2])	/* the main color of the glyphs */

/* Flags to the puttext function.
 */
#define	PT_CENTER	0x0100		/* center the text horizontally */
#define	PT_RIGHT	0x0200		/* right-align the text */
#define	PT_MULTILINE	0x0400		/* span lines & break at whitespace */
#define	PT_UPDATERECT	0x0800		/* return the unused area in rect */
#define	PT_CALCSIZE	0x1000		/* determine area needed for text */
#define	PT_DIM		0x2000		/* draw using the dim text color */
#define	PT_HILIGHT	0x4000		/* draw using the bold text color */
#define PT_SKIPLINES(n)	((n) & 0x00FF)	/* don't render the first n lines */

/*
 * Values global to this module. All the globals are placed in here,
 * in order to minimize pollution of the main module's namespace.
 */

typedef	struct oshwglobals
{
    /* 
     * Shared variables.
     */

    short		wtile;		/* width of one tile in pixels */
    short		htile;		/* height of one tile in pixels */
    short		cptile;		/* size of one tile in pixels */
    fontcolors		textclr;	/* color triplet for normal text */
    fontcolors		dimtextclr;	/* color triplet for dim text */
    fontcolors		hilightclr;	/* color triplet for bold text */
    SDL_Window         *window;     /* the window */
    SDL_Renderer         *renderer;     /* the renderer */
    SDL_Surface	       *screen;		/* the surface */
    SDL_Texture        *texture;     /* the texture */
    fontinfo		font;		/* the font */
    int keyboardRepeatEnabled;
} oshwglobals;

/* oshw's structure of globals.
 */
extern oshwglobals sdlg;


/* 
 * Shared functions.
 */

/* Process all pending events. If wait is TRUE and no events are
 * currently pending, the function blocks until an event arrives.
 */
extern void eventupdate(int wait);

/* A callback function, to be called every time a keyboard key is
 * pressed or released. scancode is an SDL key symbol. down is
 * TRUE if the key was pressed or FALSE if it was released.
 */
extern void keyeventcallback(int scancode, int down);

/* A callback function, to be called when a mouse button is
 * pressed or released. xpos and ypos give the mouse's location.
 * button is the number of the mouse button. down is TRUE if the
 * button was pressed or FALSE if it was released.
 */
extern void mouseeventcallback(int xpos, int ypos, int button, int down);

/* Given a pixel's coordinates, return an integer identifying the
 * tile on the map view display under that pixel, or -1 if the
 * pixel is not within the map view.
 */
extern int windowmappos(int x, int y);

/* Display a line (or more) of text in the program's font. The
 * text is clipped to area if necessary. If area is taller than
 * the font, the topmost line is used. len specifies the number of
 * characters to render; -1 can be used if text is NUL-terminated.
 * flags is some combination of PT_* flags defined above. When the
 * PT_CALCSIZE flag is set, no drawing is done; instead the w and
 * h fields of area area changed to define the smallest rectangle
 * that encloses the text that would have been rendered. (If
 * PT_MULTILINE is also set, only the h field is changed.) If
 * PT_UPDATERECT is set instead, then the h field is changed, so
 * as to exclude the rectangle that was drawn in.
 */
extern void puttext(SDL_Rect *rect, char const *text, int len, int flags);

/* Determine the widths necessary to display the columns of the
 * given table. area specifies an enclosing rectangle for the
 * complete table. The return value is an array of rectangles, one
 * for each column of the table. The rectangles y-coordinates and
 * heights are taken from area, and the x-coordinates and widths
 * are calculated so as to best render the columns of the table in
 * the given space. The caller has the responsibility of freeing
 * the returned array.
 */
extern SDL_Rect *measuretable(SDL_Rect const *area, tablespec const *table);

/* Draw a single row of the given table. cols is an array of
 * rectangles, one for each column. Each rectangle is altered by
 * the function as per puttext's PT_UPDATERECT behavior. row
 * points to an integer indicating the first table entry of the
 * row to display; upon return, this value is updated to point to
 * the first entry following the row. flags can be set to PT_DIM
 * and/or PT_HIGHLIGHT; the values will be applied to every entry
 * in the row.
 */
extern int drawtablerow(tablespec const *table, SDL_Rect *cols,
             int *row, int flags);

/* Return a pointer to a tile image for the given creature or
 * animation sequence with the specified direction, sub-position,
 * and animation frame.
 */
extern SDL_Surface *getcreatureimage(SDL_Rect *rect,
                      int id, int dir, int moving, int frame);

/* Return a pointer to an image of a cell with the two given
 * tiles. If the top image is transparent, the composite image is
 * created using the overlay buffer. (Thus the caller should be
 * done using the image returned before calling this function
 * again.) timerval should hold the time of the game, for
 * rendering animated cell tiles, or -1 if the game has not
 * started.
 */
extern SDL_Surface *getcellimage(SDL_Rect *rect,
                  int top, int bot, int timerval);
extern void rendercellimage(SDL_Rect *displayloc, SDL_Rect *detrect,
                  int top, int bot, int timerval);
extern void rendercreatureimage(SDL_Rect *displayloc, SDL_Rect *detrect,
                      int id, int dir, int moving, int frame);

/* The initialization functions for the various modules.
 */
extern int _sdltimerinitialize(int showhistogram);
extern int _sdlresourceinitialize(void);
extern int _sdltextinitialize(void);
extern int _sdltileinitialize(void);
extern int _sdlinputinitialize(void);
extern int _sdloutputinitialize(int fullscreen);
extern int _sdlsfxinitialize(int silence, int soundbufsize);
extern void surfacetotexture(void);

#endif
