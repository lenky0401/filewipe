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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "config.h"
#include "std.h"
#include "io.h"
#include "mt.h"
#include "main.h"
#include "rand.h"

extern char *argvzero;
extern int errno, exit_code;
extern struct opt_s options;

int entropyfd;
char entropy_name[13]; /* for do_read error reporting */

/*
  rand_init -- inits the entropy source file descriptor
*/

public int rand_init(void)
{
  /* try /dev/urandom first; if that fails, try /dev/random */
  if ((entropyfd = open("/dev/urandom", O_RDONLY)) < 0)
    {
      if ((entropyfd = open("/dev/random", O_RDONLY)) < 0)
	{
	  fprintf(stderr, "\r%s: cannot open entropy source: %s\n",
		  argvzero, strerror(errno));
	  exit(1);
	}
      else
	{
	  strncpy(entropy_name, "/dev/random", sizeof(entropy_name));
	  fprintf(stderr, "\r%s: warning: cannot open /dev/urandom, "
		  "using /dev/random instead\n", argvzero);
	}
    }
  else
    strncpy(entropy_name, "/dev/urandom", sizeof(entropy_name));

  /* we must seed once at least regardless of seclevel --tg */
  if (prng_seed())
    return FAILED;

  return 0;
}

/*** the following functions are PRNG dependent ***/

/*
  prng_seed -- init seed
*/

public int prng_seed(void)
{
  u_rand_t seed;

  if (do_read(entropy_name, entropyfd, &seed, sizeof(prng_seed)))
    return FAILED;

  seedMT(seed);
  return 0;
}

/*
  prng_get_rand -- return u_rand_t PRN
*/

public u_rand_t prng_get_rand(void)
{
  return randomMT();
}

/*
  prng_fillbuf -- fills a buffer with pseudo-random values
                  the buffer must be u_rand_t aligned
*/

public void prng_fillbuf(const int seclevel, u_rand_t *buf, const size_t size)
{
  int i, ii;
  u_rand_t rand;
  unsigned char *cbuf, *randp;

  i=0; ii = size / sizeof(u_rand_t);

  while (i < ii)
    buf[i++] = randomMT();

  ii = size % sizeof(u_rand_t);

  if (ii)
    {
      rand = randomMT();
      cbuf = (unsigned char *) ((void *) buf + (size - ii));
      randp = (unsigned char *) &rand;

      i=0;
      while (i < ii)
	{
	  cbuf[i] = randp[i];
	  i++;
	}
    }
}
