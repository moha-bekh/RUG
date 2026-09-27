# ABNF support

RUG uses Augmented Backus-Naur Form (ABNF) as its grammar language.

The frontend is based on RFC 5234 and supports the `%i` / `%s` string-value extensions defined by RFC 7405, with restrictions imposed by RUG's byte-oriented runtime.

This document describes the behavior of the current implementation rather than claiming unrestricted RFC compatibility.

## Table of contents
- [Source rules](#source-rules)
- [Rule names](#rule-names)
- [Rule definitions](#rule-definitions)
- [Alternation](#alternation)
- [Concatenation](#concatenation)
- [Groups](#groups)
- [Options](#options)
- [Repetition](#repetition)
- [Character values](#character-values)
  - [Unprefixed strings](#unprefixed-strings)
  - [`%i`](#i)
  - [`%s`](#s)
  - [Character-value source range](#character-value-source-range)
- [Numeric values](#numeric-values)
  - [Single values](#single-values)
  - [Sequences](#sequences)
  - [Ranges](#ranges)
  - [Byte restriction](#byte-restriction)
- [`prose-val`](#prose-val)
- [Comments](#comments)
- [Grammar whitespace and line continuation](#grammar-whitespace-and-line-continuation)
- [Grammar source newlines](#grammar-source-newlines)
- [Core ABNF rule names](#core-abnf-rule-names)
- [Grammar validation after lowering](#grammar-validation-after-lowering)
- [Byte terminals and `CFG_END`](#byte-terminals-and-cfgend)
- [Summary](#summary)


## Source rules

A grammar is a sequence of ABNF rules.

The first successfully parsed rule is selected as the RUG start rule.

```abnf
message = greeting CRLF
greeting = %s"Hello"
CRLF = CR LF
CR = %x0D
LF = %x0A
```

Here `message` is the start rule.

There is currently no separate API for choosing another start rule.

<p align="right"><a href="#top">↑ Back to top</a></p>

## Rule names

`Lexer::lex_rulename()` follows the ABNF form:

```abnf
rulename = ALPHA *(ALPHA / DIGIT / "-")
```

Examples:

```abnf
message
header1
content-length
```

An underscore is not valid in a rule name:

```abnf
my_rule = "x" ; invalid
```

Rule-name matching in the grammar is case-insensitive.

Internally `ABNFValidator` and `CFGBuilder` canonicalize source rule names to lowercase.

<p align="right"><a href="#top">↑ Back to top</a></p>

## Rule definitions

A normal definition uses `=`:

```abnf
method = %s"GET" / %s"POST"
```

Incremental alternatives use `=/`:

```abnf
method = %s"GET"
method =/ %s"POST"
method =/ %s"DELETE"
```

The current validator requires the base definition to appear before an incremental definition.

The resulting alternatives are lowered to productions of the same `CFG` non-terminal.

<p align="right"><a href="#top">↑ Back to top</a></p>

## Alternation

Alternatives are separated by `/`:

```abnf
answer = %s"yes" / %s"no"
```

`ABNFParser::parse_alternation()` stores an `ABNF_ALT` node when more than one alternative is present.

During CFG lowering, top-level alternatives become multiple productions.

<p align="right"><a href="#top">↑ Back to top</a></p>

## Concatenation

ABNF concatenation is represented by grammar whitespace:

```abnf
request-line = method SP target
```

`ABNFParser::parse_concatenation()` creates `ABNF_CONCAT` nodes.

During lowering, the child expressions are appended in order to one CFG RHS unless a child requires a synthetic non-terminal.

<p align="right"><a href="#top">↑ Back to top</a></p>

## Groups

Parentheses group an expression:

```abnf
line-end = (CR LF) / LF
```

`ABNFParser::parse_group()` does not create a dedicated group node. It returns the inner expression node directly because grouping changes parsing precedence but does not require a separate IR operation after parsing.

<p align="right"><a href="#top">↑ Back to top</a></p>

## Options

Square brackets represent an optional expression:

```abnf
message = greeting [line-end]
```

An `ABNF_OPTION` becomes an epsilon alternative in the CFG.

Conceptually:

```text
$option -> ε
$option -> expression
```

When an option appears directly as the full right-hand side of a source rule, `lower_as_productions()` can add the epsilon and non-empty productions directly to that rule.

When it appears inside a sequence, `lower_to_symbol()` introduces a synthetic non-terminal.

<p align="right"><a href="#top">↑ Back to top</a></p>

## Repetition

The supported ABNF repetition forms include:

```abnf
DIGIT
3DIGIT
1*5DIGIT
1*DIGIT
*5DIGIT
*DIGIT
```

Their meaning is:

```text
element        exactly once
n element      exactly n times
n*m element    between n and m times
n* element     at least n times
*m element     between zero and m times
* element      zero or more times
```

`ABNFParser::parse_repetition()` stores:

```cpp
child
min
max
unbounded
```

inside `t_abnf_repeat`.

Large bounded repetitions are not expanded into one production for every possible count.

`CFGBuilder` uses synthetic exact, bounded and unbounded grammar fragments through:

```cpp
make_exact_repeat()
make_bounded_repeat()
make_unbounded_repeat()
```

The implementation is described in [`architecture.md`](architecture.md).

<p align="right"><a href="#top">↑ Back to top</a></p>

## Character values

### Unprefixed strings

An unprefixed ABNF string is case-insensitive:

```abnf
method = "get"
```

Alphabetic bytes are lowered through synthetic case-insensitive non-terminals.

For example, one source byte conceptually becomes:

```text
$ci -> "G"
$ci -> "g"
```

Non-alphabetic bytes map directly to their byte terminal.

### `%i`

RFC 7405 explicit case-insensitive form:

```abnf
method = %i"get"
```

is handled like the unprefixed string form.

### `%s`

RFC 7405 case-sensitive form:

```abnf
method = %s"GET"
```

is lowered directly to the corresponding byte sequence.

### Character-value source range

The current grammar lexer accepts printable character-value bytes in the ABNF source and rejects control bytes embedded literally inside a quoted `char-val`.

Protocol control bytes should therefore normally be expressed with numeric values such as `%x0D`.

<p align="right"><a href="#top">↑ Back to top</a></p>

## Numeric values

RUG supports binary, decimal and hexadecimal numeric values:

```abnf
bit = %b1
cr = %d13
lf = %x0A
```

### Single values

```abnf
cr = %x0D
```

becomes one byte terminal.

### Sequences

Dot-separated values represent a byte sequence:

```abnf
crlf = %x0D.0A
```

which is equivalent to two consecutive byte terminals.

### Ranges

A range:

```abnf
digit = %x30-39
```

is lowered to a synthetic non-terminal with one terminal production for every byte in the range.

### Byte restriction

All numeric terminals must be within:

```text
0 .. 255
```

`ABNFValidator` rejects values outside that range.

This restriction follows directly from RUG's runtime terminal model, where application input bytes are the terminals.

<p align="right"><a href="#top">↑ Back to top</a></p>

## `prose-val`

The lexer and parser recognize ABNF `prose-val` syntax:

```abnf
value = <externally defined value>
```

but `ABNFValidator` deliberately rejects it:

```text
ABNF prose-val is not supported
```

A `prose-val` cannot be converted automatically into the byte-level CFG required by RUG.

<p align="right"><a href="#top">↑ Back to top</a></p>

## Comments

A semicolon starts an ABNF comment:

```abnf
method = %s"GET" ; request method
```

`Lexer::skip_comment()` ignores bytes until the next grammar newline.

<p align="right"><a href="#top">↑ Back to top</a></p>

## Grammar whitespace and line continuation

Spaces and tabs are represented by `T_WSP`.

The parser also accepts continuation whitespace where a newline is followed by grammar whitespace.

This behavior is used by `match_c_wsp()` while parsing rules.

For example, alternatives can be formatted across lines when the continuation line begins with whitespace.

<p align="right"><a href="#top">↑ Back to top</a></p>

## Grammar source newlines

The grammar lexer accepts LF and CRLF line endings.

A bare CR in the grammar source is rejected because `Lexer::lex_newline()` requires CR to be followed by LF.

This describes the **grammar file encoding**, not the input language produced by the grammar.

Application input can contain any byte `0..255` if the grammar describes it.

<p align="right"><a href="#top">↑ Back to top</a></p>

## Core ABNF rule names

RUG does not currently inject the RFC 5234 core rules automatically.

A reference such as:

```abnf
request = method SP target CRLF
```

must resolve to rules present in the loaded grammar.

For example:

```abnf
SP = %x20
CRLF = CR LF
CR = %x0D
LF = %x0A
```

If a referenced rule is not defined, `ABNFValidator` emits an undefined-rule diagnostic.

<p align="right"><a href="#top">↑ Back to top</a></p>

## Grammar validation after lowering

After ABNF validation, `CFGBuilder` performs additional CFG-level checks.

Current source diagnostics include:

- start symbol is non-productive;
- reachable non-terminal is non-productive;
- source non-terminal is unreachable from the start symbol.

These are currently build errors rather than warnings.

<p align="right"><a href="#top">↑ Back to top</a></p>

## Byte terminals and `CFG_END`

Runtime grammar terminals are:

```text
0 .. 255
```

The internal value:

```cpp
CFG_END = 256
```

is reserved for logical end of input.

It cannot be expressed as an application byte and is injected only by `RugSession::finish()`.

<p align="right"><a href="#top">↑ Back to top</a></p>

## Summary

The current RUG ABNF frontend supports:

```text
rule definitions        =
incremental definitions =/
alternation             /
concatenation
groups                  (...)
options                 [...]
repetition              n, n*m, n*, *m, *
char-val                "..."
RFC 7405 strings        %i"..." and %s"..."
numeric single values   %b, %d, %x
numeric sequences       %x0D.0A
numeric ranges          %x30-39
comments                ;
continued rule layout
```

Current important restrictions are:

```text
prose-val               recognized but rejected
numeric terminals       limited to 0..255
core ABNF rules         not injected automatically
start rule              first source rule
```
