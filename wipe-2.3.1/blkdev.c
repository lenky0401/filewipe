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
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include "config.h"

#ifdef HAVE_LINUX_FS_H
# include <linux/fs.h>
# define LINUX_BLKDEV
#else
# ifdef LINUX
#  ifdef HAVE_SYS_MOUNT_H
#   include <sys/mount.h>
#   define LINUX_BLKDEV
#  endif
# endif
#endif

#ifdef HAVE_SYS_DISKLABEL_H
# include <sys/disklabel.h>
# define BSD_BLKDEV
#endif

#ifndef LINUX_BLKDEV
# ifndef BSD_BLKDEV
#  define NO_BLKDEV
# endif
#endif

#include "std.h"
#include "io.h"
#include "main.h"
#include "percent.h"
#include "file.h"
#include "wipe.h"
#include "blkdev.h"

extern int errno;
extern int exit_code;
extern char *argvzero;
extern struct opt_s options;

private int wipe_blkdev(struct file_s *f);
private int zero_blkdev(struct file_s *f);

/*
  wipe_blkdev -- runs the wipe passes on a block device
*/

private int wipe_blkdev(struct file_s *f)
{
  if (options.zero || options.custom) return zero_blkdev(f);

  fsetbuf(f);

  if (fgetbuf(f) == FAILED)
    return FAILED;

  percent_init(&f->percent, f->name, f->bufsize, f->loop);

  /**** run the passes ****/
  if (wipe_passes(f))
    {
      fprintf(stderr, "\r%s: failed to wipe `%s'\n", argvzero, f->name);
      ffreebuf(f);
      return FAILED;
    }

  percent_done(&f->percent);

  ffreebuf(f);

  return 0;
}

/*
  zero_blkdev -- zeroes out a block device
*/

private int zero_blkdev(struct file_s *f)
{
  fsetbuf(f);

  if (fgetbuf(f) == FAILED)
    {
      fprintf(stderr, "\r%s: failed to wipe `%s'\n", argvzero, f->name);
      return FAILED;
    }

  percent_init(&f->percent, f->name, f->bufsize, f->loop);

  if (write_pass(f, options.custom_byte))
    {
      fprintf(stderr, "\r%s: failed to wipe `%s'\n", argvzero, f->name);
      ffreebuf(f);
      return FAILED;
    }

  percent_done(&f->percent);

  ffreebuf(f);

  return 0;
}

/*
  destroy_blkdev -- destroy the device's data
                    calls subroutines wipeblkdev() and zero()
*/

public int destroy_blkdev(struct file_s *f)
{
  int code;

#ifdef BSD_BLKDEV
  struct partinfo pinfo;
#endif

#ifdef NO_BLKDEV
  fprintf(stderr, "\r%s: block device support not available\n", argvzero);
  return FAILED;
#endif

#ifdef SANITY
  if (!S_ISBLK(f->st.st_mode))
    {
      fprintf(stderr, "\r%s: destroy_blkdev(): not a block dev: %s\n",
	      argvzero, f->name);
      abort();
    }

  if (strncmp(f->name, f->real_name, PATH_MAX) != 0)
    {
      fprintf(stderr, "\r%s: destroyblkdev(): f->name != f->real_name\n",
	      argvzero);
      abort();
    }
#endif

  if ((f->fd = open(f->real_name, O_WRONLY | O_NOFOLLOW | SYNC)) < 0)
    {
      fprintf(stderr, "\r%s: cannot open `%s':%s\n",
	      argvzero, f->name, strerror(errno));
      exit_code = errno; return FAILED;
    }

#ifdef BSD_BLKDEV
  if (ioctl(f->fd, DIOCGPART, &pinfo))
    {
      fprintf(stderr, "\r%s: ioctl failed, can't get disklabel for `%s': %s\n",
	      argvzero, f->name, strerror(errno));
      exit_code = errno; return FAILED;
    }
#endif

  if (options.sectors == 0)
    {
#ifdef LINUX_BLKDEV
      int tmp;

      if (ioctl(f->fd, BLKGETSIZE, &tmp))
	{
	  fprintf(stderr, "\r%s: ioctl failed, can't get sector count for `%s': %s\n",
	      argvzero, f->name, strerror(errno));
	  exit_code = errno; return FAILED;
	}
      options.sectors = tmp;
#endif

#ifdef BSD_BLKDEV
      options.sectors = pinfo.part->p_size;
#endif
    }

  if (options.sector_size == 0)
    {
#ifdef BSD_BLKDEV
      options.sector_size = pinfo.disklab->d_secsize;
#else
      options.sector_size = SECTOR_SIZE;
#endif
    }

  f->fsize = (options.sectors) * options.sector_size;

  if (f->fsize == 0)
    {
      fprintf(stderr, "\r%s: `%s' is zero length\n",
	      argvzero, f->name);
      return FAILED;
    }

#ifdef DEBUG
  fprintf(stderr, "\rzeroblkdev: f->fsize == %ld\n", f->fsize);
#endif

  code = wipe_blkdev(f); /**** wipe block device ****/

  close(f->fd);
  return code;
}
