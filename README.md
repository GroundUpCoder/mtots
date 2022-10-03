# mtots

An interpreted programming language based loosly on
the book Crafting Interpreters.

## Assumptions about the Platform

While I've tried to make sure the code works
with a C89 compiler, I've still made a few assumptions
that the C89 standard does not always guarantee.

I list these assumptions explicitly here:

* there are 8 bits in a byte
* short is exactly 2 bytes
* int is exactly 4 bytes

NOTE: there's no need to explicitly specify that a char
is one byte, since a char is required to be 1 byte by
the standard.

## Building and Testing

There is a `Makefile`, but this is mostly for demonstrative purposes
and only works with MacOS.

### Building

Run the appropriate `build-*` script under the `script` directory
to build the `mtots` binary.

### Testing

Testing requires a Python3 interpreter.

Once you have built the `mtots` binary with the appropriate `build-*`
script, you can test it with

```
python3 scripts/run-tests.py
```

## Implementation Notes

### Calling mtots functions from inside native functions

Calling an mtots function from inside another mtots function
actually will not recurse on the C stack.

A consequence of this is that it's impossible to call an mtots
function from inside a native function.

I think I'm ok with this, but I may want to reconsider this in the future

## 'final' vs 'var'

Right now, 'final' and 'var' tokens are interchangeable.
However, in the future, I intend for variables declared with 'final'
to actually be final (e.g. as in Java).

Further, at some point, it should be possible to replace uses of
final variables assigned to an immutable value with the actual
values themselves like constants.

### Bitwise Operators

All bitwise operators `<<`, `>>`, `&`, `|`, `^`, `~` are done by first
converting the number to an unsigned 32 bit integer.
