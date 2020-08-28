
sharedir = ./

ifneq (x$(BUILD_EMSCRIPTEN),x)
CC = emcc

CFLAGS := -Wall -Wextra -O3 -DNDEBUG -s USE_SDL=2 -s ASYNCIFY -g
LDFLAGS := -g -DSAVEDIR="/save" -s ASYNCIFY_WHITELIST='["Emscripten_GLES_SwapWindow", "Emscripten_UpdateWindowFramebuffer", "GLES2_RenderPresent", "SDL_Delay", "SDL_GL_SwapWindow", "SDL_RenderPresent", "SDL_UpdateWindowSurface", "SDL_WaitEvent", "SDL_WaitEventTimeout", "SW_RenderPresent", "anykey", "displayendmessage", "displaygame", "displayinputprompt", "displaylist", "displaytable", "displaytextscroll", "displaytiletable", "drawscreen", "eventupdate", "input", "keyinputcallback", "main", "onlinecontexthelp", "onlinemainhelp", "scorescrollinputcallback", "scrollinputcallback", "scrollinputcallback.1", "selectlevelbypassword", "selectseriesandlevel", "showscores", "showsolutionfiles", "solutionscrollinputcallback", "textscrollinputcallback", "tworld", "waitfortick", "yninputcallback"]' --preload-file data --preload-file res --preload-file sets -Wall -Wextra -s USE_SDL=2 -s ASYNCIFY -s TOTAL_MEMORY=67108864
LOADLIBES := -lidbfs.js

BINARY := tworld.html
endif


ifeq (x$(BUILD_EMSCRIPTEN),x)
CC = clang

CFLAGS := -Wall -Wextra -O2 -DNDEBUG $(shell sdl2-config --cflags)
LDFLAGS := -Wall -Wextra $(shell sdl2-config --libs)
LOADLIBES := 

BINARY := tworld
endif

#
# End of configure section
#

OBJS = \
tworld.o series.o play.o encoding.o solution.o res.o lxlogic.o mslogic.o \
unslist.o messages.o help.o score.o random.o cmdline.o fileio.o err.o \
oshw-sdl/sdloshw.o oshw-sdl/sdltimer.o oshw-sdl/sdlerr.o oshw-sdl/sdlin.o \
oshw-sdl/sdlout.o oshw-sdl/sdltext.o oshw-sdl/sdltile.o oshw-sdl/sdlsfx.o

RESOURCES = tworldres.o

#
# Binaries
#

$(BINARY): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LOADLIBES)

#
# Object files
#

tworld.o   : tworld.c defs.h gen.h err.h fileio.h series.h res.h play.h \
             score.h solution.h messages.h help.h oshw.h cmdline.h ver.h
series.o   : series.c series.h defs.h gen.h err.h fileio.h solution.h \
             messages.h unslist.h
play.o     : play.c play.h defs.h gen.h err.h state.h oshw.h fileio.h \
             res.h logic.h encoding.h solution.h random.h
encoding.o : encoding.c encoding.h defs.h gen.h err.h state.h
solution.o : solution.c solution.h defs.h gen.h err.h fileio.h series.h
res.o      : res.c res.h defs.h gen.h err.h oshw.h fileio.h unslist.h
lxlogic.o  : lxlogic.c logic.h defs.h gen.h err.h state.h random.h
mslogic.o  : mslogic.c logic.h defs.h gen.h err.h state.h random.h
messages.o : messages.c messages.h defs.h gen.h err.h fileio.h
unslist.o  : unslist.c unslist.h gen.h err.h fileio.h res.h solution.h
help.o     : help.c help.h defs.h gen.h state.h oshw.h ver.h comptime.h
score.o    : score.c score.h defs.h gen.h err.h play.h
random.o   : random.c random.h defs.h gen.h
cmdline.o  : cmdline.c cmdline.h
fileio.o   : fileio.c fileio.h defs.h gen.h err.h
err.o      : err.c err.h gen.h oshw.h
oshw-sdl/sdlerr.o   : oshw-sdl/sdlerr.c oshw-sdl/sdlgen.h gen.h oshw.h
oshw-sdl/sdloshw.o  : oshw-sdl/sdloshw.c oshw-sdl/sdlgen.h oshw-sdl/ccicon.c \
                      gen.h oshw.h err.h
oshw-sdl/sdltimer.o : oshw-sdl/sdltimer.c oshw-sdl/sdlgen.h gen.h oshw.h
oshw-sdl/sdltext.o  : oshw-sdl/sdltext.c oshw-sdl/sdlgen.h gen.h oshw.h err.h
oshw-sdl/sdltile.o  : oshw-sdl/sdltile.c oshw-sdl/sdlgen.h gen.h oshw.h err.h \
                      defs.h state.h
oshw-sdl/sdlsfx.o   : oshw-sdl/sdlsfx.c oshw-sdl/sdlgen.h gen.h oshw.h err.h \
                      defs.h state.h
oshw-sdl/sdlin.o    : oshw-sdl/sdlin.c oshw-sdl/sdlgen.h gen.h oshw.h defs.h
oshw-sdl/sdlout.o   : oshw-sdl/sdlout.c oshw-sdl/sdlgen.h gen.h oshw.h err.h \
                      defs.h state.h

#
# Generated files
#

comptime.h:
	echo \#define COMPILE_TIME \"`date '+%Y %b %e %T %Z'`\" > comptime.h

#
# Resources
#

tworldres.o: tworld.ico
	echo SDL_app ICON $^ | windres -o $@

#
# Other
#

install: tworld
	mkdir -p $(sharedir)/sets
	mkdir -p $(sharedir)/data
	mkdir -p $(sharedir)/res
	cp -i sets/*.dac $(sharedir)/sets/.
	cp -i data/*.dat $(sharedir)/data/.
	cp -i res/rc $(sharedir)/res/.
	cp -i res/*.bmp $(sharedir)/res/.
	cp -i res/*.txt $(sharedir)/res/.
	cp -i res/*.wav $(sharedir)/res/.

all: $(BINARY)

clean:
	rm -f $(OBJS) tworld comptime.h config.*
	rm -f tworldres.o $(BINARY) tworld.html.mem tworld.data tworld.js tworld.js.orig.js

spotless:
	rm -f $(OBJS) tworld comptime.h config.* configure
	rm -f tworldres.o $(BINARY) tworld.html.mem tworld.data tworld.js tworld.js.orig.js
