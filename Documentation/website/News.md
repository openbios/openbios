# News
**FCODE suite 1.0.2 released** (2006-10-30)

[David Paktor](mailto:David@paktor.biz) sent a new fork of the
[OpenBIOS FCODE suite](FCODE_suite). After only a
couple of days we finally got this merged into the official code base.

**LinuxBIOS Symposium 2006** (2006-10-01)

The first European [LinuxBIOS Symposium
2006](http://www.linuxbios.org/index.php/LinuxBIOS_Symposium_2006) took
place in Hamburg from October 1st to 3rd. This event was organized by
[coresystems GmbH](http://www.coresystems.de).

**FCODE suite 1.0.1 available** (2006-09-21)

[David Paktor](mailto:David@paktor.biz), on behalf of the IBM
Corporation, contributed to the [OpenBIOS FCODE suite](FCODE_suite). This release has
higher test coverage, a more detailed report on one type of error, and
removal of dead code and consolidation of some other code.

**SUN released OpenBOOT source code** (2006-09-06)

[SUN microsystems](http://www.sun.com/) has recently released their
OpenBOOT source code to the community under a BSD license. Go to their
[OpenSparc T1
website](http://opensparc-t1.sunsource.net/download_sw.html)\] to
download the full archive (190MB) or check out our [local
mirror](http://www.openbios.org/~stepan/sun-obp.tar.bz2) (1.7MB).

**New FCODE suite available** (2006-08-08)

[David Paktor](mailto:David@paktor.biz), on behalf of the IBM
Corporation, contributed some groundbreaking feature updates to the
OpenBIOS FCODE utilities toke, detok and romheaders. Find details and
binaries here

**Sparc32 support in development** (2006-05-05)

OpenBIOS has a port for 32bit Sparc CPUs now. The initially targeted
milestone is to get a full firmware implementation for the QEMU project.

**OpenBIOS switched to Subversion** (2006-04-26)

See the [development repository](OpenBIOS) for more
information.

**Bugfix version of tokenizer toke** (2005-10-15)

Version 0.6.10 of the OpenBIOS tokenizer toke has been released. This is
a bugfix release.

**New version of toke** (2005-10-05)

Version 0.6.9 of the OpenBIOS tokenizer toke has been released. This is
a bugfix release.

**New versions of toke and detok** (2005-03-10)

New versions of the OpenBIOS utilities have been released: The tokenizer
toke 0.6.8 and the detokenizer detok 0.6.1.

**Stallman calls for action on Free BIOS** (2005-02-26)

FSF President Richard M. Stallman was calling for action on free BIOS in
his speech at FOSDEM 2005. Read about The Free Software Foundation's
Campaign for Free BIOS.

**Portability and cross development** (2005-01-12)

OpenBIOS is pushing towards new hardware platforms. Improvements in the
cross compilation ability of OpenBIOS are currently merged into a
seperate development tree. This will allow to develop and test OpenBIOS
for ARM, PPC, AMD64 and others just using an ordinary PC.

**OpenBIOS boots, part II** (2004-08-01)

After some major cleanup, OpenBIOS now also boots on real PPC hardware:
The Total Impact Briq.

**Linux Tag** (2004-06-27)

The OpenBIOS team went to the LinuxTag, hacking and chatting at the
Forth e.V. booth. OpenBIOS can run forth files from a filesystem now and
it has a preliminary PCI driver.

**OpenBIOS boots** (2004-05-23)

After many bugfixes OpenBIOS booted Linux on real hardware now (AMD64
and x86).

**IDE support** (2004-01-23)

Jens Axboe wrote an IDE driver for OpenBIOS. This will help OpenBIOS to
boot on real hardware soon.

**Device and Client Interface** (2004-01-10)

OpenBIOS' interfaces for device and client (OS) interaction are
basically finished. When used in MacOnLinux (MOL), OpenBIOS can boot
MacOS and Linux.

**Bitkeeper repository** (2003-12-20)

Most of OpenBIOS moved to a bitkeeper repository. Check out
bk://openbios.bkbits.net/unstable for the latest development tree.

**Toke and Detok update** (2003-11-29)

Toke and Detok, OpenBIOS' FCode toolchain, have been updated. Toke can
now be used to directly create PCI Option ROMs with FCode. Both toke and
detok got a couple of bugfixes and integrate cleanly in the OpenBIOS
build process now.

**OpenBIOS forth kernel 1.1** (2003-10-12)

After the first release here's an update with some fixes, optimized
speed and a plugin system that allows easier development under a
Linux/Unix system.

**OpenBIOS forth kernel released** (2003-09-16)

After some months of development we are happy to announce the new
OpenBIOS forth kernel "BeginAgain".

**New domain: openbios.org!** (2003-08-23)

We got it! After many years of hazzling with a domain grabber we finally
got the domain [openbios.org](http://openbios.org/).

**dictionary dumping** (2002-10-16)

OpenBIOS' forth kernel paflof knows how to dump dictionaries now. Read
more information on how to use this feature in the
[mailing list archive](Mailinglist).

**LinuxFund.org** (2002-07-04)

OpenBIOS is one out of three projects funded in LinuxFund.org's Spring
Grant Cycle 2002. The Development Grant of \$1000 will be used to
advance the project.

**toke and detok update** (2002-05-26)

Toke 0.4 and detok 0.5.2 have been released. Detok has some new
features, such as line numbers or byte offsets, proper checksum
calculations, 64bit opcodes and several bug fixes. Toke comes with
better error messages, working case...endcase constructs, improved
number parsing, better IEEE compliance and less bugs.

**toke 0.2 released** (2002-03-20)

The new version of toke is quite an improvement compared to the last
release. Most of the missing control words and tokenizer directives are
supported now, string handling was improved, error messages and warnings
contain line numbers etc. This component needs heavy testing.

**toke 0.1 released** (2002-03-04)

Time is passing by and the OpenBIOS project has a tokenizer now. Even
though some things are yet missing, it is capable to tokenize quite a
huge amount of test code fed into it.

**detok 0.3 released** (2002-02-26)

The IEEE-1275 FCode detokenizer got some cleanup. Dictionary is no more
autogenerated, memory consumption reduced by almost 70%.

**/dev/bios 0.3.2 released** (2002-02-19)

After having almost no releases in the last 2 years, /dev/bios shows up
in version 0.3.2 now. It has support for NSC CS5530(A), AMD 7xx,
ServerWorks, Intel 4x0/8xx and other chipsets.

**new version of /dev/bios available** (2002-02-15)

The kernel level firmware flasher /dev/bios is under control of the
OpenBIOS CVS now. /dev/bios now supports quite a big number of
motherboard chipsets.

**new version of feval released** (2001-12-16)

FCode evaluator feval-0.2.6 released. This is a bug fix release.

**Talk to us LIVE!** (2001-12-03)

Yesterday we moved from IRCNet to irc.freenode.net. You can talk with us
at \#OpenBIOS usually all the day (CET timezone)

**new version of detok released** (2001-12-02)

FCode detokenizer detok-0.2.3 released. This version should compile on
any ANSI C Compiler and has corrected FCode names. Check the status page
for more details.

**OpenBIOS is now under control of CVS** (2001-11-12)

The OpenBIOS development CVS is up and running.

**new version of feval released** (2001-11-08)

FCode evaluator feval-0.2.5 released. This version has preliminary
package support.

**new version of detok released** (2001-10-15)

FCode detokenizer detok-0.2.2 released. This version fixes indentation
and a string allocation error.

**new version of detok and feval released** (2001-09-16)

Update: FCode evaluator feval-0.2 and detokenizer detok-0.2.1 available
now.

**we now have a detokenizer** (2001-09-09)

There's a first version of an fcode detokenizer now. This is the first
part of the OpenBIOS development suite.

**general news on bios-coding** (2000-02-13)

The LinuxBIOS Project replace the firmware of their Rockhopper cluster
machines with a special Linux kernel image, and instead of running the
BIOS on startup they run Linux. For information and patches, check out
their homepage at <http://www.linuxbios.org/>.
