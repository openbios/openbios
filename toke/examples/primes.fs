\ primes.4th

fcode-version2

\ Example code for kForth
\ Copyright (c) 1998 Creative Consulting for Research and Education
\
\ This program is free software; you may redistribute it under the terms of
\ the GNU General Public License.  This program has absolutely no warranty.


\ Test for a prime number. Return the largest divisor (< n ) 
\ and a flag indicating whether the number is prime or not.

: ?prime ( n -- m flag | is n a prime number? )
\ if flag is false (0), m is the largest divisor of n
    abs
    dup 3 >		\ is n > 3 ?
    if
      dup 2 /mod
      swap 0= 
      if		\ is n divisible by 2 ?
        nip false
      else
        1-		\ check for divisibility starting	  
        begin		\ with n/2 - 1 and counting down
          2dup mod
          over 1 >
          and
        while
          1-
        repeat
        nip
        dup 1 <=
      then 
    else
      drop 1 true 
    then
;

: test_prime ( n -- | test for prime number and display result )
    ?prime
    if
      ." is a prime number" drop
    else
      ." is NOT prime. Its largest divisor is " .
    then
    cr
;

: list_primes ( n -- | list all the prime numbers from 1 to n )
    abs
    dup 0>
    if 
      1+ 1 do
        i ?prime 
	if i . cr then 
	drop
      loop
    else
      drop
    then
;
 
10000 list_primes

fcode-end

