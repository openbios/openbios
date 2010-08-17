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

1 encode-int " #address-cells" property
1 encode-int " #size-cells" property
h# 05f5e100 encode-int " clock-frequency" property

new-device
	" cpus" device-name
	1 encode-int " #address-cells" property
	0 encode-int " #size-cells" property
	external

	: encode-unit ( unit -- str len )
		pocket tohexstr
	;

	: decode-unit ( str len -- unit )
		parse-hex
	;

finish-device

new-device
	" memory" device-name
	" memory" device-type
	external
	: open true ;
	: close ;
finish-device

new-device
	" hypervisor" device-name
	" hypervisor" device-type
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
