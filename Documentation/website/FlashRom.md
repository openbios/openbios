# Flash Updates with Linux

Formerly OpenBIOS provided its own flashing facility implemented as a
device driver called **/dev/bios**. One big disadvantage of
**/dev/bios** was that it needed to be recompiled for every minor kernel
update.

Ollie Lho, back when working at SIS, started a new effort which he
called **flash_and_burn**. This utility became part of the [coreboot
project](http://www.coreboot.org) and evolved to work with non-SIS
chipsets.

Stefan Reinauer added a lot of new features to Ollie's utility and
renamed it to **flashrom**. This utility still lacks some features the
old **/dev/bios** driver was having, but it can easily be used from
userspace without recompiling the kernel.

For more information about flashrom see <http://www.flashrom.org/>
