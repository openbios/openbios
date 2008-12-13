\ 7.5 Firmware Debugging command group


\ 7.5.1    Automatic stack display

: (.s
  depth 0 ?do
    depth i - 1- pick .
  loop
  depth 0<> if ascii < emit space then
  ;

: showstack    ( -- )
  ['] (.s to status
  ;
  
: noshowstack    ( -- )
  ['] noop to status
  ;

\ 7.5.2    Serial download

: dl    ( -- )
  ;

  
\ 7.5.3    Dictionary

\ 7.5.3.1    Dictionary search
: .calls    ( xt -- )
  ;
  
: $sift    ( text-addr text-len -- )
  ;
  
: sifting    ( "text< >" -- )
  ;

\ : words    ( -- )
\   \ Implemented in forth bootstrap.
\   ;

  
\ 7.5.3.2    Decompiler

\ implemented in see.fs

\ : see    ( "old-name< >" -- )
\   ;
  
\ : (see)    ( xt -- )
\   ;

  
\ 7.5.3.3    Patch

: patch ( "new-name< >old-name< >word-to-patch< >" -- )
  ;

: (patch)    ( new-n1 num1? old-n2 num2? xt -- )
  ;


\ 7.5.3.4    Forth source-level debugger

: debug    ( "old-name< >" -- )
  ;
  
: (debug    ( xt -- )
  ;
  
: stepping    ( -- )
  ;
  
: tracing    ( -- )
  ;
  
: debug-off    ( -- )
  ;
  
: resume    ( -- )
  ;
