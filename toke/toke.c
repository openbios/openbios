/*
 *                     OpenBIOS - free your system! 
 *                         ( FCode tokenizer )
 *                          
 *  toke.c - main tokenizer loop and parameter parsing.
 *  
 *  This program is part of a free implementation of the IEEE 1275-1994 
 *  Standard for Boot (Initialization Configuration) Firmware.
 *
 *  Copyright (C) 2001-2005 by Stefan Reinauer <stepan@openbios.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA, 02110-1301 USA
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef __GLIBC__
#define _GNU_SOURCE
#include <getopt.h>
#endif

#include "toke.h"
#include "stream.h"
#include "stack.h"
#include "emit.h"

#define TOKE_VERSION "0.6.10"

int verbose=0;
int noerrors=0;

static void print_copyright(void)
{
        printf( "Welcome to toke - OpenBIOS tokenizer v%s\nCopyright (c)"
		" 2001-2005 by Stefan Reinauer <stepan@openbios.org>\n"
		"This program is free software; you may redistribute it "
		"under the terms of\nthe GNU General Public License. This "
		"program has absolutely no warranty.\n\n", TOKE_VERSION);
}

static void usage(char *name)
{
	printf("usage: %s [-v] [-i] [-o target] <forth-file>\n\n",name);
	printf("  -v|--verbose          print debug messages\n");
	printf("  -i|--ignore-errors    don't bail out after an error\n");
	printf("  -h|--help             print this help message\n\n");

}

int main(int argc, char **argv)
{
	const char *optstring="vhio:?";
	char *outputname = NULL;
	int c;

	while (1) {
#ifdef __GLIBC__
		int option_index = 0;
		static struct option long_options[] = {
			{ "verbose", 0, 0, 'v' },
			{ "ignore-errors", 0, 0, 'i' },
			{ "help", 0, 0, 'h' },
			{ 0, 0, 0, 0 }
		};

		c = getopt_long (argc, argv, optstring,
				 long_options, &option_index);
#else
		c = getopt (argc, argv, optstring);
#endif
		if (c == -1)
			break;

		switch (c) {
		case 'v':
			verbose=1;
			break;
		case 'o':
			outputname = optarg;
			break;
		case 'i':
			noerrors=1;
			break;
		case 'h':
		case '?':
			print_copyright();
			usage(argv[0]);
			return 0;		
		default:
			print_copyright();
			printf ("%s: unknown options.\n",argv[0]);
			usage(argv[0]);
			return 1;
		}
	}

	if (verbose)
		print_copyright();

	if (optind >= argc) {
		print_copyright();
		printf ("%s: filename missing.\n",argv[0]);
		usage(argv[0]);
		return 1;
	}

	init_stack();
	init_dictionary();
	init_macros();
	init_scanner();
	
	while (optind < argc) {
		if (init_stream(argv[optind])) {
			printf ("%s: warning: could not open file \"%s\"\n",
					argv[0], argv[optind]);
			optind++;
			continue;
		}
		init_output(argv[optind], outputname);

		tokenize();
		finish_headers();
		
		close_output();
		close_stream();
		
		optind++;
	}
	
	exit_scanner();
	return 0;
}

