\ 7.6 Client Program Debugging command group


\ 7.6.1    Registers display

: ctrace    ( -- )
  ;
  
: .registers    ( -- )
  ;

: .fregisters    ( -- )
  ;

\ to    ( param [old-name< >] -- )


\ 7.6.2    Program download and execute

variable file-size

: !load-size file-size ! ;

: load-size file-size @ ;

: load    ( "{params}<cr>" -- )
  linefeed parse ( str len )
  open-dev ( ihandle )
  dup 0= if
    drop
    exit
  then
  dup >r
  " load-base" evaluate swap ( load-base ihandle )
  dup ihandle>phandle " load" rot find-method ( xt 0|1 )
  if swap call-package !load-size else cr ." Cannot find load for this package" 2drop then
  r> close-dev
  ;

: go    ( -- )
  ." go is not yet implemented"
  ;

: state-valid    ( -- a-addr )
  ;

: init-program    ( -- )
  ;


\ 7.6.3    Abort and resume

\ already defined !?
\ : go    ( -- )
\   ;

  
\ 7.6.4    Disassembler

: dis    ( addr -- )
  ;
  
: +dis    ( -- )
  ;

\ 7.6.5    Breakpoints
: .bp    ( -- )
  ;

: +bp    ( addr -- )
  ;

: -bp    ( addr -- )
  ;

: --bp    ( -- )
  ;

: bpoff    ( -- )
  ;

: step    ( -- )
  ;

: steps    ( n -- )
  ;

: hop    ( -- )
  ;

: hops    ( n -- )
  ;

\ already defined
\ : go    ( -- )
\   ;

: gos    ( n -- )
  ;

: till    ( addr -- )
  ;

: return    ( -- )
  ;

: .breakpoint    ( -- )
  ;

: .step    ( -- )
  ;

: .instruction    ( -- )
  ;


\ 7.6.6    Symbolic debugging
: .adr    ( addr -- )
  ;

: sym    ( "name< >" -- n )
  ;

: sym>value    ( addr len -- addr len false | n true )
  ;

: value>sym    ( n1 -- n1 false | n2 addr len true )
  ;
