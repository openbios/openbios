# Building OFW for OLPC
This page tells how to build the OLPC (One Laptop Per Child) version of
Open Firmware. This is the version of OFW that is normally shipped on
that machine. You can get precompiled ROMs from
<http://dev.laptop.org/pub/firmware> , so you only need to recompile
from source if you want to change something. See
<http://wiki.laptop.org/go/Firmware> for version history.

## Software Requirements

- Open Firmware source code
- GCC and GNU make - most versions are okay for building OFW
- IASL (Intel ASL compiler, for compiling ACPI tables) - most Linux
  distributions have an "iasl" package.

## Building Open Firmware

Get the Open Firmware source:

    svn co svn://openfirmware.info/openfirmware

Build OFW:

    cd openfirmware/cpu/x86/pc/olpc/build
    make

The last line of the compilation output should say something like:

    --- Saving as q2e24.rom

That tells you the name of the output file. You can install it in your
OLPC XO computer by copying that file to a USB FLASH key, inserting the
key into the XO, and typing on the XO:

    ok flash u:\q2e24.rom
