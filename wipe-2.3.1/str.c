/*
  Licensed under the GNU Public License.
  Copyright (C) 1998-2003 by Thomas M. Vier, Jr. All Rights Reserved.

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

#include <sys/types.h>
#include <limits.h>
#include <string.h>

#include "std.h"
#include "rand.h"
#include "str.h"

private char rand_safe_char(void);

/*
  strnlen -- glibc strnlen() emulator
*/

#ifndef HAVE_STRNLEN
public size_t strnlen(const char *str, const size_t maxlen)
{
  size_t len;

  len = strlen(str);
  if (len > maxlen) len = maxlen;

  return len;
}
#endif

/*
  rename_str -- fill a string with random chars
*/

public void rename_str(char str[], const size_t len)
{
  int i;

  i=0;
  while (i <= (len-2))
    str[i++] = rand_safe_char();

  str[len-1] = 0;
}

/*
  rand_safe_char -- return a safe low-ASCII char
*/

private char rand_safe_char(void)
{
  int i;

  /* Colin Plumb's string - taken from his program, sterilize */
  /* Possible file name characters - a very safe set. */
  const char nameset[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_+=%@#.";

  /*
    see the man page for rand(3) for more info on this algorithm.

    it was originally adapted from:

    Numerical Recipes in C: The Art of Scientific Computing
    (William  H.  Press, Brian P. Flannery, Saul A. Teukolsky,
    William T.  Vetterling;  New  York:  Cambridge  University
    Press,  1990 (1st ed, p. 207))
  */

  /* now pick a random char from nameset */
  i = (int) ((float) (sizeof(nameset) - 1.0) * prng_get_rand() / (URAND_MAX+1.0));

  return nameset[i];
}
