\ This file has been created with codegen's ccfcode

fcode-version2
headers

\ C runtime
variable $frame 0 $frame !
 " dev /pci" evaluate
 ( asm ) new-device
 ( asm )
\ function screen_open
: screen_open recursive
 24 $frame @ >r alloc-mem $frame !
 " "(61737369676E65642D616464726573736573)" get-my-property ( asm ) if
 " "(63616E6E6F742066696E642061737369676E65642D6164647265737365732070726F70657274790A)" type
 abort then

 begin decode-int ( asm ) dup $frame @ 8 + ( physhi ) l! drop
 decode-int ( asm ) dup $frame @ 16 + ( physmid ) l! drop
 decode-int ( asm ) dup $frame @ 20 + ( physlo ) l! drop
 decode-int ( asm ) dup $frame @ ( sizehi ) l! drop
 decode-int ( asm ) dup $frame @ 4 + ( sizelo ) l! drop
 dup ( top ) ( asm ) 0 > 1 and 0<> if $frame @ 8 + ( physhi ) l@ 24 >>a 7 and 2 <> 1 and then while repeat
 2drop
 ( asm )
 4 " "(636F6E6669672D6C40)" $call-parent ( asm )
 ( pop ) ( asm )
 24 $frame @ swap free-mem r> $frame ! exit dup $frame @ 12 + ( cfg ) l! drop
 4 $frame @ 12 + ( cfg ) l@ 65535 and 1 or 2 or " "(636F6E6669672D6C21)" $call-parent ( asm )
 $frame @ 8 + ( physhi ) l@ $frame @ 16 + ( physmid ) l@ $frame @ 20 + ( physlo ) l@ 480 640 * " "(6D61702D696E)" $call-parent ( asm )
 ( pop ) ( asm )
 24 $frame @ swap free-mem r> $frame ! exit to frame-buffer-adr ( asm )
 default-font ( asm )
 set-font ( asm )
 640 480 80 25 fb8-install ( asm )
 24 $frame @ swap free-mem r> $frame !
 ; \ end screen_open

\ function screen_close
: screen_close recursive
 4 $frame @ >r alloc-mem $frame !
 4 " "(636F6E6669672D6C40)" $call-parent ( asm )
 ( pop ) ( asm )
 4 $frame @ swap free-mem r> $frame ! exit dup $frame @ ( cfg ) l! drop
 4 $frame @ ( cfg ) l@ 65528 and " "(636F6E6669672D6C21)" $call-parent ( asm )
 frame-buffer-adr ( asm ) 480 640 * " "(6D61702D6F7574)" $call-parent ( asm )
 4 $frame @ swap free-mem r> $frame !
 ; \ end screen_close

\ function screen_selftest
: screen_selftest recursive
 " "(73637265656E2073656C66746573740A)" type
 0 exit
 ; \ end screen_selftest

\ function main
: main recursive
 " "(73637265656E)" device-name ( asm )
 " "(646973706C6179)" device-type ( asm )
 " "(6D616B652D70726F70657274696573)" evaluate
 " "()" " "(69736F363432392D313938332D636F6C6F7273)" property
 0 1 0 8 reg
 ['] screen_open is-install ( asm )
 ['] screen_close is-remove ( asm )
 ['] screen_selftest is-selftest ( asm )
 finish-device
 ( asm )
 ; \ end main

main

fcode-end
