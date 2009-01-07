
" /" find-device
  2 encode-int " #address-cells" property
  2 encode-int " #size-cells" property
  " sun4u" encode-string " compatible" property

  \ : encode-unit encode-unit-sbus ;
  \ : decode-unit decode-unit-sbus ;

new-device
  " memory" device-name
  external
  : open true ;
  : close ;
  \ claim ( phys size align -- base )
  : claim 2drop ;
  \ release ( phys size -- )
finish-device

new-device
  " virtual-memory" device-name
  external
  : open true ;
  : close ;
  \ claim ( phys size align -- base )
  : claim 2drop ;
  \ release ( phys size -- )
finish-device

" /options" find-device
  " disk" encode-string " boot-from" property

" /openprom" find-device
  " OBP 3.10.24 1999/01/01 01:01" encode-string " version" property

device-end

\ we only implement DD and DD,F
: encode-unit-pci ( phys.lo phy.mid phys.hi -- str len )
  nip nip ff00 and 8 >> dup 3 >>
  swap 7 and
  ( ddddd fff )

  ?dup if
    pocket tohexstr
    " ," pocket tmpstrcat
  else
    0 0 pocket tmpstrcpy
  then
  >r
  rot pocket tohexstr r> tmpstrcat drop
;

dev /

\ simple pci bus node
new-device
  " pci" device-name
	3 encode-int " #address-cells" property
	2 encode-int " #size-cells" property
	0 encode-int 0 encode-int encode+ " bus-range" property
	" pci" encode-string " device_type" property

  external
  : open ( cr ." opening PCI" cr ) true ;
  : close ;
  : decode-unit 0 decode-unit-pci-bus ;
  : encode-unit encode-unit-pci ;
finish-device

device-end

dev /pci

\ simple isa bus node
new-device
  " isa" device-name
  " isa" device-type
	2 encode-int " #address-cells" property
	1 encode-int " #size-cells" property

  external
  : open true ;
  : close ;

finish-device
