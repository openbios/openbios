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

variable file-type

0 constant elf
1 constant bootinfo
2 constant xcoff
3 constant pe

\ Array indexes and values for e_type

d# 16 constant EI_NIDENT

0 constant EI_MAG0
  h# 7f constant ELFMAG0

1 constant EI_MAG1
  [CHAR] E constant ELFMAG1

2 constant EI_MAG2
  [CHAR] L constant ELFMAG2

3 constant EI_MAG3
  [CHAR] F constant ELFMAG3

4 constant EI_CLASS
  0 constant ELFCLASSNONE
  1 constant ELFCLASS32
  2 constant ELFCLASS64

5 constant EI_DATA
  0 constant ELFDATANONE
  1 constant ELFDATA2LSB
  2 constant ELFDATA2MSB

6 constant EI_VERSION
  0 constant EV_NONE
  1 constant EV_CURRENT

\ Values for e_type

0 constant ET_NONE
1 constant ET_REL
2 constant ET_EXEC
3 constant ET_DYN
4 constant ET_CORE

\ Values for e_machine

d# 2 constant EM_SPARC
d# 3 constant EM_386
d# 6 constant EM_486
d# 18 constant EM_SPARC32PLUS
d# 20 constant EM_PPC
d# 43 constant EM_SPARCV9

/l constant Elf32_Addr
/w constant Elf32_Half
/l constant Elf32_Off
/l constant Elf32_Sword
/l constant Elf32_Word
/l constant Elf32_Size

struct ( ELF header )
  EI_NIDENT  field >Elf32_Ehdr.e_ident     ( File identification )
  Elf32_Half field >Elf32_Ehdr.e_type      ( File type )
  Elf32_Half field >Elf32_Ehdr.e_machine   ( Machine archicture )
  Elf32_Word field >Elf32_Ehdr.e_version   ( ELF format version )
  Elf32_Addr field >Elf32_Ehdr.e_entry     ( Entry point )
  Elf32_Off  field >Elf32_Ehdr.e_phoff     ( Program header file offset )
  Elf32_Off  field >Elf32_Ehdr.e_shoff     ( Section header file offset )
  Elf32_Word field >Elf32_Ehdr.e_flags     ( Architecture specific flags )
  Elf32_Half field >Elf32_Ehdr.e_ehsize    ( Size of ELF header in bytes )
  Elf32_Half field >Elf32_Ehdr.e_phentsize ( Size of program header entries )
  Elf32_Half field >Elf32_Ehdr.e_phnum     ( Number of program header entry )
  Elf32_Half field >Elf32_Ehdr.e_shentsize ( Size of section header entry )
  Elf32_Half field >Elf32_Ehdr.e_shnum     ( Number of section header entries )
  Elf32_Half field >Elf32_Ehdr.e_shstrndx  ( Section name strings section )
constant /Elf32_Ehdr

: @e_ident ( base index -- byte )
    swap >Elf32_Ehdr.e_ident + c@
;

: @e_type ( base -- type )
  >Elf32_Ehdr.e_type w@
;

: @e_machine ( base -- type )
  >Elf32_Ehdr.e_machine w@
;

: @e_entry ( base -- entry )
  >Elf32_Ehdr.e_entry l@
;

: @e_phoff ( base -- poffset )
  >Elf32_Ehdr.e_phoff l@
;

: @e_phnum ( base -- pnum )
  >Elf32_Ehdr.e_phnum w@
;

: elf?
  " load-base" evaluate
  dup EI_MAG0 @e_ident
  ELFMAG0 <> if drop false exit then
  dup EI_MAG1 @e_ident
  ELFMAG1 <> if drop false exit then
  dup EI_MAG2 @e_ident
  ELFMAG2 <> if drop false exit then
  dup EI_MAG3 @e_ident
  ELFMAG3 <> if drop false exit then
  dup EI_CLASS @e_ident
[IFDEF] CONFIG_SPARC64
  ELFCLASS64 <> if drop false exit then
[ELSE]
  ELFCLASS32 <> if drop false exit then
[THEN]
  dup EI_DATA @e_ident
  " little-endian?" evaluate if
    ELFDATA2LSB <> if drop false exit then
  else
    ELFDATA2MSB <> if drop false exit then
  then
  dup @e_type
  ET_EXEC <> if drop false exit then ( not executable )
  @e_machine
[IFDEF] CONFIG_PPC
  EM_PPC  <> if false exit then
[THEN]
[IFDEF] CONFIG_X86
  dup
  EM_386 <> if
    EM_486 <> if
     false exit
    then
  else
    drop
  then
[THEN]
[IFDEF] CONFIG_SPARC32
  dup
  EM_SPARC  <> if
    EM_SPARC32PLUS <> if
     false exit
    then
  else
    drop
  then
[THEN]
[IFDEF] CONFIG_SPARC64
  EM_SPARCV9  <> if false exit then
[THEN]
  true
  ;

variable elf-entry
variable xcoff-entry
variable bootinfo-entry
variable bootinfo-size

: init-program-elf
  elf file-type !
  " /packages/elf-loader" open-dev dup if
    dup
    " init-program" rot $call-method
    close-dev
  else
    drop
    ." /packages/elf-loader is missing" cr
  then
;

: xcoff?
  " load-base" evaluate w@
  h# 1df <> if
    false
    exit
  then
  true
  ;

: init-program-xcoff
  xcoff file-type !
  " /packages/xcoff-loader" open-dev dup if
    dup
    " init-program" rot $call-method
    close-dev
  else
    drop
    ." /packages/xcoff-loader is missing" cr
  then
  ;

: pe?
  false
;

: init-program-pe
  pe file-type !
  " /packages/pe-loader" open-dev dup if
    dup
    " init-program" rot $call-method
    close-dev
  else
    drop
    ." /packages/pe-loader is missing" cr
  then
  ;

: bootinfo?
  " load-base" evaluate dup
  " <CHRP-BOOT>" comp 0= if
    drop
    true
    exit
  then
  " <chrp-boot>" comp 0= if
    true
    exit
  then
  false
  ;

: init-program-bootinfo
  bootinfo file-type !
  " /packages/bootinfo-loader" open-dev dup if
    dup
    " init-program" rot $call-method
    close-dev
  else
    drop
    ." /packages/bootinfo-loader is missing" cr
  then
  ;

: init-program    ( -- )
  elf? if
    init-program-elf
    exit
  then
  xcoff? if
    init-program-xcoff
    exit
  then
  pe? if
    init-program-pe
    exit
  then
  bootinfo? if
    init-program-bootinfo
    exit
  then
  ;

: (encode-bootpath) ( "{params}<cr>" -- bootpath-str bootpath-len)
  bl parse 2dup
  " /chosen" (find-dev) if
    " bootpath" rot (property)
  then
  linefeed parse
  " /chosen" (find-dev) if
    " bootargs" rot (property)
  then
;

: load    ( "{params}<cr>" -- )
  (encode-bootpath)
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
  init-program
  ;

: go    ( -- )
  elf file-type @ = if
[IFDEF] CONFIG_PPC
    elf-entry @ " (go)" evaluate
[ELSE]
    ." go is not yet implemented"
[THEN]
  else
    xcoff file-type @ = if
[IFDEF] CONFIG_PPC
      xcoff-entry @ " (go)" evaluate
[ELSE]
      ." go is not yet implemented"
[THEN]
    else
        bootinfo file-type @ = if
[IFDEF] CONFIG_PPC
	bootinfo-entry @ bootinfo-size @ evaluate
[ELSE]
          ." go is not yet implemented"
[THEN]
        else
          ." go is not yet implemented"
        then
    then
  then
  ;

: state-valid    ( -- a-addr )
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
