\ 7.4.3.5 User commands for booting

: boot    ( "{param-text}<cr>" -- )
  (encode-bootpath)
  s" platform-boot" $find if 
    execute
  else
    2drop
    cr ." Booting " type cr
    ."   ... not supported on this system." cr
  then
;

\ : diagnostic-mode?    ( -- diag? )
\   ;

\ : diag-switch?    ( -- diag? )
\   ;
