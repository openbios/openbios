:noname 
  ."   Type 'help' for detailed information" cr
  \ ."   boot secondary slave cdrom: " cr
  \ ."    0 >  boot hd:2,\boot\vmlinuz root=/dev/hda2" cr
  ; DIAG-initializer

" /" find-device
  " SUNW,SparcStation-5" encode-string " name" property
  " " encode-string " idprom" property
  " SparcStation" encode-string " banner-name" property
  " sun4m" encode-string " compatible" property

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
  " STP1012PGA" device-name
  " cpu" device-type
  " " encode-string " performance-monitor" property
  d# 256 encode-int " mmu-nctx" int-property
  d# 32 encode-int " cache-line-size" int-property
  d# 512 encode-int " cache-nlines" int-property
  1 encode-int " mid" int-property
finish-device

new-device
  " iommu" device-name
finish-device

" /iommu" find-device
new-device
  " sbus" device-name
  " hierarchical" device-type
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
  

