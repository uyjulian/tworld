/* play.c: Top-level game-playing functions.
 *
 * Copyright (C) 2001 by Brian Raiter, under the GNU General Public
 * License. No warranty. See COPYING for details.
 */

#include	<stdlib.h>
#include	<assert.h>
#include	"defs.h"
#include	"err.h"
#include	"state.h"
#include	"oshw.h"
#include	"mslogic.h"
#include	"lxlogic.h"
#include	"random.h"
#include	"solution.h"
#include	"play.h"

/* The restarting-PRNG seed value. It doesn't matter what it is, as long
 * as it's always the same.
 */
#define	RESTART_PRNG_SEED	105977040UL

/* The functions used to apply the selected ruleset.
 */
static int (*initgame[])(gamestate*) = {
    NULL, lynx_initgame, ms_initgame
};
static int (*advancegame[])(gamestate*) = {
    NULL, lynx_advancegame, ms_advancegame
};

/* The current state of the current game.
 */
static gamestate	state;

/* Initialize the current state to the starting position of the
 * current level.
 */
int initgamestate(gameseries *series, int level, int replay)
{
    memset(&state, 0, sizeof state);
    state.game = &series->games[level];
    state.ruleset = series->ruleset;
    state.soundeffect = NULL;
    state.lastmove = NIL;

    if (replay) {
	if (!state.game->savedsolution.count)
	    return FALSE;
	state.replay = 0;
	copymovelist(&state.moves, &state.game->savedsolution);
	state.initrndslidedir = state.game->savedrndslidedir;
	restartprng(&state.mainprng, state.game->savedrndseed);
    } else {
	state.replay = -1;
	initmovelist(&state.moves);
	state.initrndslidedir = NIL;
	resetprng(&state.mainprng);
    }
    restartprng(&state.restartprng, RESTART_PRNG_SEED);

    return (*initgame[state.ruleset])(&state);
}

/* Advance the game one tick. cmd is the current keyboard command
 * supplied by the user. The return value is positive if the game was
 * completed successfully, negative if the game ended unsuccessfully,
 * and zero otherwise.
 */
int doturn(int cmd)
{
    action	act;
    int		n;

    state.soundeffect = NULL;
    state.currenttime = gettickcount();
    if (state.replay < 0) {
	if (cmd != CmdPreserve)
	    state.currentinput = cmd;
    } else {
	if (state.replay < state.moves.count) {
	    if (state.currenttime > state.moves.list[state.replay].when)
		die("Replay: Got ahead of saved solution: %d > %d!",
		    state.currenttime, state.moves.list[state.replay].when);
	    if (state.currenttime == state.moves.list[state.replay].when) {
		state.currentinput = state.moves.list[state.replay].dir;
		++state.replay;
	    }
	}
    }

    n = (*advancegame[state.ruleset])(&state);

    if (state.replay < 0 && state.lastmove) {
	act.when = state.currenttime;
	act.dir = state.lastmove;
	addtomovelist(&state.moves, act);
	state.lastmove = NIL;
    }

    if (n)
	return n;
    if (state.game->time)
	if (state.currenttime / TICKS_PER_SECOND >= state.game->time)
	    return -1;

    return 0;
}

/* Move the keyboard in and out of its game-play mode.
 */
int activatekeyboard(int active)
{
    return setkeyboardbehavior(state.ruleset, active);
}

/* Update the display of the current game state.
 */
int drawscreen(void)
{
    int currtime, besttime;

    if (hassolution(state.game)) {
	besttime = (state.game->time ? state.game->time : 999)
				- state.game->besttime / TICKS_PER_SECOND;
	if (besttime == 0)
	     besttime = -1;
    } else
	besttime = 0;

    if (state.game->time)
	currtime = state.game->time - state.currenttime / TICKS_PER_SECOND;
    else
	currtime = -1;

    return displaygame(&state, currtime, besttime);
}

/* Return TRUE if a solution exists for the given level.
 */
int hassolution(gamesetup const *game)
{
    return game->savedsolution.count > 0;
}

/* Compare the most recent solution for the current game with the
 * user's best solution (if any). If this solution beats what's there,
 * replace it. TRUE is returned if the solution was replaced.
 */
int replacesolution(void)
{
    if (hassolution(state.game) && state.currenttime >= state.game->besttime)
	return FALSE;

    state.game->besttime = state.currenttime;
    state.game->savedrndslidedir = state.initrndslidedir;
    state.game->savedrndseed = getinitialseed(&state.mainprng);
    copymovelist(&state.game->savedsolution, &state.moves);
    return TRUE;
}