#
#                     OpenBIOS - free your system! 
#                            ( detokenizer )
#                          
#  This program is part of a free implementation of the IEEE 1275-1994 
#  Standard for Boot (Initialization Configuration) Firmware.
#
#  Copyright (C) 2001-2005 Stefan Reinauer, <stepan@openbios.org>
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

ARCH     := $(shell uname -m | sed -e s/i.86/x86/ -e s/sun4u/sparc64/ -e s/arm.*/arm/ -e s/sa110/arm/ -e s/x86_64/amd64/ -e "s/Power Macintosh/ppc/")
TOPDIR   := $(shell /bin/pwd)
BUILDDIR ?= $(TOPDIR)/obj-$(ARCH)

VPATH    := $(BUILDDIR)


include $(TOPDIR)/Rules.make


CC	  =  gcc
CFLAGS	  =  -O2 -Wall -ansi

# For debugging the indentation code of detok, define DEBUG_INDENT
#CFLAGS	 :=  $(CFLAGS) -DDEBUG_INDENT

all: main detok
	@echo -e "\nOpenBIOS detokenizer detok build finished\n"

main:
	@echo -e "\nWelcome to the OpenBIOS detokenizer.."
	@test -r $(BUILDDIR) || ( mkdir -p $(BUILDDIR); \
	  echo -e "\nCreating build directory $(BUILDDIR)" )
		
detok: detok.o dictionary.o decode.o stream.o 
	@echo -en "\nLinking fcode detokenizer detok..."
	@cd $(BUILDDIR) && ( $(CC) $(CFLAGS) $^ -o $@; strip detok )
	@echo -e "\tok"
	
clean:
	@test ! -d $(BUILDDIR) && \
		echo "Architecture $(ARCH) is already clean." || \
		( 						 \
			echo "Cleaning up architecture $(ARCH)"; \
			rm -rf $(BUILDDIR)			 \
			rm forth.dict.core			 \
		)

distclean: clean
	rm -f detok

detok.o:      detok.h stream.h detok.c
stream.o:     detok.h stream.c
decode.o:     detok.h stream.h decode.c Makefile
dictionary.o: detok.h dictionary.c
