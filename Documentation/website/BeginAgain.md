# What is BeginAgain?

BeginAgain is the Forth kernel, basically the heart of OpenBIOS.

Right after the system is far enough to execute any C code, this kernel
will take over control and execute the forth code part of OpenBIOS.

BeginAgain supports the Forth language command group of IEEE 1275-1994
and passes the Hayes ANS forth compliance test.

# How does BeginAgain work?

The OpenBIOS forth core "BeginAgain" is split into a forth kernel
written in C and a forth dictionary which operated on by the kernel.

BeginAgain's approach is indirect threading. Forth words are compiled to
execution tokens (pointers to the words dictionary entries). Only the
prim words (minimal language support) are available as native C code.
When building the forth core, you get different versions of the forth
kernel:

- a "hosted" unix binary. This binary can be used on a unix system:
  - to execute a forth dictionary from a file. This can be used for
    testing openbios code in a development environment on a unix host.
  - to create a dictionary file. Such a dictionary file sets up all of
    the forth language. Primitives are indexed to save relocations.

The default is to create a forth dictionary forth.dict from
forth/start.fs. This file includes all of the basic forth language
constructs from forth/bootstrap.fs and starts the interpreter.

To achieve this, the hosted unix version contains a basic set of forth
words coded in C that allow creating a full dictionary.

- a varying number of target specific binaries.

On x86 you can start openbios for example from GRUB or coreboot. They
are all based on the same forth engine consisting of a dictionary
scheduler, primitive words needed to build the forth environment, 2
stacks and a simple set of console functions. These binaries can not be
started directly in the unix host environment.

# Building and Using BeginAgain

## Requirements

- gcc
- grub or any other multiboot loader to run the standalone binary
  "openbios.multiboot"

## Building and Usage

- make

this builds "openbios.multiboot", the standalone image and "unix", the
hosted image. Additionally it creates a forth dictionary file from
forth/start.fs. All generated files are written to the absolute
directory held by the variable BUILDDIR, which defaults to
obj-\[platform\]. Some compile time parameters can be tweaked in
include/config.h

- use "unix" to create a forth dictionary on your own:

   $ ./unix -Iforth start.fs

creates the file forth.dict from forth source forth/start.fs.

- use "unix" to run a created dictionary:

   $ ./unix forth.dict

This is useful for testing.

- booting openbios

You can boot openbios i.e. in grub. Add the following lines to your
menu.lst:

   title openbios
     kernel (hd0,2)/boot/openbios.multiboot
     module (hd0,2)/boot/openfirmware.dict

Note: change (hd0,2) to the partition you copied openbios and forth.dict
to.

To boot OpenBIOS from coreboot/etherboot, you can either use "openbios"
or "openbios.full":

- openbios is the pure kernel that loads the dictionary from a hardcoded
  address in flash memory (0xfffe0000)
- openbios.full also includes the dictionary directly so that it can be
  easily used from etherboot or the coreboot builtin ELF loader without
  taking care of the dictionary

# Dictionary Format

The dictionary is a linked list of forth word definitions. Each forth
word in this list looks like the following: name length of name in
bytes + 0x80 align with 0's flags (bit 7 set) LFA CFA PFA

When the forth interpreter looks for a certain word, it reads the
variable last that always points to the last defined word and iterates
over the list until it finds an appropriate word.

# Glossary

## Dictionary

- LFA == link field address (backlink)
- CFA == code field address ("word type")
- PFA == program field address (definitions)

## Forth Engine

- TIB == text input buffer
- inner interpreter: interprets dictionary, does threading
- outer interpreter: "user" interpreter, reads forth words from user.
