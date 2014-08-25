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

#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef STAT_MACROS_BROKEN
/* just in case, so we don't unlink a directory,
   we don't currently handle broken stat macros */
# define unlink(x) remove(x)
#endif

#ifndef HAVE_UNLINK
# define unlink(x) remove(x)
#endif

#include "std.h"
#include "percent.h"
#include "file.h"
#include "dir.h"
#include "main.h"
#include "wipe.h"
#include "blkdev.h"
#include "prompt.h"

extern char *argvzero;
extern struct opt_s options;

/*
  prompt_destroy -- prompt user before destroying target
*/

public void prompt_destroy(struct file_s *f, const int perm)
{
  int permdenied;
  char prompt[4];

#ifdef SANITY
  if (!S_ISBLK(f->st.st_mode) && !S_ISREG(f->st.st_mode))
    {
      fprintf(stderr, "\r%s: prompt_destroy: not a file or block dev, `%s' mode %o\n",
	     argvzero, f->name, f->st.st_mode);
      abort();
    }
#endif

  if (options.force) goto destroy;

  permdenied = access(f->name, perm);
  if (options.interactive) /* force overrides interaction */
    {
      prompt[0] = 0; /* clear prompt */

      if (!permdenied)
	{
	  while (1)
	    {
	      switch (f->st.st_mode & S_IFMT)
		{
		case S_IFBLK:
		  printf("\r%s: destroy block device `%s'? ", argvzero, f->name);
		  break;
		case S_IFCHR:
		  printf("\r%s: destory character device `%s'? ", argvzero, f->name);
		  break;
		default:
		  printf("\r%s: destroy file `%s'? ", argvzero, f->name);
		  break;
		}

	      fgets(prompt, sizeof(prompt), stdin);

	      if (prompt[0] == 'y' || prompt[0] == 'Y') goto destroy;
	      if (prompt[0] == 'n' || prompt[0] == 'N') return;
	    }
	}
      else /* perm denied */
	{
	  while (1)
	    {
	      switch (f->st.st_mode & S_IFMT)
		{
		case S_IFBLK:
		  printf("\r%s: destroy block device `%s', "
			 "overriding mode %04o? ",
			 argvzero, f->name, (unsigned int) f->st.st_mode & 07777);
		  break;
		default:
		  printf("\r%s: destroy file `%s', "
			 "overriding mode %04o? ",
			 argvzero, f->name, (unsigned int)  f->st.st_mode & 07777);
		  break;
		}

	      fgets(prompt, sizeof(prompt), stdin);

	      if (prompt[0] == 'y' || prompt[0] == 'Y') goto destroy;
	      if (prompt[0] == 'n' || prompt[0] == 'N') return;
	    }
	}
    }

 destroy:
  switch (f->st.st_mode & S_IFMT)
    {
    case S_IFBLK:
      options.delete = 0;
      destroy_blkdev(f); return;
      break;

    case S_IFCHR:
      options.delete = 0;
      destroy_file(f); return;
      break;

    case S_IFREG:
      destroy_file(f); return;
      break;

    default:
      abort();
    }
}

/*
  prompt_unlink -- prompt for removal of non-wiped files
*/

public void prompt_unlink(const char name[], const char type[], const int perm, const mode_t mode)
{
  int permdenied;
  char prompt[4];

  if (!options.delete) return;

  if (!options.rmspcl)
    {
      if (options.verbose)
	fprintf(stderr, "\r%s: `%s' is a %s -- skipping unlink\n", argvzero, name, type);
      return;
    }

  if (options.force) goto unlink;

  permdenied = access(name, perm);

  if (options.interactive)
    {
      prompt[0] = 0; /* clear prompt */

      if (!permdenied)
	{
	  while (1)
	    {
	      printf("\r%s: remove %s `%s'? ",
		     argvzero, type, name);

	      fgets(prompt, sizeof(prompt), stdin);

	      if (prompt[0] == 'y' || prompt[0] == 'Y') goto unlink;
	      if (prompt[0] == 'n' || prompt[0] == 'N') return; /* skip to next file */
	    }
	}
      else /* permdenied */
	{
	  while (1)
	    {
	      printf("\r%s: remove %s `%s', "
		     "overriding mode %04o? ",
		     argvzero, type, name, (unsigned int) mode & 07777);

	      fgets(prompt, sizeof(prompt), stdin);

	      if (prompt[0] == 'y' || prompt[0] == 'Y') goto unlink;
	      if (prompt[0] == 'n' || prompt[0] == 'N') return;
	    }
	}
    }

 unlink:
  if (unlink(name))
    {
      if (options.verbose)
	{
	  fprintf(stderr, "\r%s: cannot remove %s `%s': %s\n",
		  argvzero, type, name, strerror(errno));
	}
    }
}

/*
  prompt_recursion -- prompt for directory recursion
*/

public void prompt_recursion(const char name[], const int perm, const mode_t mode)
{
  int permdenied;
  char prompt[4];

  if (!options.recursion)
    {
      fprintf(stderr, "\r%s: `%s' is a directory -- skipping\n", argvzero, name);
      return;      
    }

#ifdef SANITY
  if (!S_ISDIR(mode))
    {
      fprintf(stderr, "\r%s: prompt_recursion: not a directory, `%s' mode %o\n",
	     argvzero, name, mode);
      abort();
    }
#endif

  if (options.force) goto recurse;

  permdenied = access(name, perm);

  if (options.interactive)
    {
      prompt[0] = 0; /* clear prompt */

      if (!permdenied)
	{
	  while (1)
	    {
	      printf("\r%s: destroy files in `%s'? ",
		     argvzero, name);

	      fgets(prompt, sizeof(prompt), stdin);

	      if (prompt[0] == 'y' || prompt[0] == 'Y') goto recurse;
	      if (prompt[0] == 'n' || prompt[0] == 'N') return; /* skip to next file */
	    }
	}
      else /* permdenied */
	{
	  while (1)
	    {
	      printf("\r%s: remove files in `%s', "
		     "overriding mode %04o? ",
		     argvzero, name, (unsigned int) mode & 07777);

	      fgets(prompt, sizeof(prompt), stdin);

	      if (prompt[0] == 'y' || prompt[0] == 'Y') goto recurse;
	      if (prompt[0] == 'n' || prompt[0] == 'N') return;
	    }
	}
    }

 recurse:
  drill_down(name);
}
