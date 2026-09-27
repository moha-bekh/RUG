# Runtime

This document describes the finalized `Rug` representation, `RugSession` workspace, parser execution cycle and memory ownership rules.

For the grammar-generation pipeline, see [`architecture.md`](architecture.md).

## Table of contents

- [Runtime boundary](#runtime-boundary)
- [`Rug`](#rug)
- [Build state: `RugBuildData`](#build-state-rugbuilddata)
- [Lazy preparation](#lazy-preparation)
- [`Rug::build()`](#rugbuild)
- [`Rug::required_alloc_size()` and `build_into()`](#rugrequiredallocsize-and-buildinto)
- [Final `Rug` image layout](#final-rug-image-layout)
- [Placement-constructed runtime views](#placement-constructed-runtime-views)
- [Finalization and destruction of build structures](#finalization-and-destruction-of-build-structures)
- [`RugSession`](#rugsession)
- [Managed session workspace](#managed-session-workspace)
- [External session workspace](#external-session-workspace)
- [Workspace layout](#workspace-layout)
- [`Session`](#session)
- [`Engine`](#engine)
  - [ACTION lookup](#action-lookup)
  - [SHIFT](#shift)
  - [REDUCE](#reduce)
- [Source-span stack](#source-span-stack)
  - [Non-empty reduction](#non-empty-reduction)
  - [Epsilon reduction](#epsilon-reduction)
- [Reduction order inside `Session`](#reduction-order-inside-session)
- [`RugSession::push()`](#rugsessionpush)
- [Incremental input](#incremental-input)
- [`RugSession::finish()`](#rugsessionfinish)
- [Parse results](#parse-results)
  - [`PARSE_CONSUMED`](#parseconsumed)
  - [`PARSE_ACCEPTED`](#parseaccepted)
  - [`PARSE_ERROR`](#parseerror)
  - [`PARSE_STACK_OVERFLOW`](#parsestackoverflow)
  - [`PARSE_SEMANTIC_ERROR`](#parsesemanticerror)
- [Reset and reuse](#reset-and-reuse)
- [Allocation behavior after construction](#allocation-behavior-after-construction)
- [Threading model](#threading-model)


## Runtime boundary

The main architectural boundary is:

```text
build time
────────────────────────────────────────
SourceFile
ABNFIR
CFG
canonical LR(1) LRGraph
LALR(1) LRGraph
TableIR
SemanticBindings

                 finalize

runtime
────────────────────────────────────────
Rug image
    ├── packed parsing table
    └── callback table

RugSession
    ├── state stack
    └── span stack
```

After `Rug::finalize_into()` succeeds, the STL-heavy build data is destroyed.

Runtime parsing does not need `ABNFIR`, `CFG`, `LRGraph` or `TableIR`.

<p align="right"><a href="#top">↑ Back to top</a></p>

## `Rug`

A `Rug` represents one compiled grammar runtime.

Its important runtime fields are conceptually:

```cpp
void   *m_image_;
size_t  m_image_size_;

size_t  m_parsing_table_size_;
size_t  m_callbacks_offset_;
size_t  m_callback_count_;

TableView       *m_table_;
SemanticRuntime *m_semantics_;
```

`Rug` is intentionally non-copyable.

A `RugSession` stores a reference to its `Rug`, so the `Rug` must outlive all sessions created from it.

<p align="right"><a href="#top">↑ Back to top</a></p>

## Build state: `RugBuildData`

Before finalization, `Rug` owns a `RugBuildData`.

It stores:

```cpp
std::string                     m_abnf_file_;
std::vector<t_pending_binding>  mv_bindings_;
std::vector<uint8_t>            mv_parsing_table_;
std::vector<t_reduce_callback>  mv_callback_table_;

size_t m_callbacks_offset_;
size_t m_required_size_;

bool mb_prepared_;
```

`RugBuildData` is temporary.

It exists to bridge the grammar compiler and the final runtime image.

<p align="right"><a href="#top">↑ Back to top</a></p>

## Lazy preparation

`Rug::prepare()` performs the complete compilation pipeline.

It is invoked automatically by:

```cpp
Rug::required_alloc_size()
Rug::build()
Rug::build_into()
```

If the grammar has already been prepared, a repeated `required_alloc_size()` call does not rebuild it.

If a new `Rug::bind()` is added after preparation but before final build, the prepared packed table and callback table are invalidated and will be regenerated on the next preparation.

No binding can be added after the `Rug` is built.

<p align="right"><a href="#top">↑ Back to top</a></p>

## `Rug::build()`

The managed-memory mode is:

```cpp
Rug rug("grammar.abnf");

rug.build();
```

`Rug::build()` calls `prepare()`, obtains the final required image size, then allocates one writable anonymous mapping with `mmap()`.

It passes that memory to `finalize_into()` with ownership enabled.

The `Rug` destructor later calls `munmap()` only when `mb_owns_memory_` is true.

<p align="right"><a href="#top">↑ Back to top</a></p>

## `Rug::required_alloc_size()` and `build_into()`

Caller-managed mode is:

```cpp
Rug rug("grammar.abnf");

size_t size = rug.required_alloc_size();

void *memory = /* caller-owned region */;

rug.build_into(memory, size);
```

`required_alloc_size()` is not merely a constant-size query. The grammar must be compiled first because the packed-table size depends on:

- generated state count;
- action exception count;
- GOTO count;
- CFG rule count;
- semantic callback count.

`build_into()` requires:

- non-null memory;
- capacity at least `required_alloc_size()`;
- pointer alignment compatible with `sizeof(void*)`.

The caller owns and must preserve the supplied region for the lifetime of the `Rug`.

<p align="right"><a href="#top">↑ Back to top</a></p>

## Final `Rug` image layout

The runtime image is:

```text
base
 │
 ▼
+-----------------------------+
| packed parsing table        |
|                             |
| size = parsing_table_size   |
+-----------------------------+
| alignment padding           |
+-----------------------------+
| t_reduce_callback[]         |
|                             |
| count = callback_count      |
+-----------------------------+
```

`m_callbacks_offset_` is aligned with:

```cpp
sizeof(void*)
```

before callback pointers are copied.

The packed parsing table itself is documented in [`packed-table.md`](packed-table.md).

<p align="right"><a href="#top">↑ Back to top</a></p>

## Placement-constructed runtime views

`Rug` stores aligned raw storage for:

```cpp
TableView
SemanticRuntime
```

because C++98 does not provide `alignas`.

The current implementation uses unions containing:

```cpp
long double
void* char data[sizeof(...)]
```

and constructs the runtime objects with placement `new`.

The objects do not own the underlying image.

They are views over memory already owned by `Rug` or by the external caller.

<p align="right"><a href="#top">↑ Back to top</a></p>

## Finalization and destruction of build structures

After the packed table and callback table have been copied into the final image, `Rug::finalize_into()` constructs:

```cpp
TableView
SemanticRuntime
```

Then:

```cpp
delete m_build_data_;
m_build_data_ = NULL;
```

At that point the build graph no longer exists.

This is why the finalized runtime does not retain the grammar compiler's vectors, maps, LR states, or ABNF tree.

<p align="right"><a href="#top">↑ Back to top</a></p>

## `RugSession`

A `RugSession` represents one mutable parse.

Important members include:

```cpp
const Rug& m_rug_;

void   *m_workspace_;
size_t  m_workspace_size_;
size_t  m_stack_capacity_;

bool    mb_owns_workspace_;

size_t  m_input_offset_;

Session *m_session_;
```

A `RugSession` is also non-copyable.

Multiple sessions can reference the same immutable `Rug`.

Each session has its own state and span stacks.

<p align="right"><a href="#top">↑ Back to top</a></p>

## Managed session workspace

The convenient constructor is:

```cpp
RugSession(const Rug& rug, void *user_ctx, size_t stack_capacity);
```

It computes:

```cpp
RugSession::workspace_size(stack_capacity)
```

and allocates that size with `mmap()`.

The session destructor unmaps the region if it owns it.

A stack capacity below 2 is rejected.

<p align="right"><a href="#top">↑ Back to top</a></p>

## External session workspace

The caller-managed constructor is:

```cpp
RugSession(const Rug& rug, void *user_ctx, void *workspace, size_t workspace_size);
```

This constructor does not receive a requested stack capacity.

Instead, `capacity_from_workspace()` derives the largest parser capacity that fits the provided region.

The workspace must be:

- non-null;
- large enough for at least capacity 2;
- aligned to `sizeof(size_t)`.

The caller retains ownership and must keep the memory valid until the `RugSession` is destroyed.

<p align="right"><a href="#top">↑ Back to top</a></p>

## Workspace layout

For parser stack capacity `N`, `RugSession::workspace_size(N)` lays out:

```text
workspace base
 │
 ▼
+-----------------------------+
| t_id_state state_stack[N]   |
+-----------------------------+
| alignment padding           |
+-----------------------------+
| t_parse_span span_stack[N-1]|
+-----------------------------+
```

The span array has one fewer element than the state array because the LR state stack always contains the initial state in addition to symbols currently represented by spans.

The central invariant is:

```text
state_stack_size == span_stack_size + 1
```

after each completed shift or reduction.

<p align="right"><a href="#top">↑ Back to top</a></p>

## `Session`

`Session` combines:

```text
Engine
span stack
SemanticRuntime pointer
user context pointer
parse error state
semantic error state
```

`Engine` owns no dynamic memory and works on the state-stack array supplied by `RugSession`.

`Session` similarly works on the supplied span-stack array.

<p align="right"><a href="#top">↑ Back to top</a></p>

## `Engine`

`Engine` implements the table-driven LR state machine.

Its stack starts as:

```text
[ state 0 ]
```

### ACTION lookup

For one terminal:

```cpp
action = m_table_.action(top_state(), terminal);
```

The action is one of:

```cpp
ACTION_SHIFT
ACTION_REDUCE
ACTION_ACCEPT
ACTION_ERROR
```

### SHIFT

For:

```text
SHIFT target
```

the target state is pushed.

The input terminal is considered consumed only after this shift succeeds.

If the state stack is full, `Engine` returns:

```cpp
ENGINE_STACK_OVERFLOW
```

instead.

### REDUCE

For a rule:

```text
A -> β
```

`Engine::do_reduce()`:

1. obtains `t_rule_entry` for the rule;
2. removes `rhs_count` states from the state stack;
3. exposes the state beneath the reduced RHS;
4. looks up:

```text
GOTO[base_state][A]
```

5. pushes the target GOTO state.

An epsilon rule has:

```text
rhs_count == 0
```

so it pops nothing and still pushes one GOTO state.

This means an epsilon reduction can increase parser stack depth by one. Stack-capacity checking is therefore necessary on reductions as well as shifts.

<p align="right"><a href="#top">↑ Back to top</a></p>

## Source-span stack

`Session` maintains one `t_parse_span` per grammar symbol represented above the initial LR state.

A shifted input byte receives:

```text
[absolute_offset, absolute_offset + 1)
```

### Non-empty reduction

For a reduction with `rhs_count > 0`, the reduced span is:

```text
begin = first RHS span begin
end   = last RHS span end
```

### Epsilon reduction

For `rhs_count == 0`, the reduced span is:

```text
[empty_position, empty_position)
```

The empty position is the current input position.

This span is then pushed as the span corresponding to the reduced LHS symbol.

<p align="right"><a href="#top">↑ Back to top</a></p>

## Reduction order inside `Session`

`Session::push_terminal()` can perform several reductions before the current terminal is shifted.

The loop is conceptually:

```text
ACTION(current state, current terminal)
        │
        ├── REDUCE
        │      ├── update span stack
        │      ├── dispatch semantic callback
        │      └── lookup ACTION again with same terminal
        │
        ├── SHIFT
        │      ├── push terminal span
        │      └── terminal consumed
        │
        ├── ACCEPT
        │
        └── ERROR
```

The current input byte is not advanced merely because reductions occur.

It advances only after the terminal has been shifted.

<p align="right"><a href="#top">↑ Back to top</a></p>

## `RugSession::push()`

The public byte frontend is:

```cpp
e_parse_result
RugSession::push(const unsigned char *data, size_t size);
```

It feeds the buffer one byte at a time.

For byte `i`, it constructs:

```cpp
span.begin = m_input_offset_;
span.end = m_input_offset_ + 1;
```

and calls:

```cpp
Session::push_terminal()
```

If the result is `PARSE_CONSUMED`, `m_input_offset_` is incremented and the next byte is processed.

If another result occurs, `push()` returns immediately.

This means that on a syntax error, the byte that triggered the error has not been counted as consumed.

<p align="right"><a href="#top">↑ Back to top</a></p>

## Incremental input

The session state is preserved between calls:

```cpp
session.push(chunk1, size1);
session.push(chunk2, size2);
session.push(chunk3, size3);
```

`m_input_offset_` is global to the current session parse, so source spans remain absolute across chunk boundaries.

`RugSession::reset()` resets both parser state and `m_input_offset_` to zero.

<p align="right"><a href="#top">↑ Back to top</a></p>

## `RugSession::finish()`

Application bytes occupy only `0..255`.

End of input is represented by:

```cpp
CFG_END = 256
```

`finish()` does not append a physical byte.

It calls:

```cpp
Session::finish(m_input_offset_)
```

which repeatedly feeds `CFG_END` to the LR engine until the parser:

- reduces;
- accepts;
- reports syntax error;
- reports stack overflow;
- reports a semantic error.

`CFG_END` is never a shiftable input byte.

<p align="right"><a href="#top">↑ Back to top</a></p>

## Parse results

The public result type is:

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

### `PARSE_CONSUMED`

The supplied bytes were consumed and the parser is ready for more input.

### `PARSE_ACCEPTED`

The grammar accepted the complete input.

In normal usage, acceptance is expected after `finish()`.

### `PARSE_ERROR`

The current terminal has no valid ACTION in the current state.

Details are stored in `t_parse_error`.

### `PARSE_STACK_OVERFLOW`

The preallocated stack cannot accommodate the required shift or reduction.

The runtime does not grow the stack dynamically.

### `PARSE_SEMANTIC_ERROR`

A bound user callback returned a non-zero `t_semantic_result`.

Details are stored in `t_semantic_error`.

<p align="right"><a href="#top">↑ Back to top</a></p>

## Reset and reuse

`RugSession::reset()`:

- resets `Engine` to state 0;
- clears the span stack;
- clears parse/semantic error state;
- resets `m_input_offset_` to zero.

The workspace itself is reused.

No grammar rebuild is required.

<p align="right"><a href="#top">↑ Back to top</a></p>

## Allocation behavior after construction

After a `Rug` and `RugSession` have been successfully constructed, the parser runtime operates on:

- immutable `Rug` image memory;
- fixed session workspace;
- caller-owned user context.

`RugSession::push()`, `finish()` and `reset()` perform no internal dynamic allocation.

This guarantee does not restrict user semantic callbacks: callback code may allocate memory if the application chooses to do so.

<p align="right"><a href="#top">↑ Back to top</a></p>

## Threading model

The packed `Rug` image is immutable after build and can be shared conceptually between independent sessions.

Each `RugSession` has independent mutable parser state.

The current code does not add synchronization around user callbacks or user context. Applications are responsible for synchronization when callback state is shared between threads.
