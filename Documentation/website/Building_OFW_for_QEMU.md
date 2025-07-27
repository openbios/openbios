# Building OFW for QEMU
This page tells how to build the x86 version of Open Firmware to run under
the QEMU emulator. In this version, OFW replaces QEMU's normal "bios.bin"
file, so QEMU boots directly into OFW, without a conventional BIOS. This
version supports most of QEMU's I/O capabilities, illustrating some of
the things that OFW can do. See the QEMU Options / Things to Try section.

The OFW tree contains a version of the platform-specific "early startup"
code that is suitable for initializing QEMU's virtual hardware. The
early-startup code for QEMU is as simple as such code can possibly be,
because the QEMU core system "hardware" requires no initialization. On
real hardware, you must turn on PLLs, configure bus bridges and superIO
chips, detect DRAM characteristics and configure memory controllers, and
do other complex chipset-dependent things in the early stages of
startup. QEMU omits all that "magic" stuff that is outside of generic
x86 programming models, so the only system-specific early startup code
for QEMU is a simple memory sizing loop.

In addition to this "direct" version, there are two other ways to run
OFW under QEMU:

- You can build OFW as "payload" for coreboot, so Core boot does the
  early-startup stuff - see [OFW as a coreboot Payload](OFW_as_a_coreboot_Payload).
- You can boot OFW from a conventional BIOS - see
  [Building OFW to Load from BIOS](Building_OFW_to_Load_from_BIOS)

## Software Requirements

- qemu-0.9.1
- Open Firmware rev. \>= 1051
- GCC and GNU make - OFW builds with most versions

## Getting QEMU

Get QEMU \>= 0.9.1 from <http://bellard.org/qemu/download.html>

## Building Open Firmware

Get the Open Firmware source:

    svn co svn://openfirmware.info/openfirmware

Build OFW:

    cd openfirmware/cpu/x86/pc/emu/build
    make

After make is finished (it shouldn't take long) there should be a file
"emuofw.rom" in the build directory. Copy this to your qemu directory.

## Run Your ROM Image

    qemu  -L . -bios emuofw.rom  -hda fat:.

That is the basic invocation; you should get a console window with OFW
running in it. OFW is using the emulated Cirrus graphics display in
linear framebuffer mode, not in VGA mode.

## QEMU Options / Things to Try

QEMU has lots of options that let you do all sorts of fun things. The
following sections illustrate a few of OFW's I/O capabilities.

### Memory Size

If you look in OFW's startup banner, you should see "128 MiB memory
installed". That is QEMU's default memory size. You can change that by
adding "-m NNN" to the qemu command line, for example:

    qemu  -L . -bios emuofw.rom  -hda fat:.  -m 32

OFW as configured will work with power-of-two size between 32 and 2048
MiB (although my QEMU installation only works up to 1024 MB). Those
limits aren't inherent inheret OFW, but rather artifacts of the way this
particular "port" is configured.

### Hard Disk Images

The command line argument "-hda fat:." as shown above makes the current
directory on the host system appear to OFW as an IDE hard disk with a
read-only FAT filesystem. You can inspect it with:

    ok .partitions c
    ok dir c:\

The device name "c" is a devalias. You can see the actual device name
with:

    ok devalias c
    ok devalias

Instead of that "virtual FAT filesystem", you can make use Linux tools
to make a filesystem image. In this example, we'll create a 10 MByte
image with a single partition and an ext2 filesystem:

    $ dd if=/dev/zero of=filesystem.img bs=1M count=10
    $ /sbin/mke2fs -F filesystem.img
    $ dd if=filesystem.img of=fs.img bs=512 seek=1
    $ rm filesystem.img
    $ /sbin/sfdisk -L -uS -C 10 -H 64 -S 32 fs.img \<\<EOF
    ,,L,*
    ;
    ;
    ;
    EOF
    $ mkdir mnt
    $ sudo mount -o loop,offset=512 fs.img mnt
    $ # Now copy some files to "mnt"
    $ sudo umount mnt
    
    $ qemu  -L . -bios emuofw.rom  -hda fs.img

In the QEMU OFW console window:

    ok dir c:\

In the recipe above, the total size (10 MiB) appears on the first "dd"
line (count=10) and on the "sfdisk" line (-C 10). The sfdisk values for
heads (-H 64) and sectors (-S 32) are set so that the cylinder size is
1MiB (64\*32\*512), making it easy to set the overall size with the
cylinders (-C) argument. It is more common to use the maximum values -H
255 -S 63 in order to maximize the possible overall size, but that
results in a cylinder size of just under 8M, which isn't quite as easy
to work with for this example. The recipe uses two "dd" lines instead of
making the image file in one step because the "mke2fs" command doesn't
give you the option of skipping the first sector where the partition map
has to be located.

### Serial Console

You can tell QEMU to emulate a serial port by adding "-serial NAME" to
the command line, as with:

    $ qemu -L . -bios emuofw.rom  -hda fat:.  -serial `tty`

Then you can tell OFW to use the serial console with:

    ok com1 io

QEMU has lots of options for directing the emulated serial port to
various places, including real serial ports on the host system and
network connections to other machines. Read the [QEMU
documentation](http://bellard.org/qemu/qemu-doc.html) (-serial option)
for more information.

You can configure OFW to use the serial port by default, instead of the
graphics screen and emulated PC keyboard. To do so, uncomment the line
"create serial-console" in openfirmware/cpu/x86/emu/config.fth and
re-execute the "make" command. Serial consoles are convenient because
it's usually easy to cut-and-paste Forth code into the window that is
acting as the "terminal emulator" for the serial console; you can't
paste into QEMU's graphical console.

### Networking

OFW has an extensive network stack, supporting many protocols including
TFTP, NFS, HTTP (client and server), and Telnet (client and server).
Here's an example qemu command line with networking support enabled:

    $ qemu -L . -bios emuofw.rom  -hda fat:. -net nic,model=pcnet -net tap

Consult the [QEMU documentation](http://bellard.org/qemu/qemu-doc.html)
(the "-net" option) to learn how to configure the "tap" interface to
connect through to the real network.

Here are some OFW networking commands that you might want to try:

    ok watch-net
    ok debug-net
    ok ping yahoo.com
    ok undebug-net
    ok ping irmworks.com
    ok more http:\\firmworks.com\hello.fth
    ok fload http:\\firmworks.com\hello.fth
    ok telnetd
    telnet://10.20.0.107

Now start a telnet client on some machine, connecting to the IP address
shown. Quitting the telnet client restores the OFW prompt to the QEMU
console.

    ok telnet 10.20.0.5

Instead of "10.20.0.5", use the hostname or IP address of a telnet
server. To exit, type Ctrl-\] .

    ok httpd
    http://10.20.0.107

Browse to the URL shown. Type any key on the QEMU console to exit the
HTTP server. In addition to static pages, the HTTP server can do
server-side scripting with Forth, and of course you do client-side
scripting by serving up Javascript or Java pages. This facility has been
used for an interactive embedded system with a Web interface programmed
entirely in Open Firmware - a multi-headed high-speed camera for
real-time analysis of golf clubs and golf swings.

OFW also supports IPV6 networking, but it's not compiled into this
version.

You can, of course, boot operating systems over the network, using TFTP,
NFS, or HTTP.

### Graphics

OFW uses the display in linear framebuffer mode, so it can display 2D
graphics. To see a simple demo:

    ok fload http:\\firmworks.com\testrect.fth

To clear the screen afterwards:

    ok page

OFW has a graphical menu system that is not enabled in this
configuration. You can navigate it with either the mouse or the
keyboard. OFW can display .BMP files.

### USB

To make QEMU emulate a USB mass storage device, do this:

    $ qemu  -L . -bios emuofw.rom  -hda fat:. -usbdevice disk:fat:.

Then, in OFW:

    ok probe-usb
    ok dir u:\

"p2" is an alias for "probe-usb" for us lazy people. Since OFW tries not
to do stuff "behind your back", if you plug in a new USB device, you
must tell OFW to rescan the USB bus. In this case, it will have already
scanned the device (it does that automatically at startup), but
rescanning is a convenient way to get OFW to show you what is there. You
could instead say:

    ok show-devs usb

but "p2" is easier to type.

In the qemu command, you could attach a writable disk image with, for
example:

    $ qemu  -L . -bios emuofw.rom  -hda fat:. -usbdevice disk:fs.img

assuming that you have created a suitable fs.img file as per the
instructions above. You can't use the same writable image for both -hda
and -usbdevice, but it's okay to use the same read-only image (as with
the fat:<directory> syntax) for both.

OFW has reasonably good support for a wide selection of USB mass storage
devices - flash keys, disks, CDROMs, multi-card readers, etc. OFW also
supports USB keyboards, but I haven't yet figured out how to make that
work under QEMU. OFW supports some USB 2.0 network interfaces and some
USB-to-serial adapters. You might be able to make those work using
QEMU's ability to connect to real USB devices on the host. On real
hardware, OFW supports UHCI (USB 1.1), OHCI (USB 1.1), and EHCI (USB
2.0) host controllers.

### Sound

To enable sound support in QEMU, do this:

    $ qemu  -L . -bios emuofw.rom  -hda fat:. -soundhw sb

In OFW:

    ok select /sound
    ok d# 200 d# 2000 tone
    ok unselect

That plays a 200 Hz sine wave for 2 seconds (2000 mS). OFW can also play
.wav files, but that's not included in this configuration.

QEMU has a lot of options to control the sound emulation; read the
documentation to get totally confused.

OFW's SoundBlaster driver, as included in this configuration, is rather
rudimentary. OFW has better drivers for the more common AC97 sound
hardware.
