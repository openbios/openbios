\ Basic display device driver
\ This version doesn't use the graphics accelerator because of conflicts
\ with the window system's use of same.

fcode-version2

hex

" INTL,cgsix"     name
" INTL,501-xxxx"  model

h# 20.0000 constant dac-offset  h#      10 constant /dac
h# 30.0000 constant fhc-offset  h#      10 constant /fhc
h# 30.1800 constant thc-offset  h#      20 constant /thc
h# 70.0000 constant fbc-offset  h#      10 constant /fbc
h# 70.1000 constant tec-offset  h#      10 constant /tec
h# 80.0000 constant fb-offset   h# 10.0000 constant /frame

: >reg-spec ( offset size -- encoded-reg )
   >r 0 my-address d+ my-space encode-phys 0 encode-int encode+
   r> encode-int encode+
;

0 0 >reg-spec                         \ Configuration space registers
dac-offset /dac >reg-spec    encode+
fhc-offset /fhc >reg-spec    encode+
thc-offset /thc >reg-spec    encode+
fbc-offset /fbc >reg-spec    encode+
tec-offset /tec >reg-spec    encode+
fb-offset  /frame >reg-spec  encode+
" reg" property

fcode-end
