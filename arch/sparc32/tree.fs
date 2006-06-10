
" /" find-device
  2 encode-int " #address-cells" property
  1 encode-int " #size-cells" property

  " SUNW,SPARCstation-5" encode-string " name" property
  " SPARCstation 5" encode-string " banner-name" property
  " sun4m" encode-string " compatible" property
  " SUNW,501-3059" encode-string " model" property
  h# 0a21fe80 encode-int " clock-frequency" property
  
  " /obio/zs@0,100000:a" encode-string " stdin-path" property
  " /obio/zs@0,100000:a" encode-string " stdout-path" property

  
  : encode-unit encode-unit-sbus ;
  : decode-unit decode-unit-sbus ;

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

new-device
  " iommu" device-name
  2 encode-int " #address-cells" property
  1 encode-int " #size-cells" property
  h# 1000 encode-int " page-size" property
  0 encode-int " cache-coherence?" property
  h# ffee8000 encode-int " address" property
  h# 0 encode-int h# 10000000 encode-int encode+ h# 00000300 encode-int encode+ " reg" property
  external
  : open ( cr ." opening iommu" cr) true ;
  : close ;
  : encode-unit encode-unit-sbus ;
  : decode-unit decode-unit-sbus ;
finish-device

" /iommu" find-device
new-device
  " sbus" device-name
  " hierarchical" device-type
  2 encode-int " #address-cells" property
  1 encode-int " #size-cells" property
  h# 01443fd0 encode-int " clock-frequency" property
  h# 1c encode-int " slot-address-bits" property
  h# 3f encode-int " burst-sizes" property
  h# 0 encode-int h# 0 encode-int encode+ h# 0 encode-int encode+ h# 30000000 encode-int encode+ h# 10000000 encode-int encode+
   h# 1 encode-int encode+ h# 0 encode-int encode+ h# 0 encode-int encode+ h# 40000000 encode-int encode+ h# 10000000 encode-int encode+
   h# 2 encode-int encode+ h# 0 encode-int encode+ h# 0 encode-int encode+ h# 50000000 encode-int encode+ h# 10000000 encode-int encode+
   h# 3 encode-int encode+ h# 0 encode-int encode+ h# 0 encode-int encode+ h# 60000000 encode-int encode+ h# 10000000 encode-int encode+
   h# 4 encode-int encode+ h# 0 encode-int encode+ h# 0 encode-int encode+ h# 70000000 encode-int encode+ h# 10000000 encode-int encode+
   " ranges" property
  external
  : open ( cr ." opening SBus" cr) true ;
  : close ;
  : encode-unit encode-unit-sbus ;
  : decode-unit decode-unit-sbus ;
finish-device

" /iommu/sbus" find-device
new-device
  " SUNW,CS4231" device-name
  " serial" device-type
  5 encode-int 0 encode-int encode+ " intr" property
  5 encode-int " interrupts" property
  h# 3 encode-int h# 0c000000 encode-int encode+ h# 00000040 encode-int encode+ " reg" property
  " audio" encode-string " alias" property
finish-device

" /iommu/sbus" find-device
new-device
  " SUNW,bpp" device-name
  h# 4 encode-int h# 0c800000 encode-int encode+ h# 0000001c encode-int encode+ " reg" property
  h# 33 encode-int 0 encode-int encode+ " intr" property
finish-device

" /iommu/sbus" find-device
new-device
  " SUNW,tcx" device-name
  " display" device-type
  h# 1d encode-int " vbporch" property
  h# a0 encode-int " hbporch" property
  h# 06 encode-int " vsync" property
  h# 88 encode-int " hsync" property
  h# 03 encode-int " vfporch" property
  h# 18 encode-int " hfporch" property
  h# 03dfd240 encode-int " pixfreq" property
  h# 3c encode-int " vfreq" property
  h# 300 encode-int " height" property
  h# 400 encode-int " width" property
  h# 400 encode-int " linebytes" property
  d# 24 encode-int " depth" property
  " no" encode-string " tcx-8-bit" property
  5 encode-int 0 encode-int encode+ " intr" property
  5 encode-int " interrupts" property
  2 encode-int h# 00800000 encode-int encode+ h# 00100000 encode-int encode+
   2 encode-int encode+ h# 02000000 encode-int encode+ h# 00000001 encode-int encode+
   2 encode-int encode+ h# 04000000 encode-int encode+ h# 00800000 encode-int encode+
   2 encode-int encode+ h# 06000000 encode-int encode+ h# 00800000 encode-int encode+
   2 encode-int encode+ h# 0a000000 encode-int encode+ h# 00000001 encode-int encode+
   2 encode-int encode+ h# 0c000000 encode-int encode+ h# 00000001 encode-int encode+
   2 encode-int encode+ h# 0e000000 encode-int encode+ h# 00000001 encode-int encode+
   2 encode-int encode+ h# 00700000 encode-int encode+ h# 00001000 encode-int encode+
   2 encode-int encode+ h# 00200000 encode-int encode+ h# 00000004 encode-int encode+
   2 encode-int encode+ h# 00300000 encode-int encode+ h# 0000081c encode-int encode+
   2 encode-int encode+ h# 00000000 encode-int encode+ h# 00010000 encode-int encode+
   2 encode-int encode+ h# 00240000 encode-int encode+ h# 00000004 encode-int encode+
   2 encode-int encode+ h# 00280000 encode-int encode+ h# 00000001 encode-int encode+
   " reg" property
finish-device

" /iommu/sbus" find-device
new-device
  " espdma" device-name
  external
  : encode-unit encode-unit-sbus ;
  : decode-unit decode-unit-sbus ;
finish-device

" /iommu/sbus" find-device
new-device
  " ledma" device-name
  h# 4 encode-int h# 08400010 encode-int encode+ h# 00000020 encode-int encode+ " reg" property
  h# 3f encode-int " burst-sizes" property
  external
  : encode-unit encode-unit-sbus ;
  : decode-unit decode-unit-sbus ;
finish-device

" /iommu/sbus/ledma" find-device
new-device
  " le" device-name
  " network" device-type
  h# 4 encode-int h# 08c00000 encode-int encode+ h# 00000004 encode-int encode+ " reg" property
  h# 7 encode-int " busmaster-regval" property
  h# 26 encode-int 0 encode-int encode+ " intr" property
finish-device

" /iommu/sbus" find-device
new-device
  \ disabled with xxx, bad interactions with Linux
  " xxxpower-management" device-name
  h# 4 encode-int h# 0a000000 encode-int encode+ h# 00000010 encode-int encode+ " reg" property
finish-device


\ obio (on-board IO)
" /" find-device
new-device
  " obio" device-name
  " hierarchical" device-type
  2 encode-int " #address-cells" property
  1 encode-int " #size-cells" property
  h# 0 encode-int h# 0 encode-int encode+ h# 0 encode-int encode+ h# 71000000 encode-int encode+ h# 01000000 encode-int encode+
   " ranges" property
  external
  : open ( cr ." opening obio" cr) true ;
  : close ;
  : encode-unit encode-unit-sbus ;
  : decode-unit decode-unit-sbus ;
finish-device

" /options" find-device
  " disk" encode-string " boot-from" property

