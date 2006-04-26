\ Common code for SCSI host adapter drivers

\ The following code is intended to be independent of the details of the
\ SCSI hardware implementation.  It is loaded after the hardware-dependent
\ file that defines execute-command, set-address, open-hardware, etc.

headers

-1 value inq-buf                  \ Address of inquiry data buffer
-1 value sense-buf                \ Holds extended error information

0 value #retries  ( -- n )        \ number of times to retry SCSI transaction

\ Classifies the sense condition as either okay (0), retryable (1),
\ or non-retryable (-1)

: classify-sense  ( -- 0 | 1 | -1 )
   sense-buf

   \ Make sure we understand the error class code.
   dup c@  h# 7f and h# 70 <>  if  drop -1 exit  then

   \ Check for filemark, end-of-media, or illegal block length.
   dup 2+ c@  h# e0  and  if  drop -1 exit  then

   2 + c@  h# f and   ( sense-key )

   \ no_sense(0) and recoverable(1) are okay.
   dup 1 <=  if  drop 0 exit  then   ( sense-key )

   \ not-ready(2) and attention(6) are retryable.
   dup 2 =  swap 6 =  or  if  1  else  -1  then
;

0 value open-count

external

\ The SCSI device node defines an address space for its children. That
\ address space is of the form "target#,unit#".  target# and unit# are
\ both integers.  parse-2int converts a text string (e.g., "3,4") into
\ a pair of binary integers.

: decode-unit  ( addr len -- unit# target# )  parse-2int  ;

: open  ( -- okay? )
   open-count  if
      reopen-hardware  dup  if  open-count 1+ to open-count  then
      exit
   else
      open-hardware  dup  if
         1 to open-count
         100 dma-alloc to sense-buf
         100 dma-alloc to inq-buf
      then
   then
;
: close  ( -- )
   open-count 1- to open-count
   open-count  if
      reclose-hardware
   else
      close-hardware
      inq-buf   100 dma-free
      sense-buf 100 dma-free
   then
;


headers

create sense-cmd  3 c, 0 c, 0 c, 0 c, ff c, 0 c,

: get-sense  ( -- )     \ Issue REQUEST SENSE, which is not supposed to fail.
   sense-buf ff  true  sense-cmd 6  execute-command   0=  if  drop  then
;

\ Give the device a little time to recover before retrying the command.
: delay-retry  ( -- )   1000 0 do loop  ;

0 value statbyte \ Local variable used by retry?

\ RETRY? is used by RETRY-COMMAND to determine whether or not to retry the
\ command, considering the following factors:
\  - Success or failure of the command at the hardware level (failure at
\    this level is usually fatal, except in the case of an incoming bus reset)
\  - The value of the status byte returned by the command
\  - The condition indicated by the sense bytes
\  - The number of previous retries
\
\ The input arguments are as returned by "scsi-exec".
\ On output, the top of the stack is true if the command is to be retried,
\ otherwise the top of the stack is false and the results that should be
\ returned by retry-command are underneath it; those results indicate the type
\ of error that occurred.

: retry?  ( hw-result | statbyte 0 -- true | [[sensebuf] f-hw] error? false )
   case
      0  of  to statbyte  endof \ No hardware error; continue checking.
      1  of  true exit    endof \ Retry after incoming bus reset.
      ( hw-result )  true false  exit   \ Other hardware errors are fatal.
   endcase

   statbyte 0=  if  false false exit  then  \ If successful, return "no-error".

   statbyte  2 and  if    \ "Check Condition", so get extended status.
      get-sense  classify-sense  case                  ( -1|0|1 )
          \ If the sense information says "no sense", return "no-error".
          0  of  false false exit                      endof

         \ If the error is fatal, return "sense-buf,valid,statbyte".
         -1  of  sense-buf false statbyte false  exit  endof
      endcase

      \ Otherwise, the error was retryable.  However, if we have
      \ have already retried the specified number of times, don't
      \ retry again; instead return sense buffer and status.
      #retries 0=  if  sense-buf false statbyte false  exit  then
   then

   \ Don't retry if vendor-unique, reserved, intermediate, or
   \ "condition met/good" bits are set. Return "no-sense,status".
   statbyte h# f5 and  if  true statbyte false  exit  then

   \ Don't retry if we have already retried the specified number
   \ of times.  Return "no-sense,status".
   #retries 0=  if  true statbyte false  exit  then
   \ Otherwise, it was either a busy or a retryable check condition,
   \ so we retry.
   true
;

\ RETRY-COMMAND executes a SCSI command.  If a check condition is indicated,
\ performs a "get-sense" command.  If the sense bytes indicate a non-fatal
\ condition (e.g., power-on reset occurred, not ready yet, or recoverable
\ error), the command is retried until the condition either goes away or
\ changes to a fatal error.
\
\ The command is retried until
\ a) The command succeeds, or
\ b) The select fails, or dma fails, or
\ c) The sense bytes indicate an error that we can't retry at this level, or
\ d) The number of retries is exceeded.
\ #retries is number of times to retry (0: don't retry, -1: retry forever)
\
\ sensebuf is the address of the sense buffer; it is present only
\ if f-hw is 0 and error? is nonzero.  The length of the sense buffer
\ is 8 bytes plus the value in byte 7 of the sense buffer.
\
\ f-hw is nonzero if there is a hardware error -- dma fails, select fails,
\ etc. -- or if the status byte was neither 0 (okay) nor 2 (check condition).
\
\ error? is nonzero if there is a transaction error.  If error? is 0,
\ f-hw and sensebuf are not returned.
\
\ If sensebuf is returned, the contents are valid until the next call to
\ retry-command.  sensebuf becomes inaccessable when this package is closed.
\
\ dma-dir is necessary because it is not always possible to infer the DMA
\ direction from the command.

\ Local variables used by retry-command?

0 instance value dbuf             \ Data transfer buffer
0 instance value dlen             \ Expected length of data transfer
0 instance value direction-in     \ Direction for data transfer

-1 instance value cbuf            \ Command base address
 0 instance value clen            \ Actual length of this command

external

: retry-command  ( dma-buf dma-len dma-dir cmdbuf cmdlen #retries -- ... )
                 ( ... -- [[sensebuf] f-hw] error? )
   to #retries   to clen  to cbuf  to direction-in  to dlen  to dbuf

   begin
      dbuf dlen  direction-in  cbuf clen  execute-command  ( hwerr | stat 0 )
      retry?
   while
      #retries 1- to #retries
      delay-retry
   repeat
;

headers

\ Collapses the complete error information returned by retry-command into
\ a single error/no-error flag.

: error?  ( false | true true | sensebuf false true -- error? )
   dup  if  swap 0=  if  nip  then  then
;

external

\ Simplified "retry-command" routine for commands with no data transfer phase
\ and simple error checking requirements.

: no-data-command  ( cmdbuf -- error? )
   >r  0 0 true  r> 6  -1  retry-command error?
;

\ short-data-command executes a command with the following characteristics:
\  a) The data direction is incoming
\  b) The data length is less than 256 bytes

\ The host adapter driver is responsible for supplying the DMA data
\ buffer; if the command succeeds, the buffer address is returned.
\ The buffer contents become invalid when another SCSI command is
\ executed, or when the driver is closed.

: short-data-command  ( data-len cmdbuf cmdlen -- true | buffer false )
   >r >r  inq-buf swap  true  r> r> -1  retry-command   ( retry-cmd-results )
   error?  dup 0=  if  inq-buf swap  then
;

headers

\ Here begins the implementation of "show-children", a word that
\ is intended to be executed interactively, showing the user the
\ devices that are attached to the SCSI bus.

\ Tool for storing a big-endian 24-bit number at an unaligned address
: 3c!  ( n addr -- )  >r lbsplit drop  r@ c!  r@ 1+ c!  r> 2+ c!  ;


\ Command block template for Inquiry command

create inquiry-cmd  h# 12 c, 0 c, 0 c, 0 c, ff c, 0 c,

: inquiry  ( -- error? )
   \ 8 retries should be more than enough; inquiry commands aren't
   \ supposed to respond with "check condition".

   inq-buf ff  true  inquiry-cmd 6  8  retry-command  error?
;

\ Returns true if the target number "select-id" responds to the inquiry
\ command.
: probe-target  ( select-id -- present? )
   0 swap set-address   inquiry 0=
;


\ Reads the indicated byte from the Inquiry data buffer.

: inq@  ( offset -- value )  inq-buf +  c@  ;

: .scsi1-inquiry  ( -- )  inq-buf 5 ca+  4 inq@  fa min  type  ;
: .scsi2-inquiry  ( -- )  inq-buf 8 ca+  d# 28 type    ;

\ Displays the results of an Inquiry command to the indicated device.

: show-lun  ( unit target -- )
   over swap  set-address                         ( unit )
   inquiry  if  drop exit  then                   ( unit )
   0 inq@  h# 7f  =  if  drop exit  then          ( unit )
   ."   Unit " . ."   "                           ( )
   1 inq@  h# 80 and  if  ." Removable "  then    ( )

   0 inq@  case                                   ( )
      0 of  ." Disk "              endof
      1 of  ." Tape "              endof
      2 of  ." Printer "           endof
      3 of  ." Processor "         endof
      4 of  ." WORM "              endof
      5 of  ." Read Only device"   endof
      ( default ) ." Device type " dup .h
   endcase                                        ( )

   1 inq@  h# 7f and  ?dup  if  ."  Qualifier  " .h  then

   4 spaces
   3 inq@ 0f and  2 =  if  .scsi2-inquiry  else  .scsi1-inquiry  then
   cr
;


external

\ Searches for devices on the SCSI bus, displaying the Inquiry information
\ for each device that responds.

: show-children  ( -- )
   open  0=  if  ." Can't open SCSI host adapter" cr  exit  then

   8 0  do
      i probe-target  if
         ." Target " i . cr
         8 0  do  i j show-lun  loop
      then
   loop

   close
;

headers
\ The Diagnose command is useful for generic SCSI devices.
\ It executes both "test-unit-ready" and "send-diagnostic"
\ commands, decoding the error status information they return.

create test-unit-rdy-cmd        0 c, 0 c, 0 c, 0 c, 0 c, 0 c,
create send-diagnostic-cmd  h# 1d c, 4 c, 0 c, 0 c, 0 c, 0 c,

: send-diagnostic ( -- error? )  send-diagnostic-cmd  no-data-command  ;

external

: diagnose  ( -- error? )
   0 0 true  test-unit-rdy-cmd 6   -1   ( dma$ dir cmd$ #retries )
   retry-command  if                    ( [ sensebuf ] hardware-error? )
      ." Test unit ready failed - "     ( [ sensebuf ] hardware-error? )

      if                                ( )
         ." hardware error (no such device?)" cr          ( )
      else                              ( sensebuf )
         ." extended status = " cr      ( sensebuf )
         base @ >r hex                  ( sensebuf )
         8 bounds ?do  i 3 u.r  loop cr ( )
	 r> base !
      then
      true
   else
      send-diagnostic  ( fail? )
   then
;

headers

