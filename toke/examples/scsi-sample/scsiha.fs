\ Example FCode driver for a hypothetical SCSI bus interface device

hex

\ The following structure defines the registers for the SCSI device.
\ This hypothetical device is designed for ease of programming.  It
\ has a separate register for each function (no bit packing).  All
\ registers are both readable and writeable.  The device has a random-
\ access buffer large enough for a maximum-length SCSI command block.

\ To execute a SCSI command with this device, write the appropriate
\ information into the registers named ">cmd-adr" through ">input?", write
\ a 1 to the ">start" register, and wait for the ">start" register to
\ change to 0.  Then read the ">phase" register to determine whether or
\ not the command completed all phases (">phase" reports 0 on success,
\ h# fd for incoming reset, h# ff for other hardware error).
\ If so, ">status" contains the SCSI status byte, and ">message-in"
\ contains the command-complete message byte.

struct  ( scsi-registers )
  0c field >cmd-adr               \ Up to 12 command bytes
   4 field >cmd-len               \ Length of command block

   4 field >data-adr              \ Base address of DMA data area
   4 field >data-len              \ Length of data area

   1 field >host-selectid         \ Host's selection ID
   1 field >target-selectid       \ Target's selection ID
   1 field >input?                \ 1 for data output; 0 for data input
   1 field >message-out           \ Outgoing message byte

   1 field >start                 \ Write 1 to start.  Reads as 0 when done.
   1 field >phase                 \ Reports the last transaction phase
   1 field >status                \ Returned status byte
   1 field >message-in            \ Incoming message byte

   1 field >intena                \ Write 1 to enable interrupts.
   1 field >reset-bus             \ Write 1 to reset the SCSI bus.
   1 field >reset-board           \ Write 1 to reset the board.
constant /scsi-regs


\ Now that we have a symbolic name for the size of the register block,
\ we can declare the "reg" property.

\ Registers begin at offset 800000 and continue for "/scsi-regs" bytes.

my-address 80.0000 +  my-space  /scsi-regs  reg


-1 instance value regs            \ Virtual base address of device registers

0 instance value my-id            \ host adapter's selection ID
0 instance value his-id           \ target's selection ID
0 instance value his-lun          \ target's unit number

\ Map device registers

: map  ( -- )
   my-address 80.0000 +  my-space  /scsi-regs  ( addr-low addr-high size )
   " map-in" $call-parent   to regs            ( )
;

: unmap  ( -- )
   regs /scsi-regs  " map-out" $call-parent  -1 to regs
;

create reset-done-time 0 ,
create resetting false ,

\ 5 seconds appears to be about the right length of time to wait after
\ a reset, considering a variety of disparate devices.
d# 5000 value scsi-reset-delay

: reset-wait  ( -- )
   resetting @  if
      begin  get-msecs reset-done-time @ -  0>=  until
      resetting off
   then
;

: reset-scsi-bus  ( -- )
   1 regs >reset-board rb!        \ Reset the controller board.
   0 regs >intena      rb!        \ Turn off interrupts.
   1 regs >reset-bus   rb!        \ Reset the SCSI bus.

   \ After resetting the SCSI bus, we have to give the target devices
   \ some time to initialize their microcode.  Otherwise the first command
   \ may hang, as with some older controllers.  We note the time when it
   \ is okay to access the bus (now plus some delay), and "execute-command"
   \ will delay until that time is reached, if necessary.
   \ This allows us to overlap the delay with other work in many cases.
   get-msecs scsi-reset-delay + reset-done-time !  resetting on
;

0 value scsi-time       \ Maximum command time in milliseconds
0 value time-limit      \ Ending time for command

: set-timeout  ( msecs -- )   to scsi-time ;

0 value devaddr

\ Returns true if select failed
: (exec)  ( dma-adr,len dir cmd-adr,len -- hwresult )
   reset-wait           \ Delay until any prior reset operation is done.

   his-lun h# 80 or  regs >message-out rb! \ Set unit number; no disconnect.
   my-id      regs >host-selectid   rb!    \ Set the selection IDs.
   his-id     regs >target-selectid rb!

   \ Write the command block into the host adapter's command register
   dup 0  ?do                       ( data-adr,len dir cmd-adr,len )
      over i + c@                   ( data-adr,len dir cmd-adr,len cmd-byte )
      regs >cmd-adr i ca+ rb!       ( data-adr,len dir cmd-adr,len )
   loop                             ( data-adr,len dir cmd-adr,len )

   regs >cmd-len rl!   drop         ( data-adr,len dir )

   \ Set the data transfer parameters.

   ( .. dir ) regs >input?   rb!    ( data-adr,len )  \ Direction
   ( .. len ) regs >data-len rl!    ( data-adr )      \ Length
   ( .. adr ) regs >data-adr rl!    ( )               \ DMA Address

   \ Now we're ready to execute the command.

   1  regs >start  rb!                    \ Tell board to start the command.

   get-msecs scsi-time +  to time-limit   \ Set the time limit.

   begin  regs >start rb@  while          \ Wait until command finished.
      scsi-time  if                       \ If timeout is enabled, and
         get-msecs time-limit -  0>=  if  \ the time-limit has been reached,
            reset-scsi-bus  true  exit    \ reset the bus and return error.
         then
      then

   repeat

   \ Nonzero phase means that the command didn't finish.

   regs >phase rb@
;


\ Returns true if select failed
: execute-command ( data-adr,len dir cmd-adr,len -- hwresult | statbyte false)
   \ Temporarily put dir and cmd-adr,len on the return stack to get them
   \ out of the way so we can work on the DMA data buffer.

   >r >r >r                     ( data-adr,len )

   dup  if                      ( data-adr,len )

      \ If the data transfer has a nonzero length, we have to map it in.

      2dup  false  dma-map-in   ( data-adr,len dma )
      2dup swap  r> r> r>       ( data-adr,len dma dma,len dir cmd-adr,len)

      (exec)                    ( data-adr,len phys hwres)

      >r swap dma-map-out  r>   ( hwresult )
   else                         ( data-adr,len )
      r> r> r>  (exec)          ( hwresult )
   then                         ( hwresult )

   ?dup  0=  if                                    ( hwresult | )
      regs >status rb@  false \ Command finished; return status byte and false.
   then                                            ( hwresult | statbyte 0 )
;

external

: reset  ( -- )  map  reset-scsi-bus  unmap  ;
reset   \ Reset the SCSI bus when we are probed.

: open-hardware  ( -- okay? )
   map
   7 to my-id
   \ Should perform a quick "sanity check" selftest here,
   \ returning true if the test succeeds.
   true
;
: reopen-hardware  ( -- okay? )  true  ;

: close-hardware  ( -- )  unmap  ;
: reclose-hardware  ( -- )  ;

: selftest  ( -- 0 | error-code )
   \ Perform reasonably extensive selftest here, displaying
   \ a message and returning an error code if the
   \ test fails and returning 0 if the test succeeds.
   0
;
: set-address  ( unit target -- )
   to his-id  to his-lun
;



