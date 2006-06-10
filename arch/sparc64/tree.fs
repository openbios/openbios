
" /" find-device
  2 encode-int " #address-cells" property
  1 encode-int " #size-cells" property

  \ : encode-unit encode-unit-sbus ;
  \ : decode-unit decode-unit-sbus ;

new-device
  " memory" device-name
  external
  : open true ;
  : close ;
  \ claim ( phys size align -- base )
  \ release ( phys size -- )
finish-device

new-device
  " virtual-memory" device-name
  external
  : open true ;
  : close ;
  \ claim ( phys size align -- base )
  \ release ( phys size -- )
finish-device

" /options" find-device
  " disk" encode-string " boot-from" property

