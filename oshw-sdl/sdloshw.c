/* sdloshw.c: Top-level SDL management functions.
 *
 * Copyright (C) 2001-2006 by Brian Raiter, under the GNU General Public
 * License. No warranty. See COPYING for details.
 */

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
#include	"SDL.h"
#include	"sdlgen.h"
#include	"../err.h"

/* Values global to this library.
 */
oshwglobals	sdlg;

/* This is an automatically-generated file, which contains a
 * representation of the program's icon.
 */
// #include	"ccicon.c"

/* Dispatch all events sitting in the SDL event queue. 
 */
void eventupdate(int wait)
{
    static int	mousevisible = TRUE;
    SDL_Event	event;
    int		x, y;

    if (wait) {
#ifdef __EMSCRIPTEN__
    	int broke = FALSE;
	    for (;;) {
	        SDL_PumpEvents();
	        switch (SDL_PeepEvents(&event, 1, SDL_PEEKEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT)) {
	        case 0:
	            emscripten_sleep(10);
	            break;
	        default:
	            /* Has events */
	            broke = TRUE;
	        }
	        if (broke)
	        	break;
	    }
#else
		SDL_WaitEvent(NULL);
#endif
	}

    while (SDL_PollEvent(&event)) {
	switch (event.type) {
	  case SDL_KEYDOWN:
	    if (event.key.repeat && !(sdlg.keyboardRepeatEnabled)) 
	    	break;
	 //    if (mousevisible) {
		// SDL_GetMouseState(&x, &y);
		// if (windowmappos(x, y) < 0) {
		//     SDL_ShowCursor(SDL_DISABLE);
		//     mousevisible = FALSE;
		// }
	 //    }

	    keyeventcallback(event.key.keysym.scancode, TRUE);
	 //    if (event.key.keysym.unicode
		// 	&& event.key.keysym.unicode != event.key.keysym.sym) {
		// keyeventcallback(event.key.keysym.unicode, TRUE);
		// keyeventcallback(event.key.keysym.unicode, FALSE);
	 //    }
	    break;
	  case SDL_KEYUP:
	    if (event.key.repeat && !(sdlg.keyboardRepeatEnabled)) 
	    	break;
	 //    if (mousevisible) {
		// SDL_GetMouseState(&x, &y);
		// if (windowmappos(x, y) < 0) {
		//     SDL_ShowCursor(SDL_DISABLE);
		//     mousevisible = FALSE;
		// }
	 //    }
	    keyeventcallback(event.key.keysym.scancode, FALSE);
	    break;
	  case SDL_MOUSEBUTTONDOWN:
	  case SDL_MOUSEBUTTONUP:
	 //    if (!mousevisible) {
		// SDL_ShowCursor(SDL_ENABLE);
		// mousevisible = TRUE;
	 //    }
	    mouseeventcallback(event.button.x, event.button.y,
			       event.button.button,
			       event.type == SDL_MOUSEBUTTONDOWN);
	    break;
	 //  case SDL_MOUSEMOTION:
	 //    if (!mousevisible) {
		// SDL_ShowCursor(SDL_ENABLE);
		// mousevisible = TRUE;
	 //    }
	 //    break;
	  case SDL_QUIT:
	    exit(EXIT_SUCCESS);
	}
    }
}

/* Alter the window decoration, translating the subtitle to UTF-8 as
 * necessary.
 */
void setsubtitle(char const *subtitle)
{
    char	buf[512] = "Tile World";
    char       *p;

    if (subtitle && *subtitle) {
	p = buf + strlen(buf);
	*p++ = ' ';
	*p++ = '-';
	*p++ = ' ';
	for ( ; *subtitle ; ++subtitle) {
	    if (*subtitle & 0x80) {
		*p++ = 0xC0 | ((*subtitle >> 6) & 0x03);
		*p++ = 0x80 | (*subtitle & 0x3F);
	    } else {
		*p++ = *subtitle;
	    }
	}
	*p = '\0';
    }
    // SDL_WM_SetCaption(buf, "Tile World");
}

/* Shut down SDL.
 */
static void shutdown(void)
{
    SDL_Quit();
}

/* Initialize SDL, create the program's icon, and then initialize
 * the other modules of the library.
 */
int oshwinitialize(int silence, int soundbufsize,
		   int showhistogram, int fullscreen)
{
    SDL_Surface	       *icon;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
	errmsg(NULL, "Cannot initialize SDL system: %s\n", SDL_GetError());
	return FALSE;
    }
    atexit(shutdown);

    setsubtitle(NULL);

 //    icon = SDL_CreateRGBSurfaceFrom(cciconimage, CXCCICON, CYCCICON,
	// 			    32, 4 * CXCCICON,
	// 			    0x0000FF, 0x00FF00, 0xFF0000, 0);
 //    if (icon) {
	// SDL_WM_SetIcon(icon, cciconmask);
	// SDL_FreeSurface(icon);
 //    } else
	// warn("couldn't create icon surface: %s", SDL_GetError());

    return _sdltimerinitialize(showhistogram)
	&& _sdltextinitialize()
	&& _sdltileinitialize()
	&& _sdlinputinitialize()
	&& _sdloutputinitialize(fullscreen)
	&& _sdlsfxinitialize(silence, soundbufsize);
}

/* The real main().
 */
int main(int argc, char *argv[])
{
    return tworld(argc, argv);
}
