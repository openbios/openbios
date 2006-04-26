\ This example tests the capability of tokenizing case..endcase
\ expressions. If the output is "choice 2" everything worked ok.

fcode-version2

2 case
	1 of " choice 1" type endof
	2 of " choice 2" type endof
	3 of " choice 3" type endof
endcase
cr

fcode-end

