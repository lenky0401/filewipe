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

#include <string.h>
#include <limits.h>

#include "config.h"

#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif
/* elif, might be defined somewhere else, but every *nix should have mode_t */

/* command line options */
struct opt_s
{
  unsigned long long sectors;   /* block device sector count                 */
  size_t sector_size;           /* sector size                               */
  size_t chunk_size;            /* how big bufsize should be                 */
  size_t stdout_size;           /* stdout write length                       */
  unsigned char custom_byte;    /* custom overwrite byte                     */
  unsigned int verbose:2;       /* verbose level                             */
  unsigned int percent_sync:1;  /* call fsync before printing progress       */
  unsigned int recursion:1;     /* traverse directories                      */
  unsigned int until_full:1;    /* write until out of space                  */
  unsigned int no_file:1;       /* write to stdout                           */
  unsigned int zero:1;          /* zero-out file                             */
  unsigned int lock:1;          /* lock files                                */
  unsigned int force:1;         /* force wipes -- override interaction       */
  unsigned int delete:1;        /* remove targets                            */
  unsigned int rmspcl:1;        /* remove special files (except blkdevs)     */
  unsigned int custom:1;        /* wipe file with user specified byte        */
  unsigned int random:1;        /* perform random passes                     */
  unsigned int statics:1;       /* perform static passes                     */
  unsigned int seclevel:2;      /* secure level                              */
  unsigned int interactive:1;   /* prompt for each file                      */
  unsigned int random_loop:6;   /* how many times to loop the random passes  */
  unsigned int wipe_multiply:6; /* how many times to loop all 35 passes      */
};

struct rename_s
{
  unsigned int valid:1, valid_mode:1;
  char orig_name[PATH_MAX+1],
  cur_name[PATH_MAX+1];
  mode_t mode;
};

public void int_hand(int sig);
public void restore_file(void);
public int main(int argc, char **argv);
