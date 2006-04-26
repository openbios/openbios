
dev /pci

[IFDEF] CONFIG_DRIVER_IDE

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

: ?devalias ( alias-str alias-len device-str device-len -- 
  \		alias-str alias-len false | true )
  active-package >r
  " /aliases" find-device
  \ 2dup ." Checking " type
  2dup find-dev if     \ check if device exists
    drop
    2over find-dev if  \ do we already have an alias?
      \ ." alias exists" cr
      drop 2drop false
    else
      \ ." device exists" cr
      encode-string
      2swap property
      true
    then
  else
    \ ." device doesn't exist" cr
    2drop false
  then
  r> active-package!
  ;

:noname 
  " hd"
  " /pci/isa/ide0/disk@0" ?devalias not if
    " /pci/isa/ide0/disk@1" ?devalias not if
      " /pci/isa/ide1/disk@0" ?devalias not if
        " /pci/isa/ide1/disk@1" ?devalias not if
	  2drop ." No disk found." cr
	then
      then
    then
  then
 
  " cdrom"
  " /pci/isa/ide0/cdrom@0" ?devalias not if
    " /pci/isa/ide0/cdrom@1" ?devalias not if
      " /pci/isa/ide1/cdrom@0" ?devalias not if
        " /pci/isa/ide1/cdrom@1" ?devalias not if
	  2drop ." No cdrom found" cr
	then
      then
    then
  then
; SYSTEM-initializer


[THEN]

device-end
