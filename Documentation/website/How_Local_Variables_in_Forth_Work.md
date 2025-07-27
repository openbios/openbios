# How Local Variables in Forth Work - Using Apple's Open Firmware Implementation

## How to try out these code examples

The following example code may be entered and ran in Apple's Open
Firmware. In order to access Open Firmware, just hold down
Command-Option-O-F while you boot your pre-Intel Mac. PowerPC iMacs and
PowerBooks, iBooks, PowerMac G3's, G4's, and G5's all have Open Firmware
built-in.


## Declaring local variables:

Place curly braces next to the word name. The curly braces must be on
the same line as the word's name. The semicolon is optional. The
semicolon lets the system know that the variables following it will not
be initializing their values to the values that would have been popped
from the stack.

### Example 1


    : myword { ; cat dog }
    ;


Omitting the semicolon will initialize the local variable's values to
values popped from the stack. Make sure the stack has enough values in
it before proceeding.


## Setting local variables:
The word "-\>" (hyphen greater-than) is used to set the values of local
variables.

### Example 2


    : myword { ; cat dog }
    4 -> cat
    5 -> dog
    ;


In the above example, the numbers are each pushed into the stack first.
Then the -\> word pops a value out of the stack and sets each variable's
value to a popped value. If a value is already in the stack, the
following code would work as well: -\> cat (The 4 has been omitted).


## Setting a local variable's value automatically

When you don't use the semicolon in the local variable declaration, the
variables get their value from the stack. The order is the last variable
declared is set to the last value pushed on the stack.

### Example 3

    stack before myword is called ...
    
    3 \<---- top of stack
    2
    1


    : myword { one two three }
    cr ." one = " one .
    cr ." two = " two .
    cr ." three = " three .
    ;


after myword is called ...

    one = 1
    two = 2
    three = 3


## Obtaining a local variable's value

To push a local variable's value onto the stack, just enter a local
variable's name in a word.

### Example 4


    : myword { ; cat dog }
    4 -> cat
    5 -> dog

    cat  \ cat's value pushed onto stack   ( - cat)
    dog  \ dog's value pushed onto stack   (cat - cat dog )
    +

    cr
    ." Total animals = " .
    cr
    ;
