# Open Firmware
## Introduction

In 2006 the company of Open Firmware inventor Mitch Bradley, [Firmworks,
Inc](http://firmworks.com/), released their Open Firmware implementation
(OFW) under a BSD license. This code shares some code with SUN's
OpenBOOT implementation. The open source OFW supports x86, PowerPC, and
ARM architectures. Other architectures, including SPARC and MIPS, may be
added as time and interest dictate.

The x86 version is used on the OLPC "XO" computer. The x86 version can
be configured for numerous other environments, including

- Direct QEMU ROM (replacing the "bios.bin" that is supplied with QEMU
- Coreboot payload
- Loadable on directly on top of a conventional PC BIOS (booted from
  floppy or hard disk like an OS). In this configuration it can run on
  an arbitrary PC, or on an emulator like QEMU, VirtualBox, or VMWare.

OFW can boot Linux directly from a disk file (FAT, ext2, ISO9660, or
jffs2 filesystems) without the need for an intermediate bootloader like
LILO or GRUB. The Linux image can be in either ELF format (as created by
the first phase of the Linux kernel compilation process) or in the
"bzImage" format that "wraps" the ELF image. When booting an ELF image,
OFW can read the ELF symbol table so OFW's assembly language debugger
can resolve kernel symbols.

OFW can also boot other ELF standalone images, providing to them
rudimentary "libc" capability. That facility has been used for booting,
for example, Minix, ReactOS, Plan9, Inferno and SqueakNOS. The OLPC
system ROM includes several such "standalone client programs", including
MicroEMACS, memtest86, and NANDblaster (a facility for fast OS updates
over multicast wireless).

On the OLPC system, OFW emulates enough legacy BIOS "real mode INTs" to
boot Windows XP. On another system, OFW supports booting Windows CE.

## Download

You can [browse the source code
online](https://github.com/openbios/openfirmware).

The repository is available through git:

You can check it out as follows:

     $ git checkout https://github.com/openbios/openfirmware.git

## Building Different Versions

- Direct QEMU ROM: see [Building OFW for QEMU](Building_OFW_for_QEMU)
- Coreboot: see [OFW as a Coreboot Payload](OFW_as_a_coreboot_Payload)
- OLPC: see [Building OFW for OLPC](Building_OFW_for_OLPC)
- BIOS-loaded: see [Building OFW to Load from BIOS](Building_OFW_to_Load_from_BIOS)
- ARM: see [Building OFW for ARM](Building_OFW_for_ARM)

## Mailing List

There's an Open Firmware mailing list at:

- <http://www.openfirmware.info/mailman/listinfo/openfirmware>.

The mailing list archive is also available:

- <http://www.openfirmware.info/pipermail/openfirmware/>
