\ 7.4.3.5 User commands for booting

: boot		( "{param-text}<cr>" -- )
  (encode-bootpath)	\ Setup bootpath/bootargs
  s" platform-boot" $find if 
    execute		\ Execute platform-specific boot code
  then
  $load			\ load and go
  go
;


\ : diagnostic-mode?    ( -- diag? )
\   ;

\ : diag-switch?    ( -- diag? )
\   ;
