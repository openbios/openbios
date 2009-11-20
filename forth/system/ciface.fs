
0 value ciface-ph

dev /openprom/
new-device
" client-services" device-name

active-package to ciface-ph

\ -------------------------------------------------------------
\ private stuff
\ -------------------------------------------------------------

variable callback-function

: ?phandle ( phandle -- phandle )
  dup 0= if ." NULL phandle" -1 throw then
;
: ?ihandle ( ihandle -- ihandle )
  dup 0= if ." NULL ihandle" -2 throw then
;

\ copy and null terminate return string
: ci-strcpy ( buf buflen str len -- len )
  >r -rot dup
  ( str buf buflen buflen R: len )
  r@ min swap
  ( str buf n buflen R: len )
  over > if
    ( str buf n )
    2dup + 0 swap c!
  then
  move r>
;

0 value memory-ih
0 value mmu-ih

:noname ( -- )
  " /chosen" find-device

  " mmu" active-package get-package-property 0= if
    decode-int nip nip to mmu-ih
  then

  " memory" active-package get-package-property 0= if
    decode-int nip nip to memory-ih
  then
  device-end
; SYSTEM-initializer

: safetype
  ." <" dup cstrlen dup 20 < if type else 2drop ." BAD" then ." >"
;

\ -------------------------------------------------------------
\ public interface
\ -------------------------------------------------------------

external

\ -------------------------------------------------------------
\ 6.3.2.1 Client interface
\ -------------------------------------------------------------

\ returns -1 if missing
: test ( name -- 0|-1 )
  dup cstrlen ciface-ph find-method
  if drop 0 else -1 then
;

\ -------------------------------------------------------------
\ 6.3.2.2 Device tree
\ -------------------------------------------------------------

: peer peer ;
: child child ;
: parent parent ;

: getproplen ( phandle name -- len|-1 )
  \ ." PH " over . dup cstrlen ."  GETPROPLEN " 2dup type cr
  dup cstrlen
  rot ?phandle get-package-property
  if -1 else nip then
;

: getprop ( phandle name buf buflen -- size|-1 )
  \ ." PH " 3 pick . ." GETPROP " 2 pick dup cstrlen type cr 
  >r >r dup cstrlen
  rot
  \ detect phandle == -1 
  dup -1 = if
    r> r> 2drop 3drop -1 exit
  then

  \ return -1 if phandle is 0 (MacOS actually does this)
  ?dup 0= if r> r> 2drop 2drop -1 exit then
  
  ?phandle get-package-property if r> r> 2drop -1 exit then
  r> r>
  ( prop proplen dest destlen )
  rot dup >r min move r>
;

\ 1 OK, 0 no more prop, -1 prev invalid
: nextprop ( phandle prev buf -- 1|0|-1 )
  rot >r
  swap ( buf prev )
  dup 0= if 0 else dup cstrlen then

  ( buf prev prev_len )
  0 3 pick c!
  
  \ verify that prev exists (overkill...)
  dup if
    2dup r@ get-package-property if
      r> 2drop 2drop -1 exit
    else
      2drop
    then
  then
  
  ( buf prev prev_len )

  r> next-property if
    ( buf name name_len )
    dup 1+ -rot ci-strcpy drop 1
  else
    ( buf )
    drop 0
  then
;

: setprop ( phandle name buf len -- size )
  dup >r encode-bytes rot dup cstrlen
  ( phandle buf len name name_len R: size )
  4 pick (property)
  drop r>
;

: finddevice ( dev_spec -- phandle|-1 )
  dup cstrlen
  \ ." FIND-DEVICE " 2dup type
  find-dev 0= if -1 then
  \ ." -- " dup . cr
;

: instance-to-package ( ihandle -- phandle )
  ?ihandle ihandle>phandle
;

: package-to-path ( phandle buf buflen -- length )
  rot
  \ XXX improve error checking
  dup 0= if 3drop -1 exit then
  get-package-path
  ( buf buflen str len )
  ci-strcpy
;

: canon ( dev_specifier buf buflen -- len )
  rot dup cstrlen find-dev if
    ( buf buflen phandle )
    -rot
    package-to-path
  else
    2drop -1
  then
;

: instance-to-path ( ihandle buf buflen -- length )
  rot
  \ XXX improve error checking
  dup 0= if 3drop -1 exit then
  get-instance-path
  \ ." INSTANCE: " 2dup type cr dup .
  ( buf buflen str len )
  ci-strcpy
;

: instance-to-interposed-path ( ihandle buf buflen -- length )
  rot
  \ XXX improve error checking
  dup 0= if 3drop -1 exit then
  get-instance-interposed-path
  ( buf buflen str len )
  ci-strcpy
;

: call-method ( ihandle method -- xxxx catch-result )
  dup 0= if ." call of null method" -1 exit then
  \ ." call-method " 2dup type cr
  rot ?ihandle ['] $call-method catch if
    \ not necessary an error but very useful for debugging...
    ." call-method " r@ dup cstrlen type ." : exception " dup . cr
  then
  \ r> drop
;


\ -------------------------------------------------------------
\ 6.3.2.3 Device I/O
\ -------------------------------------------------------------

: open ( dev_spec -- ihandle|0 )
  dup cstrlen open-dev
;

: close ( ihandle -- )
  close-dev
;

: read ( len addr ihandle -- actual )
  rot swap " read" call-method
;

: write ( len addr ihandle -- actual )
  rot swap " write" call-method
;

: seek ( pos_lo pos_hi ihandle -- status )
  " seek" call-method 
;


\ -------------------------------------------------------------
\ 6.3.2.4 Memory
\ -------------------------------------------------------------

\ : claim ( virt size align -- baseaddr|-1 ) ;
\ : release ( virt size -- ) ;

\ -------------------------------------------------------------
\ 6.3.2.5 Control transfer
\ -------------------------------------------------------------

: boot ( bootspec -- )
  ." BOOT"
;

: enter ( -- )
  ." ENTER"
;

\ exit ( -- ) is defined later (clashes with builtin exit)

: chain ( virt size entry args len -- )
  ." CHAIN"
;

\ -------------------------------------------------------------
\ 6.3.2.6 User interface
\ -------------------------------------------------------------

: interpret ( xxx cmdstring -- ??? catch-reult )
  dup cstrlen
  \ ." INTERPRET: --- " 2dup type
  ['] evaluate catch dup if
    \ this is not necessary an error...
    ." interpret: exception " dup . ." caught" cr
  then
  \ ." --- " cr
;

: set-callback ( newfunc -- oldfunc )
  callback-function @
  swap
  callback-function !
;

\ : set-symbol-lookup ( sym-to-value -- value-to-sym ) ;


\ -------------------------------------------------------------
\ 6.3.2.7 Time
\ -------------------------------------------------------------

\ : milliseconds ( -- ms ) ;


\ -------------------------------------------------------------
\ arch?
\ -------------------------------------------------------------

: start-cpu ( xxx xxx xxx --- )
  ." Start CPU unimplemented" cr
  3drop
;

\ -------------------------------------------------------------
\ special
\ -------------------------------------------------------------

: exit ( -- )
  ." EXIT"
  outer-interpreter
;

[IFDEF] CONFIG_PPC
\ PowerPC Microprocessor CHRP binding
\ 10.5.2. Client Interface

( phandle cstring-method -- missing )

: test-method
	dup cstrlen rot
	find-method 0= if -1 else drop 0 then
;
[THEN]

finish-device
device-end


\ -------------------------------------------------------------
\ entry point
\ -------------------------------------------------------------

: client-iface ( [args] name len -- [args] -1 | [rets] 0 )
  ciface-ph find-method 0= if -1 exit then
  catch ?dup if
    cr ." Unexpected client interface exception: " . -2 cr exit
  then
  0
;

: client-call-iface ( [args] name len -- [args] -1 | [rets] 0 )
  ciface-ph find-method 0= if -1 exit then
  execute
;
