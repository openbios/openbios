# OpenBIOS

Welcome to the OpenBIOS download page. Here you'll find releases of
OpenBIOS components.

After 4 years of hard work, OpenBIOS v1.1 has been released. The new
features include:

* Internal memory API (OFMEM) implementation
* Forth Source Debugger
* 64-bit 1275 6d5 implementation
* Forth Local Variables
* Internal libopenbios code reorganisation

See the [OpenBIOS issue
tracker](https://github.com/openbios/openbios/issues) for milestones,
tasks and open bugs.

# OpenBIOS - Code Releases

Download the latest release of OpenBIOS including the Forth kernel and
all of the IEEE 1275-1994 compliant Forth code for user interface,
client interface and device interface.

Latest release version is: [OpenBIOS
1.1](https://github.com/openbios/openbios/archive/v1.1.zip) (2013-05-04)

**NOTE:** The FCODE utilities are no longer part of the main OpenBIOS
distribution. Have a look at the
[FCODE suite](FCODE_suite) if you are looking for toke and detok.

# Status and use cases

OpenBIOS can be used directly as a boot ROM for [QEMU](http://qemu.org/)
system emulators for PPC, PPC64, Sparc32 and Sparc64.

OpenBIOS/SPARC32 is currently able to boot the following OS/kernels:

* Linux
* NetBSD
* OpenBSD
* Solaris

OpenBIOS/SPARC64 is currently able to boot the following OS/kernels:

* Linux
* NetBSD
* OpenBSD
* FreeBSD
* HelenOS

OpenBIOS/PPC is currently able to boot the following OS/kernels:

* Linux
* HelenOS
* Darwin/Mac OS X

The following operating systems will partially boot, but may suffer from
some emulation bugs under QEMU:

* FreeBSD
* NetBSD
* Mac OS 9

[coreboot](http://www.coreboot.org) can use OpenBIOS as a payload on
x86.

Do not try to put OpenBIOS in a real boot ROM, it will not work and may
damage your hardware!

## Kernel

There is also an ancient stand-alone version of the OpenBIOS Forth
kernel *BeginAgain*.

The last released stand-alone version is: [BeginAgain
1.1](http://www.openbios.org/data/bin/kernel-1.1.tar.bz2) (2003-10-12).

**NOTE:** You should use the latest version of *BeginAgain* that is
present in the complete *OpenBIOS release* above. It is much newer than
*BeginAgain 1.1* and it supports cross compiling and lots of other nifty
features. *BeginAgain 1.1* is here for educational purposes only: The
core binary is only 6k on x86.

# Development Environment

## FCode Suite

To download the latest version of the FCode Suite, including an FCode
detokenizer, an FCode tokenizer and the romheader utility, please go to
the [FCode Suite page](FCODE_suite).

## Flashing

/dev/bios is obsolete and has been replaced by a new and better
utility.  Please download a coreboot snapshot and use the [flashrom
utility](FlashRom) from *coreboot-v2/util/flashrom*.

# Development Repository

OpenBIOS keeps its development tree in a [git](http://git-scm.com/)
repository. If you do not want to use git, please have a look at the
Snapshots below.

## Anonymous access

You can check it out as follows:

    $ git clone https://github.com/openbios/openbios.git

or for checking out the source code for the OpenBIOS FCode Suite:

    $ git clone https://github.com/openbios/fcode-utils.git

## Developer access

Access for developers is very similar to anonymous access. Just add your
github username as follows when checking out the repository:

    $ git clone https://username@github.com/openbios/openbios.git

# Source code browsing

You can also browse the [OpenBIOS github repository](https://github.com/openbios/openbios) online.

# Snapshots

There is currently no archive of snapshots available for OpenBIOS. You
can use the [source code browser](https://github.com/openbios/openbios)
to download a ZIP archive of any revision.

Alternatively you can also download the [most current
snapshot](https://github.com/openbios/openbios/archive/master.zip)
directly.

# Building OpenBIOS

Download fcode suite:

    $ git clone https://github.com/openbios/fcode-utils.git

Build the needed programs inside the fcode-utils-devel folder:

    $ make

Install the programs:

    $ make install

Download OpenBIOS:

    $ git clone https://github.com/openbios/openbios.git

Select the build targets:

    $ ./config/scripts/switch-arch sparc32 sparc64 x86 ppc amd64

Build OpenBIOS:

    $ make

or

    $ make build-verbose

OpenBIOS can even be cross-compiled on a host which is different type
(big vs. little endian and 32 vs. 64 bits) from the target. At least
Linux and OpenBSD hosts are known to work.

If your cross tools use different prefix from what the makefiles assume,
the prefix can be overridden with:

    $ make build-verbose TARGET=powerpc-elf-

or

    $ make -C obj-ppc CC=powerpc-elf-gcc

The OpenBIOS binaries (typically openbios-builtin.elf) can be found in
obj- subdirectories. The Unix executable version (native only) is named
openbios-unix.

# Additional Resources

[PowerPC, x86, ARM, and Sparc elf cross-compilers for Mac OS
X](http://www.mediafire.com/download/wy5xgj2hwjp8k4k/AWOS_Cross-Compilers.zip)

- This compiler uses a unsupported compiler prefix. To use it, set the
  `CROSS_COMPILE` variable to "ppc-elf-" before running the switch-arch
  script.

Example:

    CROSS_COMPILE=ppc-elf- ./switch-arch ppc

# Notes for Building on Mac OS X

There is a known build issue when building on Mac OS 10.6. The
switch-arch script will report your computer as 32 bit (x86) when it is
really 64 bit (amd64). If you see the message "*panic: segmentation
violation at …*" while building, you probably have this problem.

If this happens to you, try setting the HOSTARCH variable before using
the switch-arch script.

Example:

    HOSTARCH=amd64 ./switch-arch ppc

# Troubleshooting

Seeing this message: *Unable to locate toke executable from the
fcode-utils package - aborting*
- Install the fcode suite first before trying to build OpenBIOS.
