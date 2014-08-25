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

/* minimum requirements for wipe */
#ifndef HAVE_GETOPT
# error missing function: getopt() required
#endif

#ifndef HAVE_MEMSET
# error missing function: memset() required
#endif

#ifndef HAVE_UNLINK
# ifndef HAVE_REMOVE
#  error missing function: unlink() or remove() required
# endif
#endif

#ifndef HAVE_RMDIR
# ifndef HAVE_REMOVE
#  error missing function: rmdir() or remove() required
# endif
#endif

#ifndef HAVE_UNLINK
# ifndef HAVE_REMOVE
#  error missing function: unlink() or remove() required
# endif
#endif

#ifndef HAVE_RENAME
# error missing functions: rename() required
#endif

#ifndef HAVE_FSYNC
# ifndef O_SYNC
#  error fsync() or O_SYNC required
# endif
#endif
