\ SCSI disk package implementing a "block" device-type interface

" sd" encode-string  " name" property
" block"     device-type

fload scsicom.fs    \ Utility routines for SCSI commands

hex

\ 0 means no timeout
: set-timeout  ( msecs -- )  " set-timeout" $call-parent  ;

0 instance value offset-low     \ Offset to start of partition
0 instance value offset-high

0 instance value label-package

\ Sets offset-low and offset-high, reflecting the starting location of the
\ partition specified by the "my-args" string.

: init-label-package  ( -- okay? )
   0 to offset-high  0 to offset-low
   my-args  " disk-label"  $open-package to label-package
   label-package  if
      0 0  " offset" label-package $call-method  to offset-high to offset-low
      true
   else
      ." Can't open disk label package"  cr  false
   then
;

\ Ensures that the disk is spinning, but doesn't wait forever.

create sstart-cmd  h# 1b c, 1 c, 0 c, 0 c, 1 c, 0 c,

: timed-spin  ( -- error? )
   d# 15000 set-timeout
   sstart-cmd  no-data-command
   0 set-timeout
;

0 instance value /block         \ Device native block size

create mode-sense-cmd     h# 1a c, 0 c, 0 c, 0 c, d# 12 c, 0 c,
create read-capacity-cmd  h# 25 c, 0 c, 0 c, 0 c, d# 12 c, 0 c,
                              0 c, 0 c, 0 c, 0 c,

: read-block-size  ( -- n )     \ Ask device about its block size.
   \ First try "mode sense" - data returned in bytes 9,10,11.

   d# 12  mode-sense-cmd 6  short-data-command  if  0  else  9 + 3c@  then

   ?dup  if  exit  then

   \ Failing that, try "read capacity" - data returned in bytes 4,5,6,7.

   8  read-capacity-cmd 0a  short-data-command   if  0  else  4 + 4c@  then

   ?dup  if  exit  then

   d# 512               \ Default to 512 if the device won't tell us.
;

external


\ Return device block size; cache it the first time we find the information.
\ This method is called by the deblocker.
: block-size  ( -- n )
   /block  if  /block exit  then        \ Don't ask if we already know.
   read-block-size dup to /block
;

headers

\ Read or write "#blks" blocks starting at "block#" into memory at "addr"
\ Input? is true for reading or false for writing.
\ Command is  8  for reading or  h# a  for writing.
\ We use the 6-byte forms of the disk read and write commands.

: 2c!  ( n addr -- )  >r lbsplit 2drop  r> +c!         c!  ;
: 4c!  ( n addr -- )  >r lbsplit        r> +c! +c! +c! c!  ;

: r/w-blocks  ( addr block# #blks input? command -- actual# )
   cmdbuf d# 10 erase
   2over h# 100 u>
   swap h# 200000 u>=  or if  \ Use 10-byte form  ( addr block# #blks dir cmd )
      h# 20 or  0 cb!  \ 28 (read) or 2a (write)  ( addr block# #blks dir )
      -rot swap                                   ( addr dir #blks block# )
      cmdbuf 2 + 4c!                              ( addr dir #blks )
      dup cmdbuf 7 + 2c!
      d# 10                                       ( addr dir #blks cmd-len )
   else                        \ Use 6-byte form  ( addr block# #blks dir cmd )
      0 cb!                                       ( addr block# #blks dir )
      -rot swap                                   ( addr dir #blks block# )
      cmdbuf 1+ 3c!                               ( addr dir #blks )
      dup 4 cb!                                   ( addr dir #blks )
      6                                           ( addr dir #blks cmd-len )
   then
   tuck  >r >r                  ( addr input? #blks ) ( R: #blks cmd-len )
   /block *  swap cmdbuf r> -1      ( addr #bytes input? cmd cmd-len #retries )
   retry-command  if                ( [ sensebuf ] hw? )
      0= if  drop  then  r> drop 0
   else                             (  )
      r>
   then                             ( actual# )
;

external

\ These three methods are called by the deblocker.

: max-transfer  ( -- n )   parent-max-transfer  ;
: read-blocks   ( addr block# #blocks -- #read )     true  d# 8  r/w-blocks  ;
: write-blocks  ( addr block# #blocks -- #written )  false d# 10 r/w-blocks  ;

\ Methods used by external clients

: open  ( -- flag )
   my-unit " set-address" $call-parent

   \ It might be a good idea to do an inquiry here to determine the
   \ device configuration, checking the result to see if the device
   \ really is a disk.

   \ Make sure the disk is spinning.

   timed-spin  if  false exit  then

   block-size to /block

   init-deblocker  0=  if  false exit  then

   init-label-package  0=  if
      deblocker close-package false exit
   then

   true
;

: close  ( -- )
   label-package close-package
   deblocker close-package
;

: seek  ( offset.low offset.high -- okay? )
   offset-low offset-high  x+  " seek"   deblocker $call-method
;

: read  ( addr len -- actual-len )  " read"  deblocker $call-method  ;
: write ( addr len -- actual-len )  " write" deblocker $call-method  ;
: load  ( addr -- size )            " load"  label-package $call-method  ;

headers


