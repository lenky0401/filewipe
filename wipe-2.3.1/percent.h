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

#define STATIC_PASSES 27
#define DEFAULT_SWIDTH 78

/* percent data */
struct percent_s
{
  char *name;                   /* pointer to f->name                        */
  size_t nlen;                  /* length of string pointed to by name       */
  int cur_pass;                 /* current pass                              */
  int total_passes;             /* per file                                  */
  unsigned int display:1;       /* show percentage                           */
  unsigned int reported:1;      /* set if percentage was reported            */
};

public void percent_init(struct percent_s *p, const char *name, const size_t bufsize, const long int loop);
public void percent_update(struct percent_s *p);
public void percent_done(struct percent_s *p);
public void percent_shutdown(void);
