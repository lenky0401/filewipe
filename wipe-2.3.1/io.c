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

#ifndef _POSIX_SYNCHRONIZED_IO
# define _POSIX_SYNCHRONIZED_IO
#endif

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "config.h"

#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif

#ifdef HAVE_SYS_IOCTL_H
# ifdef HAVE_LINUX_MTIO_H
#  include <sys/ioctl.h>
#  include <linux/mtio.h>
# endif
#endif

#include "std.h"
#include "percent.h"
#include "file.h"
#include "wipe.h"
#include "main.h"
#include "io.h"

extern int errno;
extern int exit_code;
extern char *argvzero;
extern struct opt_s options;

/*
  do_fwb -- file write barrier

  hopefully, linux and others will export a real
  write barrier to userspace some day
*/

public int do_fwb(const char name[], const int fd, const short failed)
{
  return sync_data(name, fd, failed);
}

/*
  sync_data -- sync file to disk
*/

public int sync_data(const char name[], const int fd, const short failed)
{
#ifdef SANITY
  if (fd == -1) abort(); /* catch any callers with invalid fd's */
#endif

#if !defined (HAVE_FSYNC) || !defined (HAVE_FDATASYNC)
  /*
    the file will still get written out
    file.h will make the file be opened with a sync flag
  */
  return 0;
#endif

#ifdef HAVE_FDATASYNC
  if (fdatasync(fd))
#endif
#ifdef HAVE_FSYNC
    if (fsync(fd))
#endif
      {
	if (!failed)
	  {
	    fprintf(stderr, "\r%s: cannot synchronize `%s': %s\n",
		    argvzero, name, strerror(errno));
#ifdef HAVE_FCNTL
	    if (fcntl(fd, F_SETFL, O_SYNC) == -1)
	      {
		fprintf(stderr, "\r%s: cannot set synchronis writes `%s': %s\n",
			argvzero, name, strerror(errno));
		exit_code = errno;
	      }
#endif
	  }
	return FAILED;
      }
  return 0;
}

/*
  do_ftruncate -- ftruncate(2) wrapper
*/

public int do_ftruncate(const char name[], const int fd, off_t length)
{
  if (ftruncate(fd, length))
    {
      fprintf(stderr, "\r%s: cannot truncate `%s': %s\n",
	      argvzero, name, strerror(errno));
      exit_code = errno; return FAILED;
    }
  return 0;
}

/*
  do_open -- open(2) wrapper
*/

public int do_open(const char name[], const char real_name[], int *fd)
{
  /*
    shred (GNU fileutils) was using fopen() in write-only
    mode which truncates the file before returning. this
    encouraged reallocation of the file's blocks as shred
    wrote to it.

    one major flaw with file wipers like this one and shred,
    is that nothing guarantees that the FS isn't reallocating
    different blocks to the file. log structured FSes, like
    LFS, will almost never use the same blocks. in that case,
    block wiping must be done by wiping free space and any
    old file versions or snapshots.
   */

  /* see file.h about the SYNC flag */
  if ((*fd = open(real_name, O_WRONLY | O_NOFOLLOW | SYNC)) < 0)
    {
      fprintf(stderr, "\r%s: cannot open `%s': %s\n",
	      argvzero, name, strerror(errno));
      exit_code = errno; return FAILED;
    }

  return 0;
}


/*
  do_close -- close(2) wrapper
*/

public int do_close(const char name[], const int fd)
{
  if (close(fd))
    {
      fprintf(stderr, "\r%s: close failed for `%s': %s\n",
	      argvzero, name, strerror(errno));
      exit_code = errno; return FAILED;
    }
  return 0;
}

/*
  do_read -- read(2) wrapper
*/

public int do_read(const char name[], const int fd, void *buf, size_t count)
{
  ssize_t c;
  int retries;

 retry:
  c = read(fd, buf, count);

  if (c == -1)
    {
      if (errno == EINTR)
	goto retry;

      if (errno == EIO)
	{
	  if (retries == 5)
	    goto abort;
	  else
	    {++retries; goto retry;}
	}

    abort:
      fprintf(stderr, "\r%s: cannot read `%s': %s\n",
	      argvzero, name, strerror(errno));
      exit_code = errno; return FAILED;
    }
  else
    {
      if (c < count)
	{
	  count -= c;
	  do_read(name, fd, (buf + c), count); /* recurse */
	}
    }

  return 0;
}

/*
  do_write -- write(2) wrapper
*/

public int do_write(const char name[], const int fd, void *buf, size_t count)
{
  int ret;
  ssize_t c, written;

  if (count == 0) abort(); /* i'd like to know about it */

  ret=0; written=0;

  while (written < count)
    {
      c = write(fd, buf, count - written);

      if (c == 0) abort(); /* shouldn't happen */

      if (c > 0) /* full or partial success */
	{
	  written += c; buf += c;
	}
      else /* failed, c < 0 */
	{
	  if (errno == ENOSPC && options.until_full)
	    {
	      --count; ret = ENOSPC;
	    }
	  else if (errno == EAGAIN || errno == EINTR)
	    continue;
	  else
	    {
	      fprintf(stderr, "\r%s: write failed to `%s': %s\n",
		      argvzero, name, strerror(errno));
	      exit_code = errno; return FAILED;
	    }
	}
    }
  return ret;
}

public int do_rewindfd(const char name[], const char real_name[], int *fd, const mode_t mode)
{
  /* can't seek on stdout */
  if (options.no_file) return 0;

  if (lseek(*fd, 0, SEEK_SET))
    {
      fprintf(stderr, "\r%s: lseek() failed for `%s': %s\n",
	      argvzero, real_name, strerror(errno));
      exit_code = errno; return FAILED;
    }

  if (S_ISCHR(mode))
    {
      /* probably a tape drive - try to rewind */

#ifdef MTIOCTOP
      struct mtop cmd;
      cmd.mt_op = MTREW; cmd.mt_count = 1;

      if (ioctl(*fd, MTIOCTOP, cmd))
	{
	  fprintf(stderr, "\r%s: warning: ioctl() failed for `%s': %s - will attempt reopen\n",
		  argvzero, real_name, strerror(errno));
	}
      else return 0;
#endif
      /* this won't work on non-rewind devices like /dev/nst0 */
      if (do_close(name, *fd)) return FAILED;
      return do_open(name, real_name, fd);
    }

  return 0;
}
