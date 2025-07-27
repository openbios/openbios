# Building OFW for ARM
This page tells how to build Open Firmware for an ARM target system.

## Host System Requirements

During the Open Firmware build process, the Forth system inside OFW is
executed several times to extend itself. Those steps must run on the
same CPU instruction set (in this case ARM) as the final target. So you
must build OFW either on an ARM computer or by using an ARM instruction
set emulator running on some other computer.
[QEMU](http://wiki.qemu.org/) works well for this purpose.

### Compiler Setup for Native ARM Host

To compile on an ARM host, you need Linux on that host, along with the
GNU toolchain (GCC), GNU make, and the Subversion version control
system.

### Compiler Setup for x86 Host

To compile on an x86 host, you need everything listed for the native
host (Linux, GNU toolchain, GNU make, and Subversion), plus QEMU.

Note that the GNU toolchain that you need is the native x86 one, not an
ARM cross-toolchain. OFW uses its own built-in ARM assembler. The native
x86 toolchain is needed for the one-time step of compiling a small
interface program that lets the OFW builder operate under Linux. It
would be possible to eliminate that need by distributing a precompiled
binary of that interface program, but in practice, most developers
already have the native x86 toolchain already installed, so requiring it
isn't a problem (as opposed to cross-toolchains, which can be a serious
hassle to set up correctly).

To install QEMU:

- Debian: sudo apt-get install qemu
- Ubuntu 8.04 (Hardy): sudo apt-get install qemu
- Ubuntu 9.10 (Karmic): sudo apt-get install qemu qemu-kvm-extras
- Fedora: sudo yum install qemu-user

In general, the file that you need is "qemu-arm". You can use commands
like (Fedora) "yum whatprovides qemu-arm" or (Debian/Ubuntu) "apt-file
search qemu-arm" to work out which top-level package to install.

For Fedora, you might also need to temporarily disable SELinux security
before OFW compilation:

    sudo /usr/sbin/setenforce 0

## Building Open Firmware

Get the Open Firmware source:

    svn co svn://openfirmware.info/openfirmware

Build OFW:

    cd openfirmware/cpu/arm/mmp2/build
    make

The last line of the compilation output should say something like:

    --- Saving as ofw.rom

That tells you the name of the output file.

There are some other build directories like "cpu/arm/versatilepb/build".
They work the same way - just type "make" in the directory. At present,
those other versions are fairly rudimentary, containing core OFW plus a
serial driver for interaction, but lacking drivers for most of the other
hardware on those systems. Essentially, they are "quick ports to get an
ok prompt". The MMP2 version is quite a bit more complete, on its way to
being a fully-fledged OFW system. The MMP2 is Marvell's hardware
development platform for their PXA688 chip, which is slated for use in
the OLPC XO-1.75 system. We are using the MMP2 to get a jump-start on
XO-1.75 development.
