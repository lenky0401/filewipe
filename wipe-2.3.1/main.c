/*
  Licensed under the GNU Public License.
  Copyright (C) 1998-2009 by Thomas M. Vier, Jr. All Rights Reserved.

  wipe v2.3
  by Tom Vier <nester@users.sf.net>

  http://wipe.sf.net/

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

#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>

#ifdef HAVE_GETOPT_H
# include <getopt.h>
#endif

#include "config.h"
#include "require.h"

#include "std.h"
#include "percent.h"
#include "file.h"
#include "rand.h"
#include "text.h"
#include "wipe.h"
#include "main.h"

/* global vars */
char *argvzero;
int exit_code;

extern int errno;

struct opt_s options;                 /* cmd line options                        */
struct rename_s rns;
struct sigaction sigact;


/* catch ctr-c */
public void int_hand(int sig)
{
  printf("\n%s: caught signal %d, exiting\n", argvzero, sig);

  restore_file();

  exit(sig);
}

public void restore_file(void)
{
  /* hack to restore current filename before exitting */
  if (rns.valid)
    if (rename(rns.cur_name, rns.orig_name))
      {
	fprintf(stderr, "%s: cannot rename `%s': %s\n",
		argvzero, rns.orig_name, strerror(errno));
      }

  if (rns.valid_mode)
    {
      /* restore file mode */
      if (chmod(rns.cur_name, (rns.mode & 07777)))
	{
	  fprintf(stderr, "\r%s: cannot restore file mode for `%s': %s\n",
		  argvzero, rns.orig_name, strerror(errno));
	}
    }
}

public int main(int argc, char **argv)
{
  int opt;                            /* option character                        */
  long long tmp; int tmpd;            /* to check input range                    */
  extern int optopt;                  /* getopt() stuff                          */
  extern char *optarg;                /* getopt() stuff                          */
  extern int optind, opterr;          /* getopt() stuff                          */

  opterr=0;                           /* we'll handle bad options                */

  errno=0;
  exit_code=0;
  argvzero = argv[0];

  /* set defaults */
  options.sectors               = 0;
  options.sector_size = SECTOR_SIZE;
  options.chunk_size   = CHUNK_SIZE;
  options.stdout_size           = 0;
  options.custom_byte        = 0x00;
  options.verbose               = 1;  /* show percent if >= PERCENT_ENABLE_SIZE  */
  options.percent_sync          = 0;  /* call fsync before printing percentage   */
  options.recursion             = 0;  /* do not traverse directories             */
  options.no_file               = 0;  /* use named files                         */
  options.until_full            = 0;  /* don't write until out of space          */
  options.zero                  = 0;  /* don't just zero-out the file            */
  options.force                 = 0;  /* respect file permissions                */
  options.delete                = 1;  /* remove targets                          */
  options.rmspcl                = 0;  /* don't unlink all special files          */
  options.custom                = 0;  /* don't use a custom byte                 */
  options.random                = 1;  /* perform random passes                   */
  options.statics               = 0;  /* don't preform static passes             */
  options.seclevel              = 1;  /* fast/secure mode                        */
  options.interactive           = 0;  /* don't confirm each file                 */
  options.random_loop           = 1;  /* one random pass per main loop           */
  options.wipe_multiply         = 1;  /* perform wipe loop once                  */

#ifdef SANITY
  /* sanity checks */
  if (sizeof(size_t) != sizeof(off_t))
    {
      printf("sizeof(size_t) != sizeof(off_t): file offsets are screwed!\n");
      abort();
    }
#endif

  /* set signal handler */
  rns.valid=0; rns.valid_mode=0;
  sigact.sa_handler = int_hand;
  sigemptyset(&sigact.sa_mask);
  sigact.sa_flags = 0;
  sigaction(SIGINT, &sigact, 0);

  while ((opt = getopt(argc, argv, "S:C:B:p:b:l::x::XucwsiIhHfFnNdDvVzZrRtTkKaAeEo::O")) != -1)
    {
      switch (opt)
	{
	case 'B': /* sector count */
	  sscanf(optarg, "%lld", &tmp);

	  if (tmp < 1)
	    {
	      fprintf(stderr, "%s: bad option: sector count < 1\n",
		      argvzero);
	      exit(BAD_USAGE);
	    }

	  options.sectors = tmp;
	  break;

	case 'S': /* sector size */
	  sscanf(optarg, "%lld", &tmp);

	  if (tmp < 1)
	    {
	      fprintf(stderr, "%s: bad option: sector size < 1\n",
		      argvzero);
	      exit(BAD_USAGE);
	    }

	  options.sector_size = tmp;
	  break;

	case 'C': /* chunk size -- max buf size */
	  sscanf(optarg, "%lld", &tmp);

	  if (tmp < 1)
	    {
	      fprintf(stderr, "%s: bad option: block device buffer size < 1k\n",
		      argvzero);
	      exit(BAD_USAGE);
	    }

	  options.chunk_size = tmp << 10;
	  break;

	case 'p': /* wipe multiply */
	  sscanf(optarg, "%lld", &tmp);
	
	  if (tmp < 1)
	    {
	      fprintf(stderr, "%s: bad option: wipe multiply < 1\n",
		      argvzero);
	      exit(BAD_USAGE);
	    }

	  if (tmp > 32)
	    {
	      fprintf(stderr, "%s: bad option: wipe multiply > 32\n",
		      argvzero);
	      exit(BAD_USAGE);
	    }

	  options.wipe_multiply = tmp;
	  break;

	case 'b':  /* overwrite file with byte */
	  sscanf(optarg, "%i", &tmpd);

	  if (tmpd < 0)
	    {
	      fprintf(stderr, "%s: bad option: wipe byte < 0\n", argvzero);
	      exit(BAD_USAGE);
	    }

	  if (tmpd > 255)
	    {
	      fprintf(stderr, "%s: bad option: wipe byte > 255\n", argvzero);
	      exit(BAD_USAGE);
	    }

	  options.custom = 1; options.custom_byte = tmpd;
	  break;

	case 'l':  /* set wipe secure level */
	  if (!optarg)
	    options.seclevel = 1;
	  else
	    {
	      sscanf(optarg, "%lld", &tmp);

	      if (tmp < 0 || tmp > 2)
		{
		  fprintf(stderr, "%s: bad option: secure level < 0 or > 2\n",
			  argvzero);
		  exit(BAD_USAGE);
		}

	      options.seclevel = tmp;
	    }
	  break;

	case 'x':  /* perform random passes */
	  if (!optarg)
	    options.random = 1;
	  else
	    {
	      tmp = atoi(optarg);

	      if (tmp < 0)
		{
		  fprintf(stderr, "%s: bad option: random loop < 0\n",
			  argvzero);
		  exit(BAD_USAGE);
		}

	      if (tmp > 32)
		{
		  fprintf(stderr, "%s: bad option: random loop > 32\n",
			  argvzero);
		  exit(BAD_USAGE);
		}

	      if (!tmp)
		options.random = 0;
	      else
		options.random_loop = tmp;
	    }
	  break;

	case 'X':  /* don't perform random passes */
	  /* random and static passes can't both be disabled */
	  options.random = 0;
	  options.statics = 1;
	  break;

	case 'r':  /* recursion */
	case 'R':  /* some people are used to '-R' */
	  options.recursion = 1; /* enable recursion */
	  break;

	case 'i':  /* interactive -- disables force */
	  options.force = 0;
	  options.interactive = 1;
	  break;

	case 'I':  /* non-interactive */
	  options.interactive = 0;
	  break;

	case 'f':  /* force -- ignore permissions and override interaction */
	  options.force = 1;
	  options.interactive = 0;
	  break;

	case 'F':  /* disable force */
	  options.force = 0;
	  break;

	case 'n':  /* remove special files, except blkdevs */
	  options.rmspcl = 1;
	  break;

	case 'N':  /* skip special files */
	  options.rmspcl = 0;
	  break;

	case 'd':  /* delete targets */
	  options.delete = 1;
	  break;

	case 'D':  /* don't remove targets */
	  options.delete = 0;
	  break;

	case 'c':  /* copyright */
	  show_copyright(); exit(0);
	  break;

	case 'w':  /* warranty */
	  show_war(); exit(0);
	  break;

	case 'u':  /* usage */
	  usage(stdout); exit(0);
	  break;

	case 'h':  /* help */
	case 'H':  /* undocumented */
	  help(stdout); exit(0);
	  break;

	case 'v':  /* force verbose */
	  if (!options.until_full)
	    options.verbose = 2;
	  break;

	case 'V':  /* verbose */
	  if (!options.until_full)
	    options.verbose = 1;
	  break;

	case 's':  /* silent */
	  options.verbose = 0;
	  options.interactive = 0;
	  break;

	case 'z':  /* zero */
	  options.zero = 1;
	  break;

	case 'Z':  /* don't just zero */
	  options.zero = 0;
	  break;

	case 't':  /* enable static passes */
	  options.statics = 1;
	  break;

	case 'T':  /* disable static passes */
	  options.statics = 0;
	  /* random and static passes can't both be disabled */
	  if (!options.random)
	    options.random = 1;
	  break;

	case 'k': /* enable file locks */
	  options.lock = 1;
	  break;

	case 'K': /* disable file locks */
	  options.lock = 0;
	  break;

	case 'a': /* enable write until full */
	  options.until_full = 1;
	  options.verbose = 0; /* screws up percentage reporting */
	  break;

	case 'A': /* disable write until full */
	  options.until_full = 0;
	  break;

	case 'e': /* fsync before printing percentage */
	  options.percent_sync = 1;
	  break;

	case 'E': /* don't - faster, but more accurate */
	  options.percent_sync = 0;
	  break;

	case 'o': /* write to stdout */
	  if (optarg)
	    {
	      sscanf(optarg, "%lld", &tmp);

	      if (tmp < 1)
		{
		  fprintf(stderr, "%s: bad option: stdout size < 1\n",
			  argvzero);
		  exit(BAD_USAGE);
		}

	      options.stdout_size = tmp;
	    }
	  else
	    options.until_full = 1;

	  options.no_file = 1;
	  options.verbose = 0;
	  break;

	case 'O': /* use named files */
	  options.no_file = 0;
	  options.stdout_size = 0;
	  break;

	default:
	  badopt(optopt);
	  break;
	}
    }

#ifdef OPTIONTEST
  printf("options are:\n");
  printf("sectors           = %lld\n", options.sectors);
  printf("sector_size       = %ld\n", options.sector_size);
  printf("chunk_size        = %ld\n", options.chunk_size);
  printf("stdout_size       = %ld\n", options.stdout_size);
  printf("verbose           = %d\n", options.verbose);
  printf("percent_sync      = %d\n", options.percent_sync);
  printf("recursion         = %d\n", options.recursion);
  printf("until_full        = %d\n", options.until_full);
  printf("stdout            = %d\n", options.no_file);
  printf("zero              = %d\n", options.zero);
  printf("force             = %d\n", options.force);
  printf("delete            = %d\n", options.delete);
  printf("rmspcl            = %d\n", options.rmspcl);
  printf("custom            = %d\n", options.custom);
  printf("custom_byte       = 0x%x\n", options.custom_byte);
  printf("random            = %d\n", options.random);
  printf("statics           = %d\n", options.statics);
  printf("seclevel          = %d\n", options.seclevel);
  printf("interactive       = %d\n", options.interactive);
  printf("random_loop       = %d\n", options.random_loop);
  printf("wipe_multiply     = %d\n\n", options.wipe_multiply);
  abort();
#endif

#ifdef FILETEST
  fprintf(stderr, "getopt() parsed %d args\n", optind - 1);
#endif

  if (optind == argc && !options.no_file)
    {
      show_copyright();
      fprintf(stderr, "\nType \'%s -u\' for usage.\n",
	      argvzero);
      exit(BAD_USAGE);
    }

  /* check for bad combinations */
  if (options.no_file)
    {
      if (options.statics && !options.stdout_size &&
	  !(options.zero || options.custom_byte))
	{
	  fprintf(stderr, "%s: bad option: "
		  "you must give a stdout length for static passes\n",
		  argvzero);
	  exit(BAD_USAGE);
	}
    }

  if (rand_init())
    {
      fprintf(stderr, "\r%s: rand_init(): fatal error\n", argvzero);
      exit(exit_code);
    }

  if (!options.no_file)
    /* check access and wipe if ok */
    while (optind < argc) do_file(argv[optind++]); /*** parse files ***/
  else
    do_file("stdout"); /*** write to stdout ***/

#ifdef FILETEST
  fprintf(stderr, "\n");
#endif

  percent_shutdown();

  return exit_code;
}
