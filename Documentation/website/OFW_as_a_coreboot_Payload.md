# OFW as a coreboot payload
This page tells how to build Open Firmware as a coreboot payload and run
it under QEMU. You can also run Open Firmware under QEMU without
coreboot:

- You can build a QEMU ROM image directly from code in the OFW tree -
  see [Building OFW for QEMU](Building_OFW_for_QEMU).
- You can boot OFW from a conventional BIOS - see
  [Building OFW to Load from BIOS](Building_OFW_to_Load_from_BIOS)

## Software Requirements

- qemu-0.9.1
- Open Firmware rev. \>= 1051
- coreboot \>= v2
- GCC and GNU make - OFW builds with most versions; I'm not sure which
  versions coreboot needs

## Building Open Firmware

Download the Open Firmware source:

    svn co svn://openfirmware.info/openfirmware

Configure OFW for coreboot:

    cd openfirmware/cpu/x86/pc/biosload
    cp config-coreboot.fth config.fth

Build OFW:

    cd build
    make

After make is finished (it shouldn't take long) there should be a file
"ofwlb.elf" in the same directory. Copy this to your coreboot-v\[x\]
directory.

## Building coreboot

Follow the instructions in the coreboot documentation, using ofwlb.elf
as your payload file.

## Getting QEMU

Get QEMU \>= 0.9.1 from <http://bellard.org/qemu/download.html>

Version 0.9.1 should "just work". It supports reasonably large ROM
images, determining the emulated ROM size from the size of the image
file. There was a "qemu_biossize.patch" for qemu-0.9.0, but the site
that hosted that patch is defunct.

## Run Your ROM Image

    qemu  -L coreboot-v3/build -hda path/to/disk.img -serial `tty` -nographic 

## Ideas for Improvement

These instructions build a rather plain OFW configuration that lacks
drivers for several of QEMU's specific I/O devices (Ethernet, video,
sound). Suitable drivers are in the OFW tree, and are included in the QEMU
build described in [Building OFW for QEMU](Building_OFW_for_QEMU). It
would be nice to add those drivers to the configuration described
herein. If the Cirrus video driver were added, qemu could be used in
graphic mode.
