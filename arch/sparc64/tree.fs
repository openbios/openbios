
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

dev /

\ simple pci bus node
new-device
  " pci" device-name
finish-device

dev /pci

\ simple isa bus node
new-device
  " isa" device-name
finish-device
