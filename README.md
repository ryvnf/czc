# Compiler for a language in the spirit of C

This is an implementation of a compiler for a modern programming language in
the spirit of C.  The language is currently unnamed, but is the result of a
hobby project to create a better version of C by completely redesigning it.
The new language is supposed to have the same level of abstractions as C and
focus on improving it by having a nicer and more concise syntax and improving
its semantics.  The compiler is written in C and also uses C as an intermediate
language for compilation.

## Design goals

* Preserve the spirit of C
* Only change minor things like syntax and semantics
* Create an aesthetically beautiful language

## Features

* A more readable declaration syntax compared to C
* Less punctuation and more concise syntax compared to C
* Types and variables have different name spaces
* Declarations are expressions
* Expression aliases to replace C's preprocessor macros
* Unambiguous grammar
* Fixed precedence levels of the bitwise operators `&`, `^`, and `|`
* Stronger type system compared to C

## Build and install 

Running this compiler requires GCC to be installed on the system, which it uses
as a step in the compilation process to translate intermediate C code to
machine code.  The compiler has currently only been tested on GNU/Linux.

To build the program run the following command in the directory of this file:

    $ make

This will build the compiler into an executable called `czc`.  The executable
can also be installed by later running:

    $ sudo make install

This will install the program into the `/usr/local/bin` directory.  Setting the
`PREFIX` variable is also supported if you want to install the program in a
different directory.

## How to use

The compiler has the same user interface as GCC.  To compile the source file
`life.z` into an executable `life` the following command is used:

    $ czc -o life life.z

Here `-o` is used to tell the compiler where to write the executable, if
omitted the output file `a.out` is used.  It is also possible to compile the
source file into an object file.  This is done by using the `-c` flag as shown
in the following command:

    $ czc -c life.z

This will create an object file called `life.o` from the source file `life.z`.
It is possible to combine the `-c` flag with the `-o` flag to specify which
file to write the object file to.  It is also possible to generate C-code from
the program.  This is done by using the `--to-c` flag as shown in the following
command:

    $ czc --to-c a.z

This will create a C file called `a.c` from the source file `a.z`.  It is
possible to combine the `--to-c` flag with the `-o` flag to specify which file
to write the C code to.

It is also possible to use the `-f<feature>`, `-O<optimization-level>`,
`-l<library>`, `-L<directory>`, and `-S` flags from GCC.  Refer to GCC's
documentation to find out what those options does.

## Documentation

Currently there does not exist any public documentation of the language.

## Examples

Some example programs written in this language is contained in the [examples
directory](./examples).  The programs can be compiled by running `make` in the
directory or by following the compilation instructions above.  The example
programs are:

| Source file | What it is                                   |
| ----------- | -------------------------------------------- |
| `cat.z`     | Minimal implementation of the command `cat`  |
| `sort.z`    | Minimal implementation of the command `sort` |
| `life.z`    | Implementation of Conway's game of life      |
