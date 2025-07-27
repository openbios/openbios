# Building OFW to Load from BIOS
This page tells how to build the x86 version of Open Firmware to boot
under a conventional BIOS. These instructions specifically describe a
configuration that is good for running on the QEMU and VirtualBox
emulators, but very similar instructions can be used to make a version
that runs on any PC. In this version, OFW is stored on a floppy disk
image. The conventional BIOS boots it as if it were an operating system
like DOS or Windows. OFW then "takes over" the machine, ignoring the
conventional BIOS that booted it. That BIOS has already done the work of
early-startup machine initialization - turning on the memory controller
and configuring chipset-dependent registers.

In addition to this BIOS-loaded version, there are two other ways to run
OFW under QEMU:

- You can build an OFW image that replaces the QEMU BIOS ROM image,
  entirely from code in the OFW tree - see
  [Building OFW for QEMU](Building_OFW_for_QEMU).
- You can build an OFW image that replaces the QEMU BIOS ROM image,
  using OFW as a "payload" for coreboot, so coreboot does the
  early-startup stuff - see [OFW as a coreboot Payload](OFW_as_a_coreboot_Payload).

## Software Requirements

- qemu-0.9.1 or VirtualBox
- Open Firmware rev. \>= 1053
- GCC and GNU make - most versions work for builing OFW

## Getting Open Firmware Source

    $ svn co svn://openfirmware.info/openfirmware

## Building OFW

    $ cd cpu/x86/pc/biosload/
    $ cp config-virtualbox.fth config.fth
    $ cd build
    $ make floppyofw.img

The "config-virtualbox.fth" configuration is known to work with QEMU.
Other configurations may work also - but the "qemu-loaded" config option
isn't what you want for this technique, because it's a subcase of the
CoreBoot-payload configuration.

You will use the "floppyofw.img" output file in a later step.

## Making a bootable Floppy Image

    $ ../../../Linux/forth fw.dic ../makefloppy.fth

This creates a file that is a bootable floppy image with an empty FAT12
filesystem. This step only has to be done once.

## Copying OFW onto the Floppy Image

    $ mkdir flp
    $ sudo mount -t msdos -o loop floppy.img flp
    $ sudo cp floppyofw.img flp/ofw.img
    $ sudo umount flp

Copy floppy.img to the QEMU or VirtualBox directory.

## Booting OFW from QEMU

    $ qemu -L *dir* -boot a -fda floppy.img

"*dir*" is the directory that contains QEMU's BIOS and VGA BIOS files.

## Booting OFW from VirtualBox

The following VirtualBox configuration settings work for me:

- OS Type = Other/Unknown
- Base Memory = 64 MB
- Video Memory = 8 MB
- Boot Order = Floppy, Hard Disk
- ACPI = Disabled
- IO APIC = Disabled
- VT-x/AMD-V = Disabled
- PAE/NX = Disabled
- Floppy Image = floppy.img
- Network Adapter 1  = PCnet-PCI II (host interface, `<tap name>`)
- (other settings are mostly irrelevant)

## Recompiling

If you want to make changes and recompile OFW, you need not repeat the
"makefloppy" step; you can just loopback mount the floppy image and copy
the new OFW version to ofw.img .

## What is on the Floppy Image

The floppy image is a bootable floppy with a FAT12 filesystem. Its first
two sectors contain a simple bootloader program that uses BIOS INT 13
callbacks to read floppy sectors. The program scans the FAT root
directory entries to find the file "ofw.img", then loads that into
memory and jumps to it.

When you build floppyofw.img, as a side effect it also builds
bootsec.img, which is that simple bootloader. The source code
(Forth-style assembly language) is in biosload/bootsec.fth .

The "makefloppy.fth" program that creates the image is pretty simple; it
copies bootsec.img to the output file "floppy.img", creates a couple of
initially empty FAT tables, zeros the root directory area, and fills the
data area with zeros.

## Making a Prototype Floppy Image with Linux Commands

Here's a pair of Linux commands that accomplish the same thing as
makefloppy.fth:

    Step6a $ /sbin/mkdosfs -C -f 2 -F 12 -R 2 -r 224 -s 1 -S 512 floppy.img 1440
    Step6b $ dd <bootsec.img of=floppy.img conv=nocreat,notrunc

The arguments to mkdosfs force the filesystem layout to match the layout
that is specified in the BIOS parameter block in bootsec.img.

The advantage of makefloppy.fth is that it reads the filesystem layout
parameters from the BPB in bootsec.img, so its guaranteed to be
consistent. If bootsec.fth were edited to change the layout, the
arguments to "mkdosfs" would have to change. (But there's little need to
change that layout, since it's a standard floppy size.)

The advantage of the Linux command sequence is that it creates a file
with "holes", thus saving disk space for awhile (until something fills
in the holes).

## Booting on a Real PC from a USB Key

To build for a real PC:

    $ cd openfirmware/cpu/x86/biosload
    $ cp config-usbkey.fth config.fth
    $ cd build
    $ make

That will create a file named "ofw.c32" that can be booted from a USB
key that has "syslinux" installed on it.
<http://www.911cd.net/forums//index.php?showtopic=21902> has some tips
on how to install syslinux.

Your syslinux.cfg file needs a line that says:

    kernel ofw.c32

## Booting on a Real PC via GRUB or Etherboot

You can make a version in "Multiboot" format that you can boot with
GRUB:

    $ cd openfirmware/cpu/x86/biosload
    $ cp config-grub.fth config.fth
    $ cd build
    $ make

The output file is "ofwgrub.elf". Copy that in to /boot on your
GRUB-enabled disk and add this to /boot/grub/menu.lst:

    title Open Firmware
    kernel /boot/ofwgrub.elf

That version can also be booted via Etherboot. Here's an example of how
to Etherboot it with QEMU (assuming that the ofwgrub.elf file is in /tmp
on the host machine):

    qemu -L . -boot n -tftp /tmp -bootp /ofwgrub.elf
