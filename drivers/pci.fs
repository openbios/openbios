[IFDEF] CONFIG_DRIVER_PCI

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

: pci-addr-encode ( addr.lo addr.mi addr.hi )
  rot >r swap >r 
  encode-int 
  r> encode-int encode+ 
  r> encode-int encode+
  ;
 
: pci-len-encode ( len.lo len.hi )
  encode-int 
  rot encode-int encode+ 
  ;



[THEN]

