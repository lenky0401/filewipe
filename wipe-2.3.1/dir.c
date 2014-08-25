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
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>

#define __USE_GNU
#include <string.h>

#include "config.h"

#ifndef HAVE_RMDIR
# define rmdir(x) remove(x)
#endif

#include "std.h"
#include "str.h"
#include "main.h"
#include "percent.h"
#include "file.h"
#include "io.h"
#include "dir.h"

extern int errno;
extern int exit_code;
extern char *argvzero;
extern struct rename_s rns;
extern struct opt_s options;

private int dir_sync(int ffd, char name[]);

/*
  drill_down -- drill down through a directory

  char str[]  --  directory name
*/

public void drill_down(const char str[])
{
  DIR *dir;
  struct dirent *entry;
  char *base, real_base[PATH_MAX+1];
  char dirname[PATH_MAX+1];
  size_t len;
  char prompt[4];

  strncpy(dirname, str, PATH_MAX);
  dirname[sizeof(dirname)-1] = 0;

#ifdef DEBUG
  fprintf(stderr, "checking %s\n", dirname);
#endif

  if ((dir = opendir(dirname)) == NULL)
    {
      fprintf(stderr, "\r%s: cannot open directory `%s': %s\n",
	      argvzero, dirname, strerror(errno));
      exit_code = errno; return;
    }

  if (chdir(dirname))
    {
      fprintf(stderr, "\r%s: cannot enter directory `%s': %s\n",
	      argvzero, dirname, strerror(errno));
      exit_code = errno; goto cleanup;
    }

  while ((entry = readdir(dir)) != NULL)
    {
      if (strncmp(entry->d_name, ".", 2) && strncmp(entry->d_name, "..", 3) != 0)
	do_file(entry->d_name); /*** process file ***/
    }

  if (chdir(".."))
    {
      fprintf(stderr, "\r%s: cannot exit current directory `%s': %s\n"
	              "We're trapped!\n", argvzero, dirname, strerror(errno));
      abort();
    }

#ifdef DEBUG
  if (options.delete)
    fprintf(stderr, "\rwould have removed: %s\n", dirname);
#endif

#ifndef DEBUG
  if (options.delete)
    {
      /*
	make sure dirname doesn't end in a slash,
	otherwise base will end up pointing to \0
      */
#ifdef HAVE_STRNLEN
      len = strnlen(dirname, PATH_MAX);
#else
      len = strlen(dirname);
      if (len > PATH_MAX) len = PATH_MAX;
#endif
      base = (char *) (dirname + (len-1)); /* find the last char */

      /* loop it, in case of multiple slashes */
      /* note that base is decremented after each use */
      while (strncmp((char *) base--, "/", 1) == 0)
	dirname[--len] = (char) 0; /* truncate string and update length */

      /*
	 point base at just after the path to the directory,
         since we're in the same directory that the target is.
         in other words, this strips the path the directory
      */
      if ((base = strrchr(dirname, '/')) == NULL)
	base = dirname;
      else
	++base;

      if (options.interactive)
	{
	  prompt[0] = 0;
	  while (prompt[0] != 'y' && prompt[0] != 'n')
	    {
	      printf("\r%s: remove directory `%s'? ",
		     argvzero, dirname);
	      fgets(prompt, sizeof(prompt), stdin);

	      if (prompt[0] == 'y' || prompt[0] == 'Y') break;
	      if (prompt[0] == 'n' || prompt[0] == 'N') goto cleanup;
	    }
	}

      strncpy(real_base, base, PATH_MAX);
      real_base[sizeof(real_base)-1] = 0;

      wipe_name(-1, real_base, strnlen(real_base, sizeof(real_base)));
      wipe_name(-1, real_base, 0);

      if (rmdir(real_base))
	{
	  fprintf(stderr, "\r%s: %s: unable to remove directory: `%s'\n",
		  argvzero, strerror(errno), base);

	  rename(real_base, base); /* try to rename it back */
	  exit_code = errno;
	}
    }

 cleanup:
  if (closedir(dir))
    {
      fprintf(stderr, "\r%s: closedir failed for `%s': %s\n",
	      argvzero, dirname, strerror(errno));
      exit_code = errno; return;
    }
#endif
}

/*
  wipe_name -- rename to random characters

  real_name - assumed to be PATHMAX+1 long
  rnlen - length to overwrite, or 0 for don't care (NAME_MAX)

  this won't work for hashed dirs
*/

public int wipe_name(int ffd, char *real_name, size_t rnlen)
{
  int i;
  size_t len, pathlen;
  char *base, dest_name[PATH_MAX+1];

#ifdef SANITY
  if ((!options.random) || (!options.delete))
    {
      fprintf(stderr, "%s: options.random is %d, options.delete is %d\n",
	      argvzero, options.random, options.delete);
      abort();
    }
#endif

  if (rnlen == 0 || rnlen > NAME_MAX)
    rnlen = NAME_MAX;

  if (rnlen > 0) ++rnlen;

  /* copy the path to the file */

  /*
    strncpy does NOT guarantee NUL termination
    it's an often over looked hole for a buffer overflow exploit from
    untrusted input.
  */
  strncpy(dest_name, real_name, rnlen);
  dest_name[sizeof(dest_name)-1] = 0;

  /* point base at just after the path, ie, strip the path */
  base = strrchr(dest_name, '/');

  /* strrchr doesn't ask for the boundry */
  if (base == NULL // not found
      || base >= (dest_name + sizeof(dest_name))) // boundry check
    base = dest_name;
  else
    ++base;

  /* truncate the path to get the length */
  *base = 0x00;

  pathlen = strnlen(dest_name, sizeof(dest_name));

  /*
    overwrite the filename, same length
    won't work for hashed dirs

    the second time wipe_name() called, it'll be trying to
    overwrite the rand name used last time
  */

  //len = strnlen((strrchr(real_name, '/')+1));

  /*
    we try to use as long a filename as possible,
    but not longer than PATH_MAX.
  */

  len = pathlen + rnlen;
  if (len > PATH_MAX)
    rnlen -= len - PATH_MAX;

  /*
    get a random filename, but make sure that there
    isn't one with the same name prior renaming --tg
  */

  i=0;
  while (!i)
    {
      rename_str(base, rnlen);
      i = access(dest_name, F_OK);
    }

#ifdef NORENAME
  /* debugging aid */
  fprintf(stderr, "\rwould have renamed to %s\n", base);
  return 0;
#endif

  if (rename(real_name, dest_name) == 0)
    {
      /* update pathname */
      strncpy(real_name, dest_name, strnlen(dest_name, sizeof(dest_name)));
      real_name[PATH_MAX] = 0;
      strncpy(rns.cur_name, dest_name, strnlen(dest_name, sizeof(dest_name)));
      rns.cur_name[PATH_MAX] = 0;
      rns.valid = 1;
    }
  else
    {
      fprintf(stderr, "\r%s: cannot rename `%s': %s\n",
	      argvzero, real_name, strerror(errno));
      exit_code = errno; return FAILED;
    }

  /* now try to commit the rename to storage */
  if (dir_sync(ffd, ".")) return FAILED;

  if (ffd == -1)
    {
      /* if the object is itself a dir, sync it also */
      if (dir_sync(ffd, real_name)) return FAILED;
    }

  return 0;
}

private int dir_sync(int ffd, char name[])
{

#ifdef HAVE_DIRFD
  int dfd;
  DIR *dir;

  /* sync the dir */
  if ((dir = opendir(".")) == NULL)
    {
      fprintf(stderr, "\r%s: cannot open directory `%s': %s\n",
	      argvzero, name, strerror(errno));
      exit_code = errno; return FAILED;
    }

  if ((dfd = dirfd(dir)) < 0)
    {
      fprintf(stderr, "\r%s: dirfd() failed for `%s': %s\n",
	      argvzero, name, strerror(errno));
      exit_code = errno; return FAILED;
    }

  sync_data(".", dfd, 0); /* this should force a write of the dir entry */

  if (closedir(dir))
    {
      fprintf(stderr, "\r%s: closedir failed for `%s': %s\n",
	      argvzero, name, strerror(errno));
      exit_code = errno; return FAILED;
    }
#endif

  /* fsync() the file, in case that's how to force the dir entry on some OS */
  if (ffd > -1) sync_data("FSYNC FOR wipe_name()", ffd, 1);

  return 0;
}
