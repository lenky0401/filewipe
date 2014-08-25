/*
  Licensed under the GNU Public License.
  Copyright (C) 1998-2009 by Thomas M. Vier, Jr. All Rights Reserved.

  wipe is free software.
  See LICENSE for more information.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "config.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

/* assume either ioctl.h or termio.h has struct winsize */
#ifdef HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
# define HAVE_WINSIZE
#endif
#if HAVE_TERMIO_H
# include <termio.h>
# define HAVE_WINSIZE
#endif
#if HAVE_TERMIOS_H
# include <termios.h>
# define HAVE_WINSIZE
#endif

#define __USE_GNU
#include <string.h>

#include "std.h"
#include "main.h"
#include "percent.h"
#include "file.h"
#include "str.h"

extern struct opt_s options;

short p_reported;
size_t oldpathlen, lastpathlen, lastpathlen;

private void percent_line_clear(struct percent_s *p);

/*
  percent_init -- initialize percent data
*/

public void percent_init(struct percent_s *p, const char *name, const size_t bufsize, const long int loop)
{
  unsigned short scrwid = DEFAULT_SWIDTH;
#ifdef HAVE_WINSIZE
  struct winsize ws;
#endif

  p->display = 0;
  p->total_passes = 0;

  if (options.no_file)
    return;

  if ((options.verbose == 1 && bufsize >= PERCENT_ENABLE_SIZE)
      || options.verbose == 2)
    p->display = 1; /* enable percentage reporting */

#ifdef HAVE_WINSIZE
  if (ioctl(1, TIOCGWINSZ, &ws))
    {/* didn't work */}
  else
    scrwid = ws.ws_col;
#endif

  p->cur_pass = 0;
  p->name = (char *) name;
  p->nlen = strnlen(name, PATH_MAX);

  /* length of ": 100% " is 7 */
  if (p->nlen + 7 > scrwid)
    {
      /* truncate to fit screen */
      p->nlen = (scrwid - 10) + 3;
      strncpy(p->name + p->nlen-4, "...", scrwid);
      p->name[p->nlen-1] = 0;
    }

  if (options.statics) p->total_passes = STATIC_PASSES;

  if (!(options.zero || options.custom))
    {
      if (options.random)
	p->total_passes += options.random_loop; // correct? two rand loops per wipe

      p->total_passes *= options.wipe_multiply;
    }
  else
    p->total_passes = 1;

  /* percent_update() should be called after each loop */
  if (loop)
    p->total_passes *= loop;

  if (p->display)
    {
      percent_line_clear(p);
      printf(" \r%s: 0%%", p->name); fflush(stdout); /* display */
    }
}

/*
  percent_line_clear -- clear line
*/

private void percent_line_clear(struct percent_s *p)
{
  char spaces[PATH_MAX+1];

  if (oldpathlen)
    {
      //printf("pathlen == %d\noldpathlen == %d\n", strnlen(pathname, PATH_MAX), oldpathlen);
      memset(spaces, (char) 0x20, oldpathlen);

      spaces[oldpathlen-1] = (char) 0x00;
      printf("\r%s       \r", spaces);
    }

  oldpathlen = p->nlen;
}

/*
  percent_update -- update and display progress
*/

public void percent_update(struct percent_s *p)
{
  int percent;

  percent = (int)(((float) ++p->cur_pass / (float) p->total_passes) * 100);

  printf(" \r%s: %d%%", p->name, percent); fflush(stdout);
}

/*
  percent_done -- called between percent_init()s after reporting
*/

public void percent_done(struct percent_s *p)
{
  fflush(stdout);
  lastpathlen = p->nlen;

  if (!p->display)
    {p->reported = 0; return;}
  else
    {
      p_reported = 1;
      printf("\n"); fflush(stdout);
    }
}

/*
  percent_shutdown -- called by main(), after last percentage report

  this is obsolete, since we print a new line after each item now
*/

public void percent_shutdown(void)
{
  char spaces[PATH_MAX+1];

#ifdef SANITY
  /* can be triggered, eg by lstat() failure when options.verbose > 0 */
  if (options.verbose && p_reported == 0)
    {
      fprintf(stderr, "\nwarning: p_reported == %d\n", p_reported);
    }
#endif

  if (p_reported)
    {
      //printf("pathlen == %d\noldpathlen == %d\n", strnlen(pathname, PATH_MAX), oldpathlen);
      memset(spaces, (char) 0x20, lastpathlen);

      spaces[lastpathlen-1] = (char) 0x00;
      printf("\r%s       \r", spaces);
    }
}
