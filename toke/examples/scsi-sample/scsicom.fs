\ This file contains some words that are useful for both
\ SCSI disk and SCSI tape device drivers.

\ The SCSI disk and SCSI tape packages need to export dma-alloc and dma-free
\ methods so the deblocker can allocate DMA-capable buffer memory.

external
: dma-alloc  ( n -- vaddr )  " dma-alloc" $call-parent  ;
: dma-free   ( vaddr n -- )  " dma-free"  $call-parent  ;
headers

: parent-max-transfer  ( -- n )  " max-transfer"  $call-parent  ;

\ Calls the parent device's "retry-command" method.  The parent device is
\ assumed to be a driver for a SCSI host adapter (device-type = "scsi").

: retry-command  ( dma-addr dma-len dma-dir cmd-addr cmd-len #retries -- ... )
           ( ... -- false )                \ No error
           ( ... -- true true )            \ Hardware error
           ( ... -- sensebuf false true )  \ Fatal error with extended status
   " retry-command" $call-parent
;

\ Simplified command execution routines for common simple command forms

: no-data-command  ( cmdbuf -- error? )  " no-data-command" $call-parent  ;

: short-data-command  ( data-len cmdbuf cmdlen -- true | buffer false )
   " short-data-command" $call-parent
;

\ Some tools for reading and writing 2-, 3-, and 4-byte numbers to and from
\ SCSI command and data buffers.  The ones defined below are used both in
\ the SCSI disk and the SCSI tape packages.  Other variations that are
\ used only by one of the packages are defined in the package where they
\ are used.

: +c!  ( n addr -- addr' )  tuck c! 1+  ;
: 3c!  ( n addr -- )  >r lbsplit drop  r> +c! +c! c!  ;

: -c@  ( addr -- n addr' )  dup c@  swap 1-  ;
: 3c@  ( addr -- n )  2 +  -c@ -c@  c@       0  bljoin  ;
: 4c@  ( addr -- n )  3 +  -c@ -c@ -c@  c@      bljoin  ;

\ "Scratch" command buffer useful for construction of read and write commands

create cmdbuf  0 c, 0 c, 0 c, 0 c, 0 c, 0 c, 0 c, 0 c, 0 c, 0 c, 0 c, 0 c,
: cb!  ( byte index -- )  cmdbuf + c!  ;        \ Write byte to command buffer

\ The deblocker converts a block/record-oriented interface to a byte-oriented
\ interface, using internal buffering.  Disk and tape devices are usually
\ block- or record-oriented, but the OBP external interface is byte-oriented,
\ in order to be independent of particular device block sizes.

0 instance value deblocker
: init-deblocker  ( -- okay? )
   " "  " deblocker"  $open-package  to deblocker
   deblocker if
      true
   else
      ." Can't open deblocker package"  cr  false
   then
;

headerless
: selftest  ( -- error? )
   my-unit " set-address" $call-parent
   " diagnose" $call-parent
;

headers

