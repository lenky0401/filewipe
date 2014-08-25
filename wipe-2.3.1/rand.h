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

#ifdef HAVE_STDINT_H
# include "stdint.h"
#else
# ifndef LINUX
#  define u_int32_t uint32_t
# endif
#endif

typedef u_int32_t u_rand_t;

#ifndef UINT32_MAX
# define UINT32_MAX ULONG_MAX
#endif

#define URAND_MAX UINT32_MAX

public int rand_init(void);
public int prng_seed(void);
public u_rand_t prng_get_rand(void);
public void prng_fillbuf(const int seclevel, u_rand_t *buf, size_t size);
