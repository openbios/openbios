/*
 *                     OpenBIOS - free your system! 
 *                            ( detokenizer )
 *                          
 *  detok.c parameter parsing and main detokenizer loop.  
 *  
 *  This program is part of a free implementation of the IEEE 1275-1994 
 *  Standard for Boot (Initialization Configuration) Firmware.
 *
 *  Copyright (C) 2001-2005 Stefan Reinauer, <stepan@openbios.org>
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
#else
/* Some systems seem to have an incomplete unistd.h.
 * We need to define getopt() and optind for them.
 */
extern int optind;
int getopt(int argc, char * const argv[], const char *optstring);
#endif

#include "detok.h"
#include "stream.h"

#define DETOK_VERSION "0.6.1"

/* prototypes for dictionary handling */
void init_dictionary(void);
void decode_token(u16 token);
/* prototype for detokenizer function */
int  detokenize(void);

extern unsigned int decode_all, verbose, linenumbers;

void print_copyright(void)
{
        printf( "Welcome to the OpenBIOS detokenizer v%s\ndetok Copyright"
		"(c) 2001-2005 by Stefan Reinauer.\nWritten by Stefan "
		"Reinauer, <stepan@openbios.org>\n" "This program is "
		"free software; you may redistribute it under the terms of\n"
		"the GNU General Public License.  This program has absolutely"
					" no warranty.\n\n" ,DETOK_VERSION);
}

void usage(char *name)
{
	printf( "usage: %s [OPTION]... [FCODE-FILE]...\n\n"
		"         -v, --verbose     print fcode numbers\n"
		"         -a, --all         don't stop at end0\n"
		"         -n, --linenumbers print line numbers\n"
		"         -o, --offsets     print byte offsets\n"
		"         -h, --help        print this help text\n\n", name);
}

int main(int argc, char **argv)
{
	int c;
	const char *optstring="vhano?";

	while (1) {
#ifdef __GLIBC__
		int option_index = 0;
		static struct option long_options[] = {
			{ "verbose", 0, 0, 'v' },
			{ "help", 0, 0, 'h' },
			{ "all", 0, 0, 'a' },
			{ "linenumbers", 0, 0, 'n' },
			{ "offsets", 0, 0, 'o' },
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
		case 'a':
			decode_all=1;
			break;
		case 'n':
			linenumbers|=1;
			break;
		case 'o':
			linenumbers|=2;
			break;
		case 'h':
		case '?':
			print_copyright();
			usage(argv[0]);
			return 0;		
		default:
			print_copyright();
			printf ("%s: unknown option.\n",argv[0]);
			usage(argv[0]);
			return 1;
		}
	}

	if (verbose)
		print_copyright();
	
	if (linenumbers>2)
		printf("Line numbers will be disabled in favour of offsets.\n");
	
	if (optind >= argc) {
		print_copyright();
		printf ("%s: filename missing.\n",argv[0]);
		usage(argv[0]);
		return 1;
	}
	
	init_dictionary();
	
	while (optind < argc) {
		
		if (init_stream(argv[optind])) {
			printf ("Could not open file \"%s\".\n",argv[optind]);
			optind++;
			continue;
		}
		detokenize();
		close_stream();
		
		optind++;
	}
	
	printf("\n");
	
	return 0;
}

