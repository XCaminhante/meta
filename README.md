Meta, a straightforward compiler-compiler
==========================================

This project is based on [META-II by Val Schorre](https://en.wikipedia.org/wiki/Compiler-compiler#META_II).

MetaG generates C code implementing a parser/compiler for a EBNF-like grammar file.

MetaG is written in itself so it needs to be bootstrapped from a C implementation.

## Build and use

To bootstrap:

> `make meta`

To compile a grammar in the same directory:

> `make grammar`

To compile directly:

> ```
> ./meta grammar.meta grammar.c
> gcc grammar.c -o grammar
> ```

## TODO

- [ ] Understandable messages for syntax errors and warnings
- [ ] Create useful grammar examples
- [ ] Create a demonstration compiler
- [ ] Saner documentation
- [ ] Documentation in english
