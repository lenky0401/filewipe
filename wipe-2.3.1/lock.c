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

#include <unistd.h>
#include <stdio.h>

#include "config.h"

#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#else
# undef HAVE_FCNTL
#endif

#include <sys/types.h>
#include <sys/stat.h>

#ifndef HAVE_FCNTL
# ifndef HAVE_LOCKF
#  ifdef HAVE_FLOCK
#   ifdef HAVE_SYS_FILE_H
#    include <sys/file.h>
#   else
#    undef HAVE_FLOCK
#   endif
#  endif
# endif
#endif

#undef LOCKABLE
#ifdef HAVE_FCNTL
# define LOCKABLE
#elif HAVE_LOCKF
# define LOCKABLE
#elif HAVE_FLOCK
# define LOCKABLE
#endif

#include "std.h"
#include "main.h"
#include "percent.h"
#include "file.h"
#include "lock.h"

extern char *argvzero;
extern struct opt_s options;

/*
  lock -- create as good a lock as possible
          what a mess
*/

public int do_lock(struct file_s *f)
{
  char prompt[4];

#ifndef LOCKABLE
  return(0);
#endif

#ifdef HAVE_FCNTL
  struct flock file_lock;

  file_lock.l_type = F_WRLCK;
  file_lock.l_whence = 0;
  file_lock.l_start = 0;
  file_lock.l_len = 0;
#endif

#ifdef LOCKTEST
  fprintf(stderr, "\rattempting lock on %s...", f->name);
#endif

#ifdef HAVE_FCHMOD
  /* try for a mandatory lock */
  if (fchmod(f->fd, 02600))
    {
# ifdef LOCKTEST
      fprintf(stderr, "mandatory lock failed\ntrying for advisory...");
# endif
    }
#endif

#ifdef HAVE_FCNTL
  if (fcntl(f->fd, F_SETLKW, &file_lock) == 0)
    {
# ifdef LOCKTEST
      fprintf(stderr, "got it\n");
# endif
      return 0; /* lock successful */
    }

#elif HAVE_LOCKF

  if (lseek(f->fd, 0, SEEK_SET))
    {
      fprintf(stderr, "\r%s: lseek() failed for `%s': %s\n",
	      argvzero, name, strerror(errno));
      exit_code = errno; return FAILED;
    }

  if (lockf(f->fd, F_LOCK, f->fsize) == 0)
    {
# ifdef LOCKTEST
      fprintf(stderr, "got it\n");
# endif
      return 0; /* lock successful */
    }

#elif HAVE_FLOCK
  if (flock(f->fd, LOCK_EX) == 0)
    {
# ifdef LOCKTEST
      fprintf(stderr, "got it\n");
# endif
      return 0; /* lock successful */
    }
#endif

  /* lock failed */
  if (options.force && !options.verbose)
    return 0;

  prompt[0] = 0; /* clear prompt */
  while (prompt[0] != 'y' && prompt[0] != 'n')
    {
      printf("\r%s: lock failed: \'%s\'. Wipe anyway? ", argvzero, f->name);
      fgets(prompt, sizeof(prompt), stdin);

      if (prompt[0] == 'y') return 0;
      if (prompt[0] == 'n') return FAILED;
    }

  return 0;
}
