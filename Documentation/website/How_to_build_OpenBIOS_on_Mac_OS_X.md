# How to build OpenBIOS on Mac OS X
Apple's GCC on Mac OS X does not support the elf binary format. So
compiling OpenBIOS on Mac OS X is not easy. Thankfully someone has
already made a PowerPC cross compiler for Mac OS X that can compile
OpenBIOS. Note: this tutorial was made with Mac OS 10.6. I suggest you
use Mac OS 10.5 or higher when trying to build OpenBIOS.


<FONT COLOR="#AA0000"><B><U>Part 1: Installing the cross compiler on Mac
OS X</U>
</B></FONT>
1). Download the AWOS cross compiler:
<http://awos.wilcox-tech.com/downloads/AWOS-CC.bz2>

2) Expand this bz2 file by double clicking on it.

3) Rename the file AWOS-CC to AWOS-CC.img.

4) Open the image file.

5) Open the "AWOS Cross-Compiler for OS X" file on the newly mounted
disk.

6) Select the "PowerPC Support" and the "i386 Support" check boxes when
given the option.

7) Continue with the installation until it is finished.

8) Add the new compiler's folder to the PATH variable. This is the
command you use if you are in the Bash shell:
export PATH=\$PATH:/usr/local/ppcelfgcc/bin
export PATH=\$PATH:/usr/local/i386elf/i386elf/bin
Note: the above step will probably need to be repeated with each new
session of the shell you start. It can be made permanent by altering the
.bash_profile file for the Bash shell. It is located in your home
folder.
To open this file use these commands:
cd ~
open .bash_profile
Then simply paste the export commands in this file. Save changes when
you are done.

To test to see if the cross compilers work issue these commands:
i386-elf-gcc --version
ppc-elf-gcc --version

You should see a message like this print:
ppc-elf-gcc (GCC) 4.2.3 Copyright (C) 2007 Free Software Foundation,
Inc. This is free software; see the source for copying conditions. There
is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.
This should conclude installing the compiler.


<FONT COLOR="#AA0000"><B><U>Part 2: Building OpenBIOS from source
code</U>
</B></FONT>
1) Open the Terminal application.

2) Change the current directory to the inside of the openbios-devel
folder.

3) Open the file makefile.target and add a \# in front of this text
"CFLAGS+= -Werror". Save the changes when done.

4) Type this command in the terminal and watch the show:

    CROSS_COMPILE=ppc-elf- ./config/scripts/switch-arch ppc && make build-verbose

There is a known build issue when building on Mac OS 10.6. The
switch-arch script will report your computer as 32 bit (x86) when it is
really 64 bit (amd64). If you see the message "panic: segmentation
violation at …" while building, you probably have this problem.

If this happens to you, try setting the HOSTARCH variable before using
the switch-arch script.

Example:

HOSTARCH=amd64 CROSS_COMPILE=ppc-elf- ./config/scripts/switch-arch ppc
&& make build-verbose

To test out your newly created binary in qemu, use the -bios option:
qemu-system-ppc -bios <path to binary>/openbios-qemu.elf.nostrip

This tutorial was made using information available on 12/21/2017. If you
encounter any problems, please report it to the openbios developer list:
openbios@openbios.org.
