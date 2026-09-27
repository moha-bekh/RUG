# Diagnostics

RUG diagnostics are split between grammar-build diagnostics and runtime parse errors.

The main public diagnostic types are:

```text
RugBuildError
RugDiag
t_parse_error
t_semantic_error
```

## Table of contents

- [Build diagnostics](#build-diagnostics)
- [`RugBuildError`](#rugbuilderror)
- [`RugDiag`](#rugdiag)
- [Source ownership](#source-ownership)
- [Formatting](#formatting)
- [Grammar build errors](#grammar-build-errors)
  - [`Lexer`](#lexer)
  - [`ABNFParser`](#abnfparser)
  - [`ABNFValidator`](#abnfvalidator)
  - [`CFGBuilder`](#cfgbuilder)
- [Runtime result model](#runtime-result-model)
- [Syntax errors: `t_parse_error`](#syntax-errors-tparseerror)
  - [`offset`](#offset)
  - [`state`](#state)
  - [`terminal`](#terminal)
  - [`span`](#span)
- [`RugDiag::syntax()`](#rugdiagsyntax)
- [Semantic errors: `t_semantic_error`](#semantic-errors-tsemanticerror)
- [`RugDiag::semantic()`](#rugdiagsemantic)
- [Stack exhaustion](#stack-exhaustion)
- [Generic failures](#generic-failures)
- [Source-less reports](#source-less-reports)
- [Color](#color)
- [Error ownership and lifetime](#error-ownership-and-lifetime)
  - [Build](#build)
  - [Runtime](#runtime)
- [Internal invariants versus public errors](#internal-invariants-versus-public-errors)


## Build diagnostics

Grammar compilation uses `DiagnosticEngine`.

It is constructed from the grammar `SourceFile` and internally owns one `RugDiag`:

```cpp
class DiagnosticEngine
{
    RugDiag m_report_;
};
```

Build stages report source-relative errors through:

```cpp
error(span, message)
warning(span, message)
note(span, message)
```

The current build pipeline checks diagnostics between major phases:

```text
Lexer
  ↓
ABNFParser
  ↓
ABNFValidator
  ↓
CFGBuilder
```

If an error has been collected, `DiagnosticEngine::throw_if_errors()` throws `RugBuildError`.

<p align="right"><a href="#top">↑ Back to top</a></p>

## `RugBuildError`

`RugBuildError` derives from `std::runtime_error` and owns a copy of the diagnostic report:

```cpp
class RugBuildError : public std::runtime_error
{
    RugDiag m_diagnostics_;
};
```

Typical handling:

```cpp
try
{
    Rug rug("grammar.abnf");
    rug.build();
}
catch (const RugBuildError& error)
{
    error.print(std::cerr, true);
    return (1);
}
```

The report can also be accessed with:

```cpp
error.diagnostics()
```

<p align="right"><a href="#top">↑ Back to top</a></p>

## `RugDiag`

A `RugDiag` contains a sequence of entries:

```cpp
typedef struct s_rug_diag
{
    e_rug_diag_level level;
    std::string      message;

    bool   has_span;
    size_t offset;
    size_t lenght;
} t_rug_diag;
```

Diagnostic levels are:

```cpp
RUG_DIAG_ERROR
RUG_DIAG_WARNING
RUG_DIAG_NOTE
```

The report also tracks total error and warning counts.

<p align="right"><a href="#top">↑ Back to top</a></p>

## Source ownership

When a `RugDiag` is constructed with source data:

```cpp
RugDiag(name, data, size)
```

the source is copied into the report.

This lets the report format source excerpts independently of the original input buffer lifetime.

Build diagnostics receive their source from `SourceFile`.

Runtime diagnostic factories can similarly receive application input.

<p align="right"><a href="#top">↑ Back to top</a></p>

## Formatting

`RugDiag::print()` renders entries in a compiler-style form:

```text
error: message
 --> input:line:column
  |
N | source text
  |     ^~~~~
```

Color output is opt-in:

```cpp
report.print(std::cerr, true);
```

The formatter:

- indexes line starts;
- expands tabs;
- escapes control/non-printable bytes;
- limits displayed source width;
- marks the relevant byte range.

`DIAG_WINDOW_WIDTH` currently controls the source window width.

<p align="right"><a href="#top">↑ Back to top</a></p>

## Grammar build errors

Errors can originate from multiple build components.

### `Lexer`

Examples include:

- unexpected grammar characters;
- invalid `%` prefixes;
- unterminated `char-val`;
- malformed numeric values;
- CR not followed by LF.

### `ABNFParser`

Examples include:

- expected rule name;
- missing `=` or `=/`;
- missing closing `)` or `]`;
- malformed rule end;
- invalid expression structure.

### `ABNFValidator`

Examples include:

- duplicate base rule definitions;
- incremental definitions without a base definition;
- references to undefined rules;
- unsupported `prose-val`;
- numeric terminals outside `0..255`.

### `CFGBuilder`

Examples include:

- non-productive start symbol;
- reachable non-productive non-terminal;
- source non-terminal unreachable from the start symbol.

Parsing-table conflicts are currently raised by `TableIR` as logic errors rather than source-rich `RugDiag` entries.

<p align="right"><a href="#top">↑ Back to top</a></p>

## Runtime result model

`RugSession::push()` and `RugSession::finish()` return:

```cpp
enum e_parse_result
{
    PARSE_CONSUMED,
    PARSE_ACCEPTED,
    PARSE_ERROR,
    PARSE_STACK_OVERFLOW,
    PARSE_SEMANTIC_ERROR
};
```

Only the last three represent runtime failure classes.

<p align="right"><a href="#top">↑ Back to top</a></p>

## Syntax errors: `t_parse_error`

A syntax error is stored as:

```cpp
typedef struct s_parse_error
{
    bool has_error;

    size_t      offset;

    t_id_state  state;
    t_id_symbol terminal;

    t_parse_span span;
} t_parse_error;
```

### `offset`

Absolute byte position in the current `RugSession` input stream.

### `state`

The LR parser state in which no valid ACTION exists for the current terminal.

### `terminal`

The unexpected byte terminal, or `CFG_END` when the error occurs during `finish()`.

### `span`

For a normal input byte:

```text
[offset, offset + 1)
```

For logical end of input:

```text
[offset, offset)
```

Retrieve the error with:

```cpp
const t_parse_error& error = session.syntax_error();
```

Calling `syntax_error()` when no syntax error is stored is an API misuse.

<p align="right"><a href="#top">↑ Back to top</a></p>

## `RugDiag::syntax()`

A formatted syntax report can be built with:

```cpp
RugDiag::syntax(session.syntax_error(), input, input_size, "input");
```

The current message identifies the unexpected terminal.

`terminal_name()` formats:

- printable bytes as character plus hexadecimal value;
- CR, LF and TAB with escaped names;
- non-printable bytes as hexadecimal;
- `CFG_END` as end of input.

The current formatter also records the parser state as diagnostic context.

RUG does not currently compute or print an expected-terminal set.

<p align="right"><a href="#top">↑ Back to top</a></p>

## Semantic errors: `t_semantic_error`

A semantic callback can return an application-defined non-zero `t_semantic_result`.

The runtime stores:

```cpp
typedef struct s_semantic_error
{
    bool              has_error;
    t_semantic_result code;
    t_reduce_event    event;
} t_semantic_error;
```

Retrieve it with:

```cpp
const t_semantic_error& error = session.semantic_error();
```

The `code` belongs entirely to the application.

RUG does not assign meaning to non-zero semantic error values.

The attached `t_reduce_event` identifies the reduction and source span in which the application rejected the semantic value.

<p align="right"><a href="#top">↑ Back to top</a></p>

## `RugDiag::semantic()`

A semantic report can be created with:

```cpp
RugDiag::semantic(session.semantic_error(), "invalid application value", input, input_size, "input");
```

The supplied message is application-defined.

The reduction span is used to highlight the source range.

<p align="right"><a href="#top">↑ Back to top</a></p>

## Stack exhaustion

`PARSE_STACK_OVERFLOW` means the fixed session workspace does not have enough parser-stack capacity for the next operation.

This is not a C++ allocation failure.

The parser deliberately does not resize its runtime stacks.

A report can be created with:

```cpp
RugDiag::stack_overflow(session.input_offset(), session.stack_capacity(), input, input_size, "input");
```

The diagnostic records the configured state-stack capacity.

Stack exhaustion can occur on:

- SHIFT, when another parser state must be pushed;
- REDUCE, especially an epsilon reduction that pops zero states and pushes one GOTO state.

<p align="right"><a href="#top">↑ Back to top</a></p>

## Generic failures

`RugDiag::failure()` creates a report containing one span-less error:

```cpp
RugDiag::failure("parser finished without accepting the input", "my parser");
```

This is useful for application-level failures that do not correspond to a precise source span.

<p align="right"><a href="#top">↑ Back to top</a></p>

## Source-less reports

A `t_rug_diag` can have:

```cpp
has_span == false
```

In that case formatting prints the message without attempting a source excerpt.

This is useful when an application no longer retains the original streamed source data.

<p align="right"><a href="#top">↑ Back to top</a></p>

## Color

Color is controlled only when printing:

```cpp
report.print(out, false);
report.print(out, true);
```

The report itself stores semantic diagnostic information, not ANSI color sequences.

<p align="right"><a href="#top">↑ Back to top</a></p>

## Error ownership and lifetime

### Build

`RugBuildError` owns its `RugDiag`, and that `RugDiag` owns a copy of the grammar source.

### Runtime

`t_parse_error` and `t_semantic_error` are stored inside the session.

They remain available until the session error state is replaced or the session is reset.

A `RugDiag` created from them becomes an independent formatted report and can own a copy of the supplied application source.

<p align="right"><a href="#top">↑ Back to top</a></p>

## Internal invariants versus public errors

The normal user-visible runtime errors are:

```text
syntax error
semantic callback error
stack exhaustion
```

Broken parser-table invariants are different.

The finalized table is validated by `TableView` during initialization, after which the runtime can trust internal offsets, rule references and transitions.

Internal invariant failures should therefore not be treated as ordinary input errors.
