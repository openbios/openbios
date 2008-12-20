:noname 
  ."   Type 'help' for detailed information" cr
  \ ."   boot secondary slave cdrom: " cr
  \ ."    0 >  boot hd:2,\boot\vmlinuz root=/dev/hda2" cr
  ; DIAG-initializer

" /" find-device

new-device
  " memory" device-name
  \ 12230 encode-int " reg" property
  external
  : open true ;
  : close ;
  \ claim ( phys size align -- base )
  \ release ( phys size -- )
finish-device

new-device
  " cpus" device-name
  1 " #address-cells" int-property
  0 " #size-cells" int-property

  external
  : open true ;
  : close ;
  : decode-unit parse-hex ;

finish-device

: make-openable ( path )
  find-dev if
    begin ?dup while
      \ install trivial open and close methods
      dup active-package! is-open
      parent
    repeat
  then
;

: preopen ( chosen-str node-path )
  2dup make-openable
    
  " /chosen" find-device
  open-dev ?dup if
    encode-int 2swap property
  else
    2drop
  then
  device-end
;
 
:noname
  set-defaults
; SYSTEM-initializer

\ preopen device nodes (and store the ihandles under /chosen)
:noname
  " memory" " /memory" preopen
  " mmu" " /cpus/@0" preopen
  " stdout" " /builtin/console" preopen
  " stdin" " /builtin/console" preopen

; SYSTEM-initializer

\ use the tty interface if available
:noname
  " /builtin/console" find-dev if drop
    " /builtin/console" " input-device" $setenv
    " /builtin/console" " output-device" $setenv
  then
; SYSTEM-initializer
	    
:noname
  " keyboard" input
; CONSOLE-IN-initializer
 

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
