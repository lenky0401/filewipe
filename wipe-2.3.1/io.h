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

public int do_fwb(const char name[], const int fd, const short failed);
public int sync_data(const char name[], const int fd, const short failed);
public int do_open(const char name[], const char real_name[], int *fd);
public int do_close(const char name[], const int fd);
public int do_ftruncate(const char name[], const int fd, off_t length);
public int do_read(const char name[], const int fd, void *buf, size_t count);
public int do_write(const char name[], const int fd, void *buf, size_t count);
public int do_rewindfd(const char name[], const char real_name[], int *fd, const mode_t mode);
