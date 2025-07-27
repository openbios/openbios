# FCode Suite

## What is the OpenBIOS FCODE Suite?

OpenBIOS provides a sophisticated set of FCODE utilities:

- the tokenizer **toke**
- the detokenizer **detok**
- and a PCI rom header utility.
- a portable implementation of forth local values

These files are offered without any warranty. If you experience
problems, please contact the [OpenBIOS mailinglist](Mailinglist).

## Downloading the OpenBIOS FCode Suite

**The latest version of the OpenBIOS FCode Suite is 1.0.2 (released
2006-10-30)**

### Source Code

View the [Sources](https://github.com/openbios/fcode-utils) online.
Available as a ZIP-File: [FCode-utils
1.0.2](https://github.com/openbios/fcode-utils/archive/v1.0.2.zip)

See the
[ChangeLog](https://raw.githubusercontent.com/openbios/fcode-utils/v1.0.2/ChangeLog)
for a list of changes since 1.0.1. A hand crafted html document
describes [some more
changes](http://www.openbios.org/data/visiblediffs/V01_versus_V2x/).

### Documentation

There are three documents, all in html format, plus a sub-directory of
templates that provide common formatting support.

It is important that these be kept in the same directory, as there are
some links from one file to another.

The documents are User's Guides to:

- the [New Features of the
  Tokenizer](http://www.openbios.org/data/fcodesuite/Documents/toke.html),
- [the
  Detokenizer](http://www.openbios.org/data/fcodesuite/Documents/detok.html),
  and
- the [Local Values
  feature](http://www.openbios.org/data/fcodesuite/Documents/localvalues.html),
  which is mentioned briefly in the Tokenizer User's Guide and described
  fully in the Local Values document.

These documents are also part of the source code repository.

There is also doxygen generated documentation available

- [doxygen documentation for toke
  0.6.10](http://openbios.org/data/toke/toke-0.6.10)
- [doxygen documentation for toke
  1.0.0](http://openbios.org/data/toke/toke-1.0)
- TODO: doxygen documentation for toke 1.0.2

### Executables for three platforms

While you can find a couple of executables here we strongly recommend
that you compile the FCode toolchain from the sources above so you gain
from the integration work and fixes that have been done since these
executables have been created.

There are three programs: the Tokenizer, the Detokenizer and the
ROMHeaders utility.
([Binaries](http://www.openbios.org/data/fcodesuite/Binaries))

There is a version for each of three platforms (i.e., combinations of
Processor and O/S): Cygwin running on an X86, GNU Linux running on a
Power-PC, and AIX running on a Power-PC.

There are two variants of each version: One that has level-2
Optimization and one that has no optimization at all, which I provided
for purposes of Debugging. Optimization causes some routines and
variables to become obscured and inaccessible to debuggers, and also
re-arranges the sequence of execution in a way that can become confusing
during single-stepping.

And finally, for each, there is a "stripped" and an "unstripped"
executable image. The "unstripped" image has an extension of
"unstripped"; the "stripped" image has no extension.

There are separate directories for the Debug and Optim(ized Level)2
variants.

Under each are sub-directories for the different platforms, within which
the executable images reside.

All binaries are also available in a single Tar-File:
[Binaries.tar.bz2](http://www.openbios.org/data/fcodesuite/Binaries.tar.bz2)

### Local Value Support

Includ-able tokenizer-source files for [Local Values
Support](https://github.com/openbios/fcode-utils/tree/master/localvalues)
(explained in one of the User's Guide documents). Five files:

- One supplies the [basic
  functionality](https://github.com/openbios/fcode-utils/tree/master/localvalues/LocalValuesSupport.fth)
- the second adds a [development-time
  facility](https://github.com/openbios/fcode-utils/tree/master/localvalues/LocalValuesDevelSupport.fth)
- the third generates a variant behavior (["Global"
  scope](https://github.com/openbios/fcode-utils/tree/master/localvalues/GlobalLocalValues.fth)
  rather than scope limited to a single Device-Node)
- and the fourth [combines the "Global" variant behavior with the
  development-time
  facility](https://github.com/openbios/fcode-utils/tree/master/localvalues/GlobalLocalValuesDevel.fth).
- The fifth [allows the choice of combinations to be governed by
  command-line
  switches](https://github.com/openbios/fcode-utils/tree/master/localvalues/TotalLocalValuesSupport.fth),
  and is probably the best to use with Makefiles in commercial
  development and production environments.

There is commentation in each one explaining how it is to be used.

Available as part of the OpenBIOS FCODE suite.

### Todo

A list of "Still To Be Done" items, excerpted from the commentation in
the Sources

The source files have, scattered among their commentation, an occasional
item discussing a feature or implementation detail that might be worth
attention in future revisions.

This file is a [collection of all of
them](https://github.com/openbios/fcode-utils/tree/master/TODO) in a
single convenient location.

## Unit-Test Suite

### The suite of unit-test cases

This is the [accumulation of
test-cases](https://github.com/openbios/fcode-utils/tree/master/testsuite)
that were created in the course of development. Some of these are a
straightforward invocation of a feature, others are convoluted
combinations of features whose interaction needed to be carefully
watched, and still others are collections of coding errors, for purposes
of verifying the Error-detection capabilities of the Tokenizer. They are
grouped into sub-directories representing broad categories.

Run the unit-test cases with

    $ make tests

### Test Tools

The tools to run the Unit-Test Suite as a batch and examine the results
for changes relative to the results from a previous run.

The process of manually running a unit-test and comparing against the
previous output, after every change, became unwieldy, especially when it
came to running the entire suite of tests. These scripts were developed
to automate both processes:

- [AutoExec](https://github.com/openbios/fcode-utils/tree/master/testsuite/AutoExec)
  automates the execution, and
- [AutoCompare](https://github.com/openbios/fcode-utils/tree/master/testsuite/AutoCompare)
  automates the comparison.

There is commentation in each explaining how it is used.

## Unit-Test Suite Logs

These can be used as base-lines for comparison against future versions,
or, if so be, versions compiled for additional platforms.

Note that a comparison of these against each other will not yield exact
identity. Some of the test-cases, for instance, code the current date
and time, others display a complete file-path, and still one other
attempts to load a file for encoding using a syntax that is erroneous on
some O/S's but not on others.

All in all, five or six file differences will be expected to be reported
by AutoCompare.

- The results from a run of the Unit-Test Suite on the
  [X86/Cygwin](https://github.com/openbios/fcode-utils/tree/master/testlogs/testlogs-x86-cygwin)
  platform.

<!-- -->

- The results from a run of the Unit-Test Suite on the
  [PowerPC/Linux](https://github.com/openbios/fcode-utils/tree/master/testlogs/testlogs-ppc-linux)
  platform.

<!-- -->

- The results from a run of the Unit-Test Suite on the
  [PowerPC/AIX](https://github.com/openbios/fcode-utils/tree/master/testlogs/testlogs-ppc-aix)
  platform.

## Coverage Reports

The test suite has been run using gcov/lcov to produce graphical code
coverage reports.

- [coverage report for FCODE suite
  1.0.2](http://www.openbios.org/data/coverage-fcode-suite-1.0.2/)
- [coverage report for toke
  1.0.2](http://www.openbios.org/data/coverage-toke-1.0.2/)
- [coverage report for toke
  1.0.0](http://openbios.org/data/toke/coverage/)

## Kudos and Thanks

The OpenBIOS FCODE Suite has been significantly enhanced to meet
commercial grade requirements by [David
Paktor](mailto:David@paktor.biz). Regarding to code readability and
stability, he made the FCODE suite the best part of OpenBIOS. And it's
probably the best FCODE development environment out there. **Thank you,
David!**
