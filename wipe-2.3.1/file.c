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

#ifndef _POSIX_SYNCHRONIZED_IO
# define _POSIX_SYNCHRONIZED_IO
# include <unistd.h>
#endif

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "config.h"

#include "std.h"
#include "main.h"
#include "percent.h"
#include "file.h"
#include "rand.h"
#include "prompt.h"
#include "wipe.h"

extern int errno;
extern int exit_code;
extern char *argvzero;
extern struct rename_s rns;
extern struct opt_s options;

private int fgetdata(const char *name, struct file_s *f);

/*
  do_file -- file processing begins here
*/

public void do_file(const char *name)
{
  int code;
  struct file_s f;

  rns.valid=0; rns.valid_mode=0;

  if ((code = fgetdata(name, &f)) == FAILED)
    {exit_code = code; return;}

  if (options.no_file)
    {destroy_file(&f); return;}

  switch (f.st.st_mode & S_IFMT)
    {
    case S_IFREG: /* regular file */
    case S_IFBLK: /* block device */
    case S_IFCHR: /* char dev */
      prompt_destroy(&f, R_OK|W_OK);
      break;

    case S_IFDIR: /* directory */
      prompt_recursion(name, R_OK|W_OK|X_OK, f.st.st_mode);
      break;

    case S_IFIFO: /* fifo */
      prompt_unlink(name, "fifo", W_OK, f.st.st_mode);
      return;

    case S_IFSOCK: /* socket */
      prompt_unlink(name, "socket", W_OK, f.st.st_mode);
      return;

    case S_IFLNK: /* sym link */
      prompt_unlink(name, "symbolic link", W_OK, f.st.st_mode);
      return;

    default:
      abort();
    }
}

/*
  fsetbuf -- inits fsize, bufsize, loop, sfail
*/

public void fsetbuf(struct file_s *f)
{
  /* init */
  f->sfail = 0;
  f->bufsize = options.chunk_size;

  /* 
     the block size is a multiple of the sector size, so
     covering all allocated blocks covers all sectors
  */

  /* 
     cover all allocated blocks 

     since sector_size % 3 is almost always > 0, we
     always run ++f->fsize at least once, so that it'll
     effectively do an if (fsize % 3) ++three_write_count,
     if you catch my drift. that way, write3_pass() doesn't
     have to worry about f->bufsize % 3 == 0
  */

  if (!options.no_file)
    {
      if (!S_ISBLK(f->st.st_mode) && !S_ISCHR(f->st.st_mode))
	{
	  f->fsize = f->st.st_size;

	  if (f->fsize % f->st.st_blksize)
	    f->fsize += f->st.st_blksize - (f->fsize % f->st.st_blksize);
	  else
	    ++f->fsize;
	}
    }
  else
    f->fsize = options.stdout_size;

  /*
    note that i don't do 
    while (f->bufsize % sizeof(unsigned long)) ++f->bufsize;
    which would simplify prng_fillbuf()

    in order for both that and f->bufsize % 3 == 0 to be true,
    you'd have to loop repeatedly which could possibly make
    the file much larger than we'd want

    see previous comment block for why we don't have to worry
    about this. this block is left here for historical reasons.
  */

  if (f->fsize > options.chunk_size)
    {
      f->loop = f->fsize / options.chunk_size;
      f->loopr = f->fsize % options.chunk_size;
    }
  else
    {
      f->loop = 1; f->loopr = 0;
      if (f->fsize) f->bufsize = f->fsize;
    }
}

/*
  ffreebuf -- deallocate file buffer
*/

public void ffreebuf(struct file_s *f)
{
  free(f->buf);
}

/*
  fgetbuf -- allocate file buffer
*/

public int fgetbuf(struct file_s *f)
{
  if ((f->buf = malloc(f->bufsize)) == NULL)
    {
      fprintf(stderr, "\r%s: cannot allocate %ld bytes for `%s': %s\n", 
	      argvzero, (long int) f->bufsize, f->name, strerror(errno));
      exit_code = errno; return FAILED;
    }

#ifdef SANITY
  if ((unsigned long int) f->buf % sizeof(u_rand_t))
    {
      fprintf(stderr, "\r%s: buffer not %ld byte aligned! Skipping `%s'\n",
	      argvzero, (long int) sizeof(u_rand_t), f->name);
      return FAILED;
    }
#endif

  return 0;
}

/*
  fgetdata -- gets stat() info and fills in struct file_s
*/

private int fgetdata(const char *name, struct file_s *f)
{
  /*
   * save the original pathname, so we can use it
   * when interacting with the user
   */

  /* copy pathname */
  strncpy(f->name, name, PATH_MAX);
  strncpy(f->real_name, name, PATH_MAX);
  strncpy(rns.orig_name, name, PATH_MAX);
  strncpy(rns.cur_name, name, PATH_MAX);

  /* 
   * make sure the destination string is NUL terminated
   * strncpy() does NOT guarantee dest strings are NUL terminated
   */
  f->name[sizeof(f->name)-1] = 0;
  f->real_name[sizeof(f->real_name)-1] = 0;
  rns.orig_name[sizeof(rns.orig_name)-1] = 0;
  rns.cur_name[sizeof(rns.cur_name)-1] = 0;

  if (!options.no_file)
    {
      rns.valid = 0;
      /* get inode data */
      if (lstat(f->name, &f->st))
	{
	  fprintf(stderr, "\r%s: cannot stat `%s': %s\n",
		  argvzero, f->name, strerror(errno));
	  return FAILED;
	}
      rns.mode = f->st.st_mode;
      rns.valid_mode = 1;
    }

  /* init to safe values */
  if (options.no_file)
    f->fd = 1;
  else
    f->fd = -1;

  f->buf = NULL;
  f->bufsize=0; f->loop=0;

  return 0;
}
