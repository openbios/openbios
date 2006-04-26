#
#                     OpenBIOS - free your system! 
#                         ( FCode tokenizer )
#                          
#  This program is part of a free implementation of the IEEE 1275-1994 
#  Standard for Boot (Initialization Configuration) Firmware.
#
#  Copyright (C) 2001-2005  Stefan Reinauer, <stepan@openbios.org>
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; version 2 of the License.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA, 02110-1301 USA
#

ARCH     := $(shell uname -m | sed -e s/i.86/x86/ -e s/sun4u/sparc64/ -e s/arm.*/arm/ -e s/sa110/arm/ -e s/x86_64/amd64/)
TOPDIR   := $(shell /bin/pwd)
BUILDDIR ?= $(TOPDIR)/obj-$(ARCH)
VPATH    := $(BUILDDIR)

include $(TOPDIR)/Rules.make

CC     = gcc
CFLAGS = -O2 -Wall -W -ansi -pedantic
# CFLAGS = -O -g -Wall -ansi -pedantic

# CFLAGS := $(CFLAGS) -DDEBUG_SCANNER
# CFLAGS := $(CFLAGS) -DDEBUG_DSTACK

OBJECTS= toke.o stack.o stream.o dictionary.o macros.o scanner.o emit.o

.SUFFIXES: .c .o

all: main toke
	@cd $(BUILDDIR) && strip toke
	@echo -e "\nOpenBIOS tokenizer toke build finished\n"

# main should go to rules.make
main:
	@echo -e "\nWelcome to toke, the OpenBIOS fcode tokenizer.."
	@test -r $(BUILDDIR) || ( mkdir -p $(BUILDDIR); \
	  echo -e "\nCreating build directory $(BUILDDIR)" )

toke: $(OBJECTS)
	@echo -n "  linking tokenizer executable... "
	@cd $(BUILDDIR) && $(CC) $(CFLAGS) -o $@ $^
	@echo -e "\tok"
	
clean:
	@test ! -d $(BUILDDIR) && \
		echo "Architecture $(ARCH) is already clean." || \
		( 						 \
			echo "Cleaning up architecture $(ARCH)"; \
			rm -rf $(BUILDDIR)			 \
			rm forth.dict.core			 \
		)

toke.o:       toke.c toke.h stack.h stream.h emit.h
dictionary.o: dictionary.c toke.h dictionary.h
emit.o:       emit.c toke.h stack.h
macros.o:     macros.c toke.h
scanner.o:    scanner.c toke.h stack.h stream.h dictionary.h
stack.o:      stack.c toke.h stack.h
stream.o:     stream.c toke.h
