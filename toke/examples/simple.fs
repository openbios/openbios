\ this is an fcode test file.

fcode-version2

: gray ( n1 -- n2 )
  dup begin 
    2/ swap over xor swap 
  while repeat  
  ;
  
: macrotest 
  dup dup >> 1+ 1+ accept 
  ;
  
\ another comment.

fcode-end

