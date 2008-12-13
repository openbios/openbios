\ 7.7 FCode Debugging command group

\ The user interface versions of these FCode functions allow
\ the user to debug FCode programs by providing named commands 
\ corresponding to FCode functions.

: headerless    ( -- )
  ;

: headers    ( -- )
  ;

: begin-package ( arg-str arg-len reg-str reg-len dev-str dev-len -- )
  open-dev dup 0= abort" failed opening parent."
  dup to my-self
  ihandle>phandle active-package!
  new-device
  set-args
;

: end-package    ( -- )
  my-parent >r
  finish-device
  0 active-package!
  0 to my-self
  r> close-dev
;

: apply    ( ... "method-name< >device-specifier< >" -- ??? )
  ;
