/* series.c: Functions for finding and reading the data files.
 *
 * Copyright (C) 2001 by Brian Raiter, under the GNU General Public
 * License. No warranty. See COPYING for details.
 */

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<ctype.h>
#include	"defs.h"
#include	"err.h"
#include	"fileio.h"
#include	"solution.h"
#include	"series.h"

/* The signature bytes of the data files.
 */
#define	CCSIG			0x0002AAACUL
#define	CCSIG_RULESET_MS	0x00
#define	CCSIG_RULESET_LYNX	0x01

/* Mini-structure for our findfiles() callback.
 */
typedef	struct seriesdata {
    gameseries *list;		/* the gameseries list */
    int		allocated;	/* number of gameseries currently allocated */
    int		count;		/* number of gameseries filled in */
} seriesdata;

/* The path of the directory containing the data files.
 */
char   *datadir = NULL;

/*
 * File I/O functions, with error-handling specific to the data files.
 */

/* Read an unsigned 8-bit number.
 */
static int datfilereadint8(fileinfo *file, int *val, int *size,
			    unsigned char min, unsigned char max)
{
    unsigned char	val8;

    if (*size < 1)
	return fileerr(file, "invalid metadata in data file");
    if (!filereadint8(file, &val8, "invalid data in data file"))
	return FALSE;
    *val = (int)val8;
    --*size;
    if (val8 < min || val8 > max)
	return fileerr(file, "invalid data in data file");
    return TRUE;
}

/* Read an unsigned 16-bit number.
 */
static int datfilereadint16(fileinfo *file, int *val, int *size,
			    unsigned short min, unsigned short max)
{
    unsigned short	val16;

    if (*size < 2)
	return fileerr(file, "invalid metadata in data file");
    if (!filereadint16(file, &val16, "invalid data in data file"))
	return FALSE;
    *val = (int)val16;
    *size -= 2;
    if (val16 < min || val16 > max)
	return fileerr(file, "invalid data in data file");
    return TRUE;
}

/* Read a section of the data file into an allocated buffer.
 */
static int datfilereadbuf(fileinfo *file, unsigned char **buf, int bufsize,
			  int *size)
{
    if (*size < bufsize)
	return fileerr(file, "invalid metadata in data file");
    if (!(*buf = filereadbuf(file, bufsize, "invalid data in data file")))
	return FALSE;
    *size -= bufsize;
    return TRUE;
}

/*
 * The parts of the data file.
 */

/* Examine the top of the game file. FALSE is returned if the header
 * bytes appear to be invalid.
 */
static int readseriesheader(gameseries *series)
{
    unsigned long	sig;
    unsigned short	total;

    if (!filereadint32(&series->mapfile, &sig, "not a valid data file"))
	return FALSE;
    if ((sig & 0x00FFFFFF) != CCSIG) {
	fileerr(&series->mapfile, "not a valid data file");
	return FALSE;
    }
    switch (sig >> 24) {
      case CCSIG_RULESET_MS:	series->ruleset = Ruleset_MS;	break;
      case CCSIG_RULESET_LYNX:	series->ruleset = Ruleset_Lynx;	break;
      default:
	fileerr(&series->mapfile, "unrecognized header in data file");
	return FALSE;
    }
    if (!filereadint16(&series->mapfile, &total, "not a valid data file"))
	return FALSE;
    series->total = total;
    if (!series->total) {
	fileerr(&series->mapfile, "file contains no maps");
	return FALSE;
    }

    return TRUE;
}

/* Read a single level out of the data file.
 */
int readlevelmap(fileinfo *file, gamesetup *game)
{
    unsigned char      *data;
    unsigned short	val16;
    int			levelsize, id, size, i;

    if (!filereadint16(file, &val16, NULL))
	return FALSE;
    levelsize = val16;
    if (!datfilereadint16(file, &game->number, &levelsize, 1, 999)
		|| !datfilereadint16(file, &game->time, &levelsize, 0, 999)
		|| !datfilereadint16(file, &game->chips, &levelsize, 0, 999))
	return FALSE;

    while (levelsize) {
	if (!datfilereadint8(file, &id, &levelsize, 1, 10))
	    return FALSE;
	if (id == 1) {
	    if (!datfilereadint8(file, &i, &levelsize, 0, 0)
			|| !datfilereadint16(file, &game->map1size,
						&levelsize, 0, 1024)
			|| !datfilereadbuf(file, &game->map1, game->map1size,
						&levelsize)
			|| !datfilereadint16(file, &game->map2size,
						&levelsize, 0, 1024)
			|| !datfilereadbuf(file, &game->map2, game->map2size,
						&levelsize)
			|| !datfilereadint16(file, &i, &levelsize, 0, 65535))
		return FALSE;
	} else {
	    if (!datfilereadint8(file, &size, &levelsize, 0, 255)
			|| !datfilereadbuf(file, &data, size, &levelsize))
		return FALSE;
	    switch (id) {
	      case 3:
		memcpy(game->name, data, size);
		game->name[size - 1] = '\0';
		break;
	      case 4:
		game->trapcount = size / 10;
		for (i = 0 ; i < game->trapcount ; ++i) {
		    game->traps[i].from = data[i * 10 + 0]
					+ data[i * 10 + 2] * CXGRID;
		    game->traps[i].to   = data[i * 10 + 4]
					+ data[i * 10 + 6] * CXGRID;
		}
		break;
	      case 5:
		game->clonercount = size / 8;
		for (i = 0 ; i < game->clonercount ; ++i) {
		    game->cloners[i].from = data[i * 8 + 0]
					  + data[i * 8 + 2] * CXGRID;
		    game->cloners[i].to   = data[i * 8 + 4]
					  + data[i * 8 + 6] * CXGRID;
		}
		break;
	      case 6:
		for (i = 0 ; i < size - 1 ; ++i)
		    game->passwd[i] = data[i] ^ 0x99;
		game->passwd[i] = '\0';
		break;
	      case 7:
		memcpy(game->hinttext, data, size);
		game->hinttext[size - 1] = '\0';
		break;
	      case 10:
		game->creaturecount = size / 2;
		for (i = 0 ; i < game->creaturecount ; ++i)
		    game->creatures[i] = data[i * 2 + 0]
				       + data[i * 2 + 1] * CXGRID;
		break;
	      default:
		return fileerr(file, "unrecognized field in data file");
	    }
	}
    }
    if (levelsize)
	warn("Level %d: %d bytes left over!", game->number, levelsize);
    return TRUE;
}

/*
 * Functions to read the data files.
 */

/* Read the game file corresponding to series, and the corresponding
 * solutions in the solution file, until at least level maps have been
 * successfully parsed, or the end of the data file is reached. The
 * files are opened if they have not been already. Nothing is done if
 * the requested level is already in memory.
 */
int readlevelinseries(gameseries *series, int level)
{
    int	n;

    if (level < 0)
	return FALSE;
    if (series->count > level)
	return TRUE;

    if (!series->allmapsread) {
	if (!series->mapfile.fp) {
	    if (!openfileindir(&series->mapfile, datadir, series->mapfile.name,
			       "rb", "unknown error"))
		return FALSE;

	    if (!readseriesheader(series))
		return FALSE;
	    if (*savedir) {
		if (openfileindir(&series->solutionfile, savedir,
				  series->mapfile.name, "rb", NULL)) {
		    if (!readsolutionheader(&series->solutionfile,
					    &series->solutionflags))
			return FALSE;
		    if (series->solutionflags != series->ruleset)
			return fileerr(&series->solutionfile,
				       "saved-game file is for a different"
				       " ruleset than the data file");
		    savedirchecked = TRUE;
		    series->solutionsreadonly = TRUE;
		} else {
		    series->solutionflags = series->ruleset;
		}
	    } else {
		clearfileinfo(&series->solutionfile);
	    }
	}
	while (!series->allmapsread && series->count <= level) {
	    while (series->count >= series->allocated) {
		n = series->allocated ? series->allocated * 2 : 16;
		if (!(series->games = realloc(series->games,
					      n * sizeof *series->games)))
		    memerrexit();
		memset(series->games + series->allocated, 0,
		       (n - series->allocated) * sizeof *series->games);
		series->allocated = n;
	    }
	    if (readlevelmap(&series->mapfile,
			     series->games + series->count)) {
		if (!series->allsolutionsread)
		    readsolution(&series->solutionfile,
				 series->games + series->count);
		++series->count;
	    }
	    if (filetestend(&series->mapfile)) {
		fileclose(&series->mapfile, NULL);
		series->allmapsread = TRUE;
	    }
	    if (series->solutionfile.fp && filetestend(&series->solutionfile))
		series->allsolutionsread = TRUE;
	}
    }
    return series->count > level;
}

/* A wrapper around readlevelinseries() that reads the entire file.
 */
int readseriesfile(gameseries *series)
{
    if (series->allmapsread)
	return TRUE;
    if (!(series->games = realloc(series->games,
				  series->total * sizeof *series->games)))
	memerrexit();
    memset(series->games + series->allocated, 0,
	   (series->total - series->allocated) * sizeof *series->games);
    series->allocated = series->total;
    return readlevelinseries(series, series->total - 1);
}

/*
 * Functions to find the data files.
 */

/* A callback function that initializes a gameseries structure for
 * filename and adds it to the list stored under the second argument.
 */
static int getseriesfile(char *filename, void *data)
{
    seriesdata *sdata = (seriesdata*)data;
    gameseries *series;
    int		f;

    while (sdata->count >= sdata->allocated) {
	++sdata->allocated;
	if (!(sdata->list = realloc(sdata->list,
				    sdata->allocated * sizeof *sdata->list)))
	    memerrexit();
    }
    series = sdata->list + sdata->count;
    clearfileinfo(&series->mapfile);
    clearfileinfo(&series->solutionfile);
    series->mapfile.name = filename;
    series->solutionflags = 0;
    series->allmapsread = FALSE;
    series->allsolutionsread = FALSE;
    series->allocated = 0;
    series->count = 0;
    series->games = NULL;
    strncpy(series->name, series->mapfile.name, sizeof series->name - 1);
    series->name[sizeof series->name - 1] = '\0';

    f = FALSE;
    if (openfileindir(&series->mapfile, datadir, series->mapfile.name,
		      "rb", "unknown error")) {
	f = readseriesheader(series);
	fileclose(&series->mapfile, NULL);
    }
    if (f)
	++sdata->count;
    return f ? 1 : 0;
}

/* A callback function to compare two gameseries structures by
 * comparing their filenames.
 */
static int gameseriescmp(void const *a, void const *b)
{
    return strcmp(((gameseries*)a)->mapfile.name,
		  ((gameseries*)b)->mapfile.name);
}

/* Search the game file directory and generate an array of gameseries
 * structures corresponding to the data files found there.
 */
static int getseriesfiles(char const *filename, gameseries **list, int *count)
{
    seriesdata	s;
    int		n;

    s.list = NULL;
    s.allocated = 0;
    s.count = 0;
    if (filename && *filename && haspathname(filename)) {
	if (getseriesfile((char*)filename, &s) <= 0 || !s.count)
	    die("%s: couldn't read data file", filename);
	*datadir = '\0';
	*savedir = '\0';
    } else {
	if (!findfiles(datadir, &s, getseriesfile) || !s.count)
	    die("%s: directory contains no data files", datadir);
	if (filename && *filename) {
	    for (n = 0 ; n < s.count ; ++n) {
		if (!strcmp(s.list[n].name, filename)) {
		    s.list[0] = s.list[n];
		    s.count = 1;
		    n = 0;
		    break;
		}
	    }
	    if (n == s.count)
		die("%s: no such data file", filename);
	}
	if (s.count > 1)
	    qsort(s.list, s.count, sizeof *s.list, gameseriescmp);
    }
    *list = s.list;
    *count = s.count;
    return TRUE;
}

/* Produce an array of strings that describe the available data files.
 */
int createserieslist(char const *preferredfile, gameseries **pserieslist,
		     char ***pptrs, int *pcount, char const **pheader)
{
    static char const  *rulesetname[Ruleset_Count];
    static char		header[64];
    gameseries	       *serieslist;
    char	      **ptrs;
    char	       *textheap;
    int			listsize;
    int			count, used, col, n;

    if (!getseriesfiles(preferredfile, &serieslist, &listsize))
	return FALSE;
    if (!pptrs) {
	*pserieslist = serieslist;
	*pcount = listsize;
	return TRUE;
    }

    col = 21;
    for (n = 0 ; n < listsize ; ++n)
	if (col < (int)strlen(serieslist[n].name))
	    col = strlen(serieslist[n].name);
    if (col > 40)
	col = 40;

    rulesetname[Ruleset_Lynx] = "Lynx";
    rulesetname[Ruleset_MS] = "MS";
    ptrs = malloc(listsize * sizeof *ptrs);
    textheap = malloc(listsize * 64);
    count = 0;
    used = 0;
    for (n = 0 ; n < listsize ; ++n) {
	ptrs[count] = textheap + used;
	used += sprintf(ptrs[count], "%-*.*s  %3d  %-4s", col, col,
				     serieslist[n].name, serieslist[n].total,
				     rulesetname[serieslist[n].ruleset]);
	++used;
	++count;
    }

    *pserieslist = serieslist;
    *pptrs = ptrs;
    *pcount = count;
    if (pheader) {
	sprintf(header, "%-*.*s  Number of levels  Ruleset",
			col - 13, col - 13, "Filename");
	*pheader = header;
    }
    return TRUE;
}

/* Free all memory allocated by createserieslist().
 */
void freeserieslist(char **ptrs, int count)
{
    (void)count;
    free(ptrs[0]);
    free(ptrs);
}