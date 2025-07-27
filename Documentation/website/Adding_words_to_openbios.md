# Adding words to OpenBIOS
How to add “words” to the OpenBIOS Dictionary using the C language.

In this example, we are going to implement a forth “word” in C. There
are two arrays that need to be looked at. One is located in the file
openbios-devel/kernel/bootstrap.c. The other array is located in the
file openbios-devel/kernel/forth.c. The two arrays are used to map a C
function to a forth word.

The array in bootstrap.c is where the name of the forth word would go.
The array in forth.c is where the corresponding C function would go.
When adding to these arrays, make sure to add at the end of the list.
This ensures everything continues to run smoothly.

Here is an example on adding to the dictionary. Say you want to add a
word called dog. You can define the C function like this:

```
static void dog(void)
{
       printk(“bark bark!”);
}
```

Add this function to a .c file. Knowing which file to pick can involve a
lot of trial and error. In this example, we will use the file
openbios-devel/kernel/forth.c. Be sure to include the static keyword
before in the function declaration, or OpenBIOS won’t build
successfully.

Now we add the word dog to the end of the forth word list. Open the file
bootstrap.c, then find the array called wordnames. When adding the word
dog, be sure the name of the word is surrounded in double quotes. Here
is how the array should look:

```
/* the word names are used to generate the prim words in the
     dictionary. This is done by the C written interpreter.
*/

static const char *wordnames[] = {
   "(semis)", "", "(lit)", "", "", "", "", "(do)", "(?do)", "(loop)",
   "(+loop)", "", "", "", "dup", "2dup", "?dup", "over", "2over", "pick", "drop",
   "2drop", "nip", "roll", "rot", "-rot", "swap", "2swap", ">r", "r>",
   "r@", "depth", "depth!", "rdepth", "rdepth!", "+", "-", "*", "u*",
   "mu/mod", "abs", "negate", "max", "min", "lshift", "rshift", ">>a",
   "and", "or", "xor", "invert", "d+", "d-", "m*", "um*", "@", "c@",
   "w@", "l@", "!", "+!", "c!", "w!", "l!", "=", ">", "<", "u>", "u<",
   "sp@", "move", "fill", "(emit)", "(key?)", "(key)", "execute",
   "here", "here!", "dobranch", "do?branch", "unaligned-w@",
   "unaligned-w!", "unaligned-l@", "unaligned-l!", "ioc@", "iow@",
   "iol@", "ioc!", "iow!", "iol!", "i", "j", "call", "sys-debug",
   "$include", "$encode-file", "(debug", "(debug-off)", “dog”     // <------- insert here
};
```

Next we add the C function name to the end of the function list. Open
the file forth.c and add dog (no quotes this time) to the end of the
list. You may want to add a comment to the right indicating the word
this c function corresponds to, but this isn’t necessary.

```C
   sysdebug,           /* sys-debug */
   do_include,         /* $include */
   do_encode_file,             /* $encode-file */
   do_debug_xt,                /* (debug  */
   do_debug_off,               /* (debug-off) */
   dog                 /* dog */                   //<--------  insert here
};
```

Now you should be ready to build OpenBIOS. Go to the shell and set the
current directory to the root level of openbios-devel folder:

    cd openbios-devel

Set the architure you wish to build openbios for (amd64 is used in this
example):

    ./config/scripts/switch-arch amd64

Then start building OpenBIOS:

    make build-verbose

Once these commands are done, you need to be able to execute OpenBIOS.
In this example we will be using the unix program version of OpenBIOS.

We run the program like this:

    ./obj-amd64/openbios-unix ./obj-amd64/openbios-unix.dict

We should then be greeted by the OpenBIOS Banner:

    Welcome to OpenBIOS v1.0 built on Mar 9 2010 21:06
      Type 'help' for detailed information

    [unix] Booting default not supported.

    0 > 

Type dog at the prompt and see what it says:

    0 > dog 
    bark bark! ok
    0 > 

Congratulations, you have just taken your first step to improving
OpenBIOS.

## Advanced example:

Suppose you want to make a forth word that would give you access to a
PowerPC CPU register. To do this you can use inline assembly language to
access the register from C. Then use the `bind_func()` function to make
this C function available to forth.

In this example we implement a word called fpscr@ that will push the
value of the FPSCR onto the top of the stack.

In the file arch/ppc/qemu/init.c add this code right above the
`arch_of_init()` function:

```C
 static void get_fpscr(void)
 {
     asm volatile("mffs 0");
     asm volatile("stfd 0, 40(1)");
     uint32_t return_value;
     asm volatile("lwz %0, 44(1)" : "=r"(return_value));
     PUSH(return_value);
 }
```

Then to make this function available from forth we add this line to the
end of the `arch_of_init()` function in the same file:

```C
bind_func("fpscr@", get_fpscr);
```

To build this example you would have to issue this command:

    ./config/scripts/switch-arch ppc && make build-verbose

The resulting file can be used with QEMU like this:

    qemu-system-ppc -bios `<path to openbios folder>`/obj-ppc/openbios-qemu.elf.nostrip

Entering fpscr@ in the OpenBIOS prompt would return the value to the
fpscr.
