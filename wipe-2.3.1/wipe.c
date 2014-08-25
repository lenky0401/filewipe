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
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>

#define __USE_GNU
#include <string.h>
#include <strings.h>

#include "config.h"

#ifdef STAT_MACROS_BROKEN
/* just in case, so we don't unlink a directory,
   we don't currently handle broken stat macros */
# define unlink(x) remove(x)
#endif

#ifndef HAVE_UNLINK
# define unlink(x) remove(x)
#endif

#include "std.h"
#include "str.h"
#include "io.h"
#include "main.h"
#include "percent.h"
#include "file.h"
#include "lock.h"
#include "dir.h"
#include "rand.h"
#include "wipe.h"

extern int errno;
extern int exit_code;
extern char *argvzero;
extern struct rename_s rns;
extern struct opt_s options;

private void pass_pause(void);
private int random_pass(struct file_s *f, int passes);
private int write3_pass(struct file_s *f, const unsigned char byte1, const unsigned char byte2, const unsigned char byte3);

private int zero(struct file_s *f);
private int wipe(struct file_s *f);

/*
  pass_pause -- pause for pass inspection
*/

private void pass_pause(void)
{
#ifdef PAUSE
  printf("Hit a key:");
  fgetc(stdin);
#endif
}

/*
  random_pass -- fills file with random bytes
*/

private int random_pass(struct file_s *f, int passes)
{
  int i;

#ifdef PAUSE
  fprintf(stderr, "\rentering random_pass()\n");
#endif

  while (passes--)
    {
      if (options.seclevel == 2)
	prng_seed();

      if (!options.until_full)
	{
	  i=0;
	  while (i++ < f->loop)
	    {
	      prng_fillbuf(options.seclevel, f->buf, f->bufsize);
	      if (do_write(f->name, f->fd, f->buf, f->bufsize)) return FAILED;

	      if (f->percent.display)
		{
		  if (options.percent_sync)
		    {
		      f->sfail = sync_data(f->name, f->fd, f->sfail);
		      if (f->sfail && !S_ISCHR(f->st.st_mode)) return FAILED;
		    }

		  percent_update(&f->percent);
		}
	    }

	  if (f->loopr)
	    {
	      prng_fillbuf(options.seclevel, f->buf, f->loopr);
	      if (do_write(f->name, f->fd, f->buf, f->loopr)) return FAILED;
	      /* no percent update */
	    }
	}
      else
	{
	  while (1)
	    {
	      prng_fillbuf(options.seclevel, f->buf, f->bufsize);
	      i = do_write(f->name, f->fd, f->buf, f->bufsize);

	      if (i == ENOSPC) break;
	      else if (i != 0) return FAILED;
	    }
	}

      f->sfail = do_fwb(f->name, f->fd, f->sfail);
      if (f->sfail && !S_ISCHR(f->st.st_mode)) return FAILED;

      pass_pause();
      if (do_rewindfd(f->name, f->real_name, &f->fd, f->st.st_mode)) return FAILED; /* rewind */
    }

#ifdef PAUSE
  fprintf(stderr, "\rleaving random_pass()\n");
#endif

  return 0;
}

/*
  write_pass -- fill file with given byte
*/

public int write_pass(struct file_s *f, const unsigned char byte)
{
  int i;

#ifdef PAUSE
  fprintf(stderr, "\rentering write_pass()\n");
#endif

#ifdef HAVE_BZERO
  if (byte == 0)
    bzero(f->buf, f->bufsize);
  else
#endif
    memset(f->buf, byte, f->bufsize);

  if (!options.until_full)
    {
      i=0;
      while (i++ < f->loop)
	{
	  if (do_write(f->name, f->fd, f->buf, f->bufsize)) return FAILED;

	  if (f->percent.display)
	    {
	      if (options.percent_sync)
		{
		  f->sfail = sync_data(f->name, f->fd, f->sfail);
		  if (f->sfail && !S_ISCHR(f->st.st_mode)) return FAILED;
		}

	      percent_update(&f->percent);
	    }
	}

      if (f->loopr)
	{
	  if (do_write(f->name, f->fd, f->buf, f->loopr)) return FAILED;
	  /* no percent update */
	}
    }
  else
    {
      while (1)
	{
	  i = do_write(f->name, f->fd, f->buf, f->bufsize);

	  if (i == ENOSPC) break;
	  else if (i != 0) return FAILED;
	}
    }

  f->sfail = do_fwb(f->name, f->fd, f->sfail);
  if (f->sfail && !S_ISCHR(f->st.st_mode)) return FAILED;

  pass_pause();
  if (do_rewindfd(f->name, f->real_name, &f->fd, f->st.st_mode)) return FAILED; /* rewind */

#ifdef PAUSE
  fprintf(stderr, "\rleaving write_pass()\n");
#endif

  return 0;
}

/*
  write3_pass -- over writes file with given bytes
*/

private int write3_pass(struct file_s *f, 
			const unsigned char byte1, 
			const unsigned char byte2, 
			const unsigned char byte3)
{
  int i, r;
  size_t size;
  off_t offset;
  unsigned char bytes[3], *cbuf;

#ifdef PAUSE
  fprintf(stderr, "\rentering write3_pass()\n");
#endif

  /* size must be a multiple of 3 */
  size = f->bufsize - (f->bufsize % 3);
  cbuf = (unsigned char *) f->buf;

  i=0;
  while (i < size)
    {
      cbuf[i++] = byte1;
      cbuf[i++] = byte2;
      cbuf[i++] = byte3;
    }

  r = f->bufsize - size; /* find remainder */

  if (r)
    {
      offset = size; i=0;

      bytes[0] = byte1;
      bytes[1] = byte2;
      bytes[2] = byte3;

      while (r--)
	*(unsigned char *)((void *) f->buf + offset++) = bytes[i++];
    }

  /* 
     ok, we've filled the buffer with the pass image

     see fsetbuf() for why we don't have to worry about
     f->bufsize (or f->fsize) being an interger
     multiple of 3
  */

  if (!options.until_full)
    {
      i=0;
      while (i++ < f->loop)
	{
	  if (do_write(f->name, f->fd, f->buf, f->bufsize)) return FAILED;

	  if (f->percent.display)
	    {
	      if (options.percent_sync)
		{
		  f->sfail = sync_data(f->name, f->fd, f->sfail);
		  if (f->sfail && !S_ISCHR(f->st.st_mode)) return FAILED;
		}

	      percent_update(&f->percent);
	    }
	}

      /* smooth out any wrinkles */
      offset = f->bufsize - size;

      if (f->loopr)
	{
	  if (do_write(f->name, f->fd, (f->buf + offset), f->loopr)) return FAILED;
	  /* no percent update */
	}
    }
  else
    {
      while (1)
	{
	  i = do_write(f->name, f->fd, f->buf, f->bufsize);

	  if (i == ENOSPC) break;
	  else if (i != 0) return FAILED;
	}
    }

  f->sfail = do_fwb(f->name, f->fd, f->sfail);
  if (f->sfail && !S_ISCHR(f->st.st_mode)) return FAILED;

  pass_pause();
  if (do_rewindfd(f->name, f->real_name, &f->fd, f->st.st_mode)) return FAILED; /* rewind */

#ifdef PAUSE
  fprintf(stderr, "\rleaving write3_pass()\n");
#endif

  return 0;
}

/*
  destroy_file -- destroy the file
                  calls wipe() and zero()
*/

public int destroy_file(struct file_s *f)
{
  rns.valid=0; rns.valid_mode=0;

  if (options.no_file)
    {
      /**** wipe ****/
      if (wipe(f) == FAILED)
	return FAILED;

      return 0;
    }

#ifdef SANITY
  if (strncmp(f->name, f->real_name, PATH_MAX))
    {
      fprintf(stderr, "\r%s: destroy(): f->name != f->real_name\n",
	      argvzero);
      abort();
    }
#endif

#ifdef DEBUG
  fprintf(stderr, "filename: %s\n", f->name);
#endif

  /* open */
  if (do_open(f->name, f->name, &f->fd)) return FAILED;

  /** lock **/
  if (options.lock)
    if (do_lock(f))
      goto failure;

  /*
    if they don't want random passes, or the file won't
    be unlinked, we'll skip the rename
  */

  /** rename **/
  if (options.delete && options.random)
    {
      if ((S_ISCHR(f->st.st_mode) || S_ISBLK(f->st.st_mode)) && !options.rmspcl)
	{ /* skip devices, unless we're going to delete */ }
      else
	{
	  /* first, just (hopefully) overwrite the name */
	  if (wipe_name(f->fd, f->real_name, strnlen(f->real_name, sizeof(f->real_name))) == FAILED)
	    goto failure;

	  /* now use a rand name */
	  if (wipe_name(f->fd, f->real_name, 0) == FAILED)
	    goto failure;
	}
    }

  /**** wipe ****/
  if (wipe(f) == FAILED)
    goto failure;

  if (options.delete)
    {
      /* destroy file references, unless this is debug build */
# ifdef HAVE_FTRUNCATE
      do_ftruncate(f->name, f->fd, 0);
# endif

      /** rename again **/
      if (options.random) 
	{
	  if ((S_ISCHR(f->st.st_mode) || S_ISBLK(f->st.st_mode)) && !options.rmspcl)
	    { /* skip devices, unless we're going to delete */ }
	  else
	    {
	      if (wipe_name(f->fd, f->real_name, 0) == FAILED)
		goto failure;
	    }
	}

#ifndef DEBUG
      if (unlink(f->real_name))
	{
	  fprintf(stderr, "\r%s: cannot unlink `%s': %s\n", 
		  argvzero, f->name, strerror(errno));
	  exit_code = errno; goto failure;
	}
#endif

      rns.valid=0; rns.valid_mode=0;
    }

  if (!options.delete)
    {
      /* restore file mode */
      if (fchmod(f->fd, (f->st.st_mode & 07777)))
	{
	  fprintf(stderr, "\r%s: cannot restore file mode for `%s': %s\n",
		  argvzero, f->name, strerror(errno));
	  exit_code = errno; goto failure;
	}
    }

  if (sync_data(f->name, f->fd, f->sfail) && !S_ISCHR(f->st.st_mode)) goto failure;

  do_close(f->name, f->fd);
  return 0;

 failure:
  restore_file();
  rns.valid=0; rns.valid_mode=0;
  sync_data(f->name, f->fd, f->sfail);
  do_close(f->name, f->fd);
  fprintf(stderr, "\r%s: failed to wipe `%s'\n", argvzero, f->name);
  return FAILED;
}

/*
  zero -- zeroes out a file. if options.custom, fills with custom byte
*/

private int zero(struct file_s *f)
{
  fsetbuf(f);

  if (fgetbuf(f))
    return FAILED;

  percent_init(&f->percent, f->name, f->bufsize, f->loop);

  if (write_pass(f, options.custom_byte)) return FAILED;

  percent_done(&f->percent);

  ffreebuf(f);

  return 0;
}

/*
  wipe -- runs wipe passes on a given file
*/

private int wipe(struct file_s *f)
{
  if (!options.no_file && (f->st.st_size == 0 && !options.until_full))
    {
      if (options.verbose)
	{
	  fprintf(stderr, "\r%s: zero length, skipping `%s'\n",
		  argvzero, f->name);

	}
      return 0; /* no need to write anything */
    }

  if (options.zero || options.custom) return zero(f);

  fsetbuf(f);

  if (fgetbuf(f))
    return FAILED;

  percent_init(&f->percent, f->name, f->bufsize, f->loop);

  /**** run the passes ****/
  if (wipe_passes(f)) {ffreebuf(f); return FAILED;}

  percent_done(&f->percent);

  ffreebuf(f);

  return 0;
}

/*
  wipe_passes -- runs the actual passes
*/

public int wipe_passes(struct file_s *f)
{
  int i, loop;

  if (STATIC_PASSES != 27)
    {
      /*
      STATIC_PASSES is defined in percent.h and used by percent_init()
      it should only need to be changed if this function is changed

      if this function is changed, STATIC_PASSES in percent_init()
      must be updated and the above constant must be changed to match
      */
      fprintf(stderr, "\rSTATIC_PASSES != 27\npercent code broken!\n");
      abort();
    }

  if (options.seclevel == 1)
    if (prng_seed())
      return FAILED;

  loop = options.wipe_multiply;

  while (loop--)
    {
      if (options.random)
	if (random_pass(f, options.random_loop / 2))
	  return FAILED;

      /*
	these patterns where taking from Peter Gutmann's 1996 USENIX article,
	Secure Deletion of Data from Magnetic and Solid-State Memory

	http://www.cs.auckland.ac.nz/~pgut001/secure_del.html
	http://wipe.sourceforge.net/secure_del.html

	thanks, peter!
      */

      if (options.statics)
	{
	  /*
	    comment format: 
	    pass number -- binary pattern -- target encoding scheme
	  */

	  /* fifth pass -- 01 -- RLL(1,7) and MFM */
	  if (write_pass(f, 0x55)) return FAILED;

	  /* sixth pass -- 10 -- same */
	  if (write_pass(f, 0xaa)) return FAILED;

	  /* seventh pass -- 10010010 01001001 00100100 -- RLL(2,7) and MFM */
	  if (write3_pass(f, 0x92, 0x49, 0x24)) return FAILED;

	  /* eighth pass -- 01001001 00100100 10010010 -- same */
	  if (write3_pass(f, 0x49, 0x24, 0x92)) return FAILED;

	  /* ninth pass -- 00100100 10010010 01001001 -- same */
	  if (write3_pass(f, 0x24, 0x92, 0x49)) return FAILED;

	  /* tenth pass -- start 0x11 increment passes */
	  for (i = 0x00; i <= 0xff; i += 0x11)
	    if (write_pass(f, i))
	      return FAILED;

	  /* 26 -- RLL(2,7) and MFM passes, again */
	  if (write3_pass(f, 0x92, 0x49, 0x24)) return FAILED;
	  if (write3_pass(f, 0x49, 0x24, 0x92)) return FAILED;
	  if (write3_pass(f, 0x24, 0x92, 0x49)) return FAILED;

	  /* 29 -- 01101101 10110110 11011011 -- RLL(2,7) */
	  if (write3_pass(f, 0x6d, 0xb6, 0xdb)) return FAILED;
	  /* 10110110 11011011 01101101 */
	  if (write3_pass(f, 0xb6, 0xdb, 0x6d)) return FAILED;
	  /* 11011011 01101101 10110110 */
	  if (write3_pass(f, 0xdb, 0x6d, 0xb6)) return FAILED;
	}

      /* odd rand passes are done last */
      if (options.random)
	if (random_pass(f, options.random_loop / 2 + options.random_loop % 2))
	  return FAILED;
    }

  return 0;
}
