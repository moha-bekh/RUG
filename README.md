# RUG

**RUG** — *Rug Understands Grammar* — is a C++98 parser generator and runtime library that compiles an ABNF grammar into a packed LALR(1) parsing table.

RUG parses application input directly at the **byte level** and does not require a separate lexer or tokenization stage at runtime. Input bytes are used as grammar terminals, making the parser suitable for protocols, structured text, and other byte-oriented formats.

Semantic actions can be associated with grammar non-terminals. Registered callbacks are invoked when a production whose left-hand side is that non-terminal is reduced.

A `Rug` object contains the compiled runtime image. Once built, this representation is immutable and can be shared by multiple `RugSession` instances.

A `RugSession` contains the mutable state of one parsing operation. It consumes input incrementally, executes the LALR(1) state machine, tracks source spans, and dispatches semantic callbacks when required.

## Table of contents

- [High-level architecture](#high-level-architecture)
- [Highlights](#highlights)
- [Getting started](#getting-started)
  - [Build the library](#build-the-library)
  - [Build the examples](#build-the-examples)
- [Basic usage](#basic-usage)
  - [1. Define an ABNF grammar](#1-define-an-abnf-grammar)
  - [2. Create and build a `Rug`](#2-create-and-build-a-rug)
  - [3. Create a `RugSession`](#3-create-a-rugsession)
  - [4. Push input](#4-push-input)
  - [5. Finish the input](#5-finish-the-input)
  - [Semantic actions](#semantic-actions)
  - [Linking](#linking)
- [Memory control](#memory-control)
  - [`Rug` runtime image](#rug-runtime-image)
  - [`RugSession` workspace](#rugsession-workspace)
- [Error handling](#error-handling)
  - [Build errors](#build-errors)
  - [Parse results](#parse-results)
  - [Syntax errors](#syntax-errors)
  - [Semantic errors](#semantic-errors)
  - [Stack exhaustion](#stack-exhaustion)
- [Documentation](#documentation)
- [Project status](#project-status)
- [Contributing](#contributing)
- [Credits](#credits)
- [License](#license)

## High-level architecture

```text
grammar.abnf
     │
     ▼
    RUG
     │
     ├── ABNF frontend
     ├── CFG
     ├── canonical LR(1)
     ├── LALR(1)
     ├── TableIR
     └── packed parse table       Semantic Bindings (manual)
             │                              │
             ▼                              ▼
        packed table                  callback table
             └──────────────┬───────────────┘
                            ▼
                   immutable RUG image
                            ▲
                            │
                       (read only)
                            │
                      ┌─────┴─────┐
                      │           │
                RugSession A  RugSession B
                      │           │
                    input       input
```

The complete compilation and runtime pipeline is described in [`docs/architecture.md`](docs/architecture.md).

<p align="right"><a href="#top">↑ Back to top</a></p>

## Highlights

- **Standard ABNF input** — RUG accepts ABNF based on [RFC 5234](https://www.rfc-editor.org/rfc/rfc5234) together with the `%i` and `%s` string extensions from [RFC 7405](https://www.rfc-editor.org/rfc/rfc7405). Byte-oriented restrictions and unsupported constructs are documented separately.

- **Direct byte-level parsing** — input bytes are used directly as grammar terminals (`t_id_symbol`). No separate lexer or tokenizer interface is required at runtime.

- **Canonical LR(1) to LALR(1)** — `LRBuilder` constructs the canonical LR(1) graph (`LRGraph`). `LALRBuilder` then merges states sharing the same LR(0) core before `TableIRBuilder` produces the parsing table.

- **Packed runtime representation** — the dense `TableIR` representation is converted by `TableBuilder` into a compact immutable table. ACTION rows use a per-state `t_action_span`, sparse `t_action_entry` exceptions, and 32-bit `t_packed_action` values. GOTO rows are stored as sparse `t_goto_entry` entries.

- **Explicit memory control** — `Rug::build()` allocates the final runtime image automatically, while `Rug::required_alloc_size()` and `Rug::build_into()` allow the caller to provide its storage. `RugSession` provides the same choice for its state and span stacks through `RugSession::workspace_size()` and its external-workspace constructor.

- **Incremental sessions and semantic actions** — multiple `RugSession` instances can share the same immutable `Rug`. Input is processed incrementally through `RugSession::push()` and completed with `RugSession::finish()`. Semantic callbacks (`t_reduce_callback`) receive a `t_reduce_event` describing the completed reduction.

- **Structured diagnostics** — grammar build errors, syntax errors, semantic callback failures and parser stack exhaustion are represented through `RugDiag`, `t_parse_error` and `t_semantic_error`.

<p align="right"><a href="#top">↑ Back to top</a></p>

## Getting started

RUG is built as a static C++ library named `librug.a`.

### Build the library

From the repository root:

```sh
make
```

This compiles RUG with:

```text
-Wall -Wextra -Werror -std=c++98
```

and produces `librug.a` at the repository root.

### Build the examples

Two standalone examples are currently provided.

Build the calculator:

```sh
make calculator
./calculator "10 - 2 * (-3 + 5.5)"
```

The calculator demonstrates semantic bindings, reduction events and semantic error handling.

Build the ticket parser:

```sh
make ticket
./ticket path/to/ticket.txt
```

The ticket example demonstrates structured data extraction using the source spans attached to reductions.

Both targets build `librug.a` automatically when required.

The usual Makefile targets are also available:

```sh
make clean
make fclean
make re
```

<p align="right"><a href="#top">↑ Back to top</a></p>

## Basic usage

RUG exposes two main public classes:

- `Rug` builds and owns the immutable runtime image.
- `RugSession` holds the mutable state of one parsing operation.

If the repository is visible from the compiler include search path as `RUG/`, include:

```cpp
#include "RUG/inc/Rug.hpp"
#include "RUG/inc/RugSession.hpp"
```

### 1. Define an ABNF grammar

For example, `hello.abnf`:

```abnf
message  = greeting [my-CRLF]      ; brackets [] make the element optional
greeting = %s"Hello" / %s"Hi"     ; %s forces case-sensitive matching

; numeric terminal values map directly to input bytes
my-CRLF = (CR LF) / LF
CR      = %x0D
LF      = %x0A
```

The first rule in the file is the start rule, so `message` is the start symbol in this example.

### 2. Create and build a `Rug`

```cpp
Rug rug("hello.abnf");

rug.build();
```

`Rug::build()` parses and validates the ABNF grammar, builds the canonical LR(1) and LALR(1) automata, generates `TableIR`, packs it, and finalizes the immutable runtime image.

### 3. Create a `RugSession`

```cpp
static const size_t STACK_CAPACITY = 64;

RugSession session(rug, NULL, STACK_CAPACITY);
```

The `Rug` object must remain alive for the entire lifetime of every `RugSession` that references it.

### 4. Push input

```cpp
const unsigned char input[] = "Hello\r\n";

e_parse_result result = session.push(input, sizeof(input) - 1);
```

`RugSession::push()` consumes bytes while preserving parser state between calls.

Input may therefore be split across several calls:

```cpp
result = session.push((const unsigned char *)"Hel", 3);

if (result == PARSE_CONSUMED)
    result = session.push((const unsigned char *)"lo\r\n", 4);
```

### 5. Finish the input

After all input chunks have returned `PARSE_CONSUMED`, signal the logical end of input:

```cpp
if (result == PARSE_CONSUMED)
    result = session.finish();
```

`RugSession::finish()` injects RUG's logical end symbol (`CFG_END`) and lets the parser perform the remaining reductions.

A successful complete parse returns:

```cpp
PARSE_ACCEPTED
```

### Semantic actions

A callback is bound to a grammar non-terminal before the `Rug` is built:

```cpp
static t_semantic_result on_greeting(const t_reduce_event& event, void *user_context)
{
    (void)event;
    (void)user_context;

    return (SEMANTIC_OK);
}
```

Register it with:

```cpp
Rug rug("hello.abnf");

rug.bind("greeting", on_greeting);
rug.build();
```

Semantic callbacks use the `t_reduce_callback` signature:

```cpp
typedef t_semantic_result (*t_reduce_callback)(const t_reduce_event& event, void *user_context);
```

A `t_reduce_event` exposes the reduced rule through `rule_id`, its left-hand-side symbol through `lhs`, its right-hand-side size through `rhs_count`, the reusable semantic slot index through `rhs_begin`, and the reduced input range through `span`.

### Linking

With the following layout:

```text
path/to/
├── RUG/
│   ├── inc/
│   └── librug.a
└── my-project/
```

an application using:

```cpp
#include "RUG/inc/Rug.hpp"
#include "RUG/inc/RugSession.hpp"
```

can be compiled with:

```sh
c++ -Wall -Wextra -Werror -std=c++98 -Ipath/to main.cpp path/to/RUG/librug.a -o my_parser
```

<p align="right"><a href="#top">↑ Back to top</a></p>

## Memory control

RUG can either manage its runtime memory automatically or use storage provided by the caller.

### `Rug` runtime image

The simplest mode lets RUG allocate the final runtime image:

```cpp
Rug rug("grammar.abnf");

rug.build();
```

Alternatively:

```cpp
Rug rug("grammar.abnf");

size_t size = rug.required_alloc_size();

void *memory = /* caller-provided storage */;

rug.build_into(memory, size);
```

The finalized `Rug` occupies one contiguous runtime region containing the packed parsing table, alignment padding, and the semantic callback table.

Temporary build structures such as `ABNFIR`, `CFG`, canonical/LALR `LRGraph` objects and `TableIR` are discarded after finalization.

Caller-provided memory must satisfy the capacity and alignment requirements checked by `Rug::build_into()`.

### `RugSession` workspace

A parsing session can allocate its own workspace:

```cpp
static const size_t STACK_CAPACITY = 256;

RugSession session(rug, user_context, STACK_CAPACITY);
```

or use caller-provided memory:

```cpp
static const size_t STACK_CAPACITY = 256;

size_t size = RugSession::workspace_size(STACK_CAPACITY);

void *workspace = /* caller-provided storage */;

RugSession session(rug, user_context, workspace, size);
```

The workspace contains the parser state stack and source-span stack.

Once a `RugSession` has been constructed, `RugSession::push()`, `RugSession::finish()` and `RugSession::reset()` operate inside this preallocated workspace and do not perform internal dynamic allocations.

User-provided semantic callbacks remain responsible for their own memory usage.

<p align="right"><a href="#top">↑ Back to top</a></p>

## Error handling

RUG separates grammar build errors from errors encountered while parsing application input.

### Build errors

Invalid ABNF, unsupported constructs and invalid grammar properties are reported through `RugBuildError`.

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

A `RugBuildError` owns a `RugDiag` report containing diagnostics collected during grammar compilation.

### Parse results

`RugSession::push()` and `RugSession::finish()` return an `e_parse_result`:

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

`PARSE_CONSUMED` means the supplied bytes were consumed successfully and more input may follow.

`PARSE_ACCEPTED` means the complete input was accepted by the grammar.

### Syntax errors

When the parsing table returns `ACTION_ERROR`, the session records a `t_parse_error`:

```cpp
if (result == PARSE_ERROR)
{
    const t_parse_error& error = session.syntax_error();

    RugDiag::syntax(error, input, input_size, "input").print(std::cerr, true);
}
```

`t_parse_error` records the input offset, parser state, unexpected terminal and source span.

### Semantic errors

Any non-zero `t_semantic_result` returned by a callback aborts the parse with `PARSE_SEMANTIC_ERROR`.

```cpp
static t_semantic_result on_value(const t_reduce_event& event, void *user_context)
{
    (void)event;
    (void)user_context;

    if (/* semantic validation failed */)
        return (1);

    return (SEMANTIC_OK);
}
```

The resulting `t_semantic_error` stores the user-defined error code and the `t_reduce_event` that triggered it.

```cpp
if (result == PARSE_SEMANTIC_ERROR)
{
    const t_semantic_error& error = session.semantic_error();

    RugDiag::semantic(error, "semantic callback failed", input, input_size, "input").print(std::cerr, true);
}
```

### Stack exhaustion

`RugSession` uses a fixed-capacity parser stack.

If the configured workspace cannot accommodate the required parser depth, the parser returns `PARSE_STACK_OVERFLOW` instead of allocating additional memory.

```cpp
if (result == PARSE_STACK_OVERFLOW)
{
    size_t input_offset = session.input_offset();

    size_t stack_capacity = session.stack_capacity();

    RugDiag::stack_overflow(input_offset, stack_capacity, input, input_size, "input").print(std::cerr, true);
}
```

Detailed diagnostic behavior is described in [`docs/diagnostics.md`](docs/diagnostics.md).

<p align="right"><a href="#top">↑ Back to top</a></p>

## Documentation

Detailed documentation is available in [`docs/`](docs/):

- [`architecture.md`](docs/architecture.md) — compilation pipeline from ABNF to `CFG`, canonical LR(1), LALR(1), `TableIR` and runtime.
- [`abnf.md`](docs/abnf.md) — supported ABNF syntax, RFC 5234 / RFC 7405 behavior and byte-oriented restrictions.
- [`runtime.md`](docs/runtime.md) — `Rug`, `RugSession`, incremental input, workspace layout and memory ownership.
- [`semantic-bindings.md`](docs/semantic-bindings.md) — `Rug::bind()`, `t_reduce_callback`, `t_reduce_event` and user-owned semantic state.
- [`diagnostics.md`](docs/diagnostics.md) — build diagnostics, `t_parse_error`, `t_semantic_error` and `RugDiag`.
- [`packed-table.md`](docs/packed-table.md) — `TableIR`, `t_packed_action`, sparse ACTION/GOTO storage and the final table representation.

<p align="right"><a href="#top">↑ Back to top</a></p>

## Project status

RUG is currently under active development.

The public API and packed table representation should be considered unstable until the first stable release. Changes may still affect class interfaces, runtime layout and internal table structures.

RUG currently targets C++98 and is built with:

```text
-Wall -Wextra -Werror -std=c++98
```

<p align="right"><a href="#top">↑ Back to top</a></p>

## Contributing

Contributions, bug reports and design discussions are welcome.

- Open an **Issue** to report a bug or propose a specific improvement.
- Open a **Pull Request** to submit a code or documentation change.
- Use **GitHub Discussions** for broader design questions, ideas or technical discussions.

<p align="right"><a href="#top">↑ Back to top</a></p>

## Credits

See [`CREDITS.md`](CREDITS.md) for contributors and acknowledgements.

<p align="right"><a href="#top">↑ Back to top</a></p>

## License

RUG is licensed under the [Apache License 2.0](LICENSE).

<p align="right"><a href="#top">↑ Back to top</a></p>
