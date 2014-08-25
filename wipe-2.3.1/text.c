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

#include <stdio.h>
#include "version.h"

#include "std.h"
#include "main.h"
#include "text.h"

extern char *argvzero;

const char version[] = VERSION;
const char email[] = EMAIL;
const char copyright[] = COPYRIGHT;
const char reldate[] = RELEASE_DATE;

/*
  help -- prints extented help
*/

public void help(FILE *out)
{

  fprintf(out, ""
  "Wipe v%s - released %s\n"
  "by Tom Vier <%s>\n\n"

  "Usage is %s [options] [file-list]\n\n"

  "Options:         Default: %s -ZdNTVEAkO -S512 -C%d -l1 -x1 -p1\n\n"

  "-h          --   help - display this screen\n"
  "-u          --   usage\n"
  "-c          --   show copyright and license\n"
  "-w          --   show warranty information\n"
  "-i  and  -I --   enable (-i) or disable (-I) interaction - overrides force\n"
  "-f          --   force file wiping and override interaction\n"
  "-r  and  -R --   recursion - traverse subdirectories\n"
  "-s          --   silent - disable percentage and error reporting\n"
  "-v          --   force verbose - always show percentage\n"
  "-V          --   verbose - show percentage if file is >= %dK\n"
  "-e  and  -E --   enhance (-e) percentage accuracy or faster writes (-E)\n"
  "-d  and  -D --   delete (-d) or keep (-D) after wiping\n"
  "-n  and  -N --   delete (-n) or skip (-N) special files\n"
  "-k  and  -K --   lock (-k) or don't lock (-K) files\n"
  "-z          --   zero-out file - single pass of zeroes\n"
  "-Z          --   perform normal wipe passes\n"
  "-t  and  -T --   enable (-t) or disable (-T) static passes\n"
  "-a  and  -A --   write until out of space (-a) or don't (-A)\n"
  "-o[size] -O --   write to stdout (-o) or use files (-O)\n"
  "-B(count)   --   block device sector count\n"
  "-S(size)    --   block device sector size - default 512 bytes\n"
  "                 or stdout write length when used with -A\n"
  "-C(size)    --   chunk size - maximum file buffer size in kilobytes (2^10)\n"
  "-l[0-2]     --   sets wipe secure level\n"
  "-x[1-32] -X --   sets number of random passes per wipe or disables\n"
  "-p(1-32)    --   wipe file x number of times\n"
  "-b(0-255)   --   overwrite file with this value byte\n"
  "", version, reldate, email, argvzero, argvzero,
      (CHUNK_SIZE >> 10), (PERCENT_ENABLE_SIZE >> 10));
}

/*
  badopt -- prints bad option error
*/

public void badopt(const int c)
{
  fprintf(stderr, ""
	  "error: bad option: %c\n"
	  "Type \'%s -h\' for help.\n"
	  "", c, argvzero);
}

/*
  usage -- prints usage info
*/

public void usage(FILE *out)
{
  fprintf(out, ""
  "Wipe v%s - released %s\n"
  "by Tom Vier <%s>\n\n"
  "", version, reldate, email);

  fprintf(out, ""
  "default: "
  "%s -ZdNTVEAkO -S512 -C%d -l1 -x1 -p1\n\n"

  "usage: "
  "%s [-ucwsiIhfFdDnNvVzZrRtTkKaAeE] "
  "[-B(count)] [-S(size)] [-C(size)] [-l[0-2]] [-x[1-32] -X] "
  "[-p(1-32)] [-b(0-255)] [-o[size] -O] [files]\n"
  "", argvzero, (CHUNK_SIZE >> 10), argvzero);
}

/*
  show_copyright -- shows copyright info from version.h
*/

public void show_copyright(void)
{
  printf(""
  "Wipe v%s - released %s\nby Tom Vier <%s>\n\n"
  "%s\n\n"

  "Wipe homepage: %s\n"
  "Freshmeat Appindex: %s\n\n"

  "Licensed under the GNU Public License.\n"
  "Wipe comes with ABSOLUTELY NO WARRANTY; for details type \"%s -w\".\n"
  "", version, reldate, email, copyright, URL, FRESHMEAT, argvzero);
}

/*
  show_war -- shows lack of warranty
*/

public void show_war(void)
{
  printf(""
  "Wipe v%s - released %s\n\n"
  "%s\n\n"

  "This program is free software; you can redistribute it and/or modify\n"
  "it under the terms of the GNU General Public License as published by\n"
  "the Free Software Foundation; either version 2 of the License, or\n"
  "(at your option) any later version.\n\n"

  "This program is distributed in the hope that it will be useful,\n"
  "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
  "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
  "GNU General Public License for more details.\n\n"

  "You should have received a copy of the GNU General Public License\n"
  "along with this program; if not, write to the Free Software\n"
  "Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.\n"
  "", version, reldate, copyright);
}
