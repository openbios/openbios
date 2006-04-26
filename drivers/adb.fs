[IFDEF] CONFIG_DRIVER_ADB

dev /pci
\ simple pci bus node
new-device
" via-cuda" device-name
" via-cuda" encode-string " device_type" property
" cuda" encode-string " compatible" property

external
: open ( ." opening CUDA" cr ) true ;
: close ;
: decode-unit 0 decode-unit-pci-bus ;
: encode-unit encode-unit-pci ;
finish-device

dev /pci/via-cuda

new-device
" adb" device-name
" adb" encode-string " device_type" property
0 encode-int " #size-cells" property
1 encode-int " #address-cells" property

external
: open ( ." opening ADB" cr ) true ;
: close ;
finish-device

device-end

[THEN]

