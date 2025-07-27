# What is Forth?

From the [Forth
FAQ](http://www.faqs.org/faqs/computer-lang/forth-faq/part1/): Forth is
a stack-based, extensible language without type-checking. It is probably
best known for its "reverse Polish" (postfix) arithmetic notation,
familiar to users of Hewlett-Packard calculators: to add two numbers in
Forth, you would type 3 5 + instead of 3+5. The fundamental program unit
in Forth is the "word": a named data item, subroutine, or operator.
Programming in Forth consists of defining new words in terms of existing
ones.

# Why and where is Forth used?

Although invented in 1970, Forth became widely known with the advent of
personal computers, where its high performance and economy of memory
were attractive. These advantages still make Forth popular in embedded
microcontroller systems, in locations ranging from the Space Shuttle to
the bar-code reader used by your Federal Express driver. Forth's
interactive nature streamlines the test and development of new hardware.
Incremental development, a fast program-debug cycle, full interactive
access to any level of the program, and the ability to work at a high
"level of abstraction," all contribute to Forth's reputation for very
high programmer productivity. These, plus the flexibility and
malleability of the language, are the reasons most cited for choosing
Forth for embedded systems. Find more information
[here](http://www.complang.tuwien.ac.at/forth/faq/why-forth).

# FCode

FCode is a Forth dialect compliant to ANS Forth, that is available in
two different forms: source and bytecode. FCode bytecode is the compiled
form of FCode source.

# Why bytecode?

Bytecode is small and efficient. And an evaluator (bytecode virtual
machine) is almost trivial to implement. For example, putting a 1 to the
stack only takes one byte in an FCode bytecode binary. This is
especially valuable on combined system or expansion hardware roms where
the available space is limited.
