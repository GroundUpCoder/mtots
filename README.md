# mtots

The mtots programming langauge

## TLDR

mtots is a scripting language implemented as a collection of
`.c` and `.h` files that is easy to embed in any C/C++ project.

Using mtots should feel a bit like Python, but mtots isn't
meant to be Python. A lot of syntax and function and method names
are borrowed from Python to help someone from Python feel at home,
but all variables need to be declared with `var` or `final`,
making it impossible to run most Python code as mtots or vice versa.

For some sample mtots code see [tests](test/) and [sample apps](misc/apps/)

There is also a vscode extension for [mtots](https://marketplace.visualstudio.com/items?itemName=mtots.mtots)
(The source for the vscode extension is in this repository [here](misc/vscode/))

## Goals

None of these goals are novel in and of themselves, but as far as I know
I am not aware of any single mainstream programming language that satisfies
all these goals.

### Clean Implementation

The language implementation should be a simple collection of C89 sources
and headers that can be included in any project with a C or C++
compiler. It is ok to have a little Makefile or build script for convenience,
but given a conforming C89 compiler on any platform, it should not be very
difficult for someone to build the interpreter by passing sources manually.

In my opinion, C89 is still the gold standard of portability, and the core of
programming language implementation should not need anything more.

The language core should not depend on anything beyond the standard library.

The only mainstream language I know of that satisfies this goal is `Lua`.

### Fun to Use and Play with

I want a language that's fun to use. It should be easy to prototype ideas
and there should be enough libraries and bindings for the language
that given an idea, it should be easy to test.

This is probably the most subjctive goal of the three, but for me
I think `Python` satisfies this requirement for me the best.

### Potential for Optimization

I want a language that could potentially run code very fast, given
enough type annotations and enough resources to improve the implementation.

Or at least as fast, as say `Java`.

## Building and Testing

The `make.py` will build and test the `mtots` executable for some
common platforms. Python3 is required for the make.py script.

To build the `mtots` executable (`out/desktop/mtots`):

```
python make.py desktop
```

To build and test:

```
python make.py test
```

If you have emcc installed and you want to build for webassembly:

```
python make.py web
```

If you have graphviz and want to see a graph of the dependency
between C files:

```
python make.py graph
```

### Assumptions about the Platform

While I've tried to make sure that the code compiles
with any C89 conforming compiler, I've still made a few assumptions
that the C89 standard does not always guarantee.

These assumptions are tested in `mtots_assumptions.h` and `mtots_assumptions.c`.

What can be tested at compile time is tested for in the header,
and the rest are tested for at runtime in `checkAssumptions()`.
`initVM()` will call `checkAssumptions()` and exit if any of the assumptions

Here are a subset of these assumptions:

* there are 8 bits in a byte
* `short` is exactly 2 bytes
* `int` is exactly 4 bytes
* `float` is exactly 32 bits
* `double` is exactly 64 bits

NOTE: there's no need to explicitly specify that a char
is one byte, since a char is required to be 1 byte by
the standard.

Part of the reason for some of these assumptions is because `stdint.h`
is not available in pure C89.

## VSCode Extension

There is a vscode extension for mtots in `misc/vscode`.

Beyond just syntax highlighting, the extension currently supports
goto definition, parameter type hints, show documentation on hover, and
autocomplete for variable, field and method names.

## Implementation Notes

### Calling mtots functions from inside native functions

Calling an mtots function from inside another mtots function
actually will not recurse on the C stack and instead just
push some mtots frame data.

Due to some messiness here, calling an mtots function from
inside a C function called by mtots may be a bit brittle.

At some point, I want to clean this up and make it easier to
do.

### 'final' vs 'var'

Right now, 'final' and 'var' tokens are interchangeable.
However, in the future, I intend for variables declared with 'final'
to actually be final (e.g. as in Java).

Further, at some point, it should be possible to replace uses of
final variables assigned to an immutable value with the actual
values themselves like constants.

### Bitwise Operators

All bitwise operators `<<`, `>>`, `&`, `|`, `^`, `~` are done by first
converting the number to an unsigned 32 bit integer.
