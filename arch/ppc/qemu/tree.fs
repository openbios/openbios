\   Qemu specific initialization code
\
\   This program is free software; you can redistribute it and/or
\   modify it under the terms of the GNU General Public License
\   as published by the Free Software Foundation
\

\ -------------------------------------------------------------
\ device-tree
\ -------------------------------------------------------------

" /" find-device

" bootrom" device-type
" PowerMac1,1" model
" PowerMac1,1" encode-string
" MacRisc" encode-string encode+
" Power Macintosh" encode-string encode+ " compatible" property
" 0000000000000" encode-string " system-id" property
1 encode-int " #address-cells" property
1 encode-int " #size-cells" property
h# 05f5e100 encode-int " clock-frequency" property

new-device
	" cpus" device-name
	1 encode-int " #address-cells" property
	0 encode-int " #size-cells" property
finish-device

new-device
	" memory" device-name
	" memory" device-type
	external
	: open true ;
	: close ;
finish-device

\ -------------------------------------------------------------
\ /packages
\ -------------------------------------------------------------

" /packages" find-device

	" packages" device-name
	external
	\ allow packages to be opened with open-dev
	: open true ;
	: close ;

\ /packages/terminal-emulator
new-device
	" terminal-emulator" device-name
	external
	: open true ;
	: close ;
	\ : write ( addr len -- actual )
	\	dup -rot type
	\ ;
finish-device

\ -------------------------------------------------------------
\ The END
\ -------------------------------------------------------------
device-end
