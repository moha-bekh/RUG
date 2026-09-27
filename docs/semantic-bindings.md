# Semantic bindings

RUG semantic bindings let application code react when grammar reductions occur.

The design deliberately keeps semantic values outside the parser runtime.

RUG reports **where and what was reduced**; the application decides what semantic data to store and how to represent it.

## Table of contents

- [Binding model](#binding-model)
- [Callback type](#callback-type)
- [`t_reduce_event`](#treduceevent)
  - [`rule_id`](#ruleid)
  - [`lhs`](#lhs)
  - [`rhs_count`](#rhscount)
  - [`span`](#span)
  - [`rhs_begin`](#rhsbegin)
- [Application-owned semantic stack](#application-owned-semantic-stack)
- [Span-only semantics](#span-only-semantics)
- [User context](#user-context)
- [Build-time resolution](#build-time-resolution)
- [Rule metadata](#rule-metadata)
- [Runtime dispatch](#runtime-dispatch)
- [Semantic errors](#semantic-errors)
- [Binding restrictions](#binding-restrictions)
  - [Bind before build](#bind-before-build)
  - [One binding per source non-terminal](#one-binding-per-source-non-terminal)
  - [Unknown names](#unknown-names)
  - [Current name lookup detail](#current-name-lookup-detail)
- [No implicit semantic objects](#no-implicit-semantic-objects)


## Binding model

Bindings are registered before `Rug::build()`:

```cpp
Rug rug("grammar.abnf");

rug.bind("value", on_value);
rug.build();
```

`Rug::bind()` stores pending bindings until the grammar has been compiled far enough to resolve source non-terminal names.

A binding is associated with one **non-terminal**, not one individual production.

Given:

```abnf
value = integer
      / quoted-string
      / boolean
```

this:

```cpp
rug.bind("value", on_value);
```

causes `on_value` to run whenever any CFG production whose left-hand side is `value` is reduced.

The callback can inspect `t_reduce_event::rhs_count`, `rule_id`, input span, or application-owned semantic values to distinguish the production that reduced.

<p align="right"><a href="#top">↑ Back to top</a></p>

## Callback type

The public callback type is:

```cpp
typedef t_semantic_result (*t_reduce_callback)(const t_reduce_event& event, void *user_context);
```

The return type is:

```cpp
typedef int32_t t_semantic_result;
```

with:

```cpp
SEMANTIC_OK == 0
```

Any non-zero result is treated as an application-defined semantic error.

<p align="right"><a href="#top">↑ Back to top</a></p>

## `t_reduce_event`

A callback receives:

```cpp
typedef struct s_reduce_event
{
    size_t       rule_id;

    t_id_symbol  lhs;
    size_t       rhs_count;
    size_t       rhs_begin;

    t_parse_span span;
} t_reduce_event;
```

### `rule_id`

`rule_id` is the `CFG` rule ID that reduced.

The packed runtime preserves CFG rule numbering in `t_rule_entry[]`.

This means:

```text
t_reduce_event::rule_id
```

can be correlated with internal grammar/debug information if the application maintains such metadata.

### `lhs`

`lhs` is the non-terminal symbol ID on the left-hand side of the reduced rule.

Source and synthetic CFG non-terminals use the same `t_id_symbol` namespace.

Public bindings, however, are resolved only from named source non-terminals.

### `rhs_count`

`rhs_count` is the number of grammar symbols on the rule right-hand side.

For:

```text
expr -> expr "+" term
```

the value is:

```text
3
```

For an epsilon rule:

```text
option -> ε
```

it is:

```text
0
```

### `span`

`span` is the input range covered by the reduction:

```cpp
typedef struct s_parse_span
{
    size_t begin;
    size_t end;
} t_parse_span;
```

The interval is half-open:

```text
[begin, end)
```

For a non-empty reduction it extends from the first RHS symbol to the last RHS symbol.

For epsilon it is:

```text
[position, position)
```

### `rhs_begin`

`rhs_begin` is designed to support an application-owned semantic stack.

RUG itself does not own semantic values.

When a reduction occurs, `rhs_begin` identifies:

1. the slot occupied by the first RHS semantic value before reduction;
2. the slot that should be reused for the new LHS semantic value.

For a three-symbol reduction:

```text
A -> B C D
```

an application can treat:

```text
rhs_begin + 0   B
rhs_begin + 1   C
rhs_begin + 2   D
```

as the RHS values and write the result for `A` back to:

```text
rhs_begin
```

This is the pattern used by the calculator example.

<p align="right"><a href="#top">↑ Back to top</a></p>

## Application-owned semantic stack

A possible application context is:

```cpp
typedef struct s_value
{
    /* application value */
} t_value;

typedef struct s_context
{
    t_value *stack;
    size_t   stack_capacity;
} t_context;
```

A helper can retrieve the LHS slot:

```cpp
static t_value * lhs_value(t_context& ctx, const t_reduce_event& event)
{
    return (&ctx.stack[event.rhs_begin]);
}
```

and RHS values:

```cpp
static const t_value * rhs_value(const t_context& ctx, const t_reduce_event& event, size_t index)
{
    return (&ctx.stack[event.rhs_begin + index]);
}
```

The application remains responsible for checking its own semantic-stack capacity and value types.

RUG only guarantees the reduction positions produced by its parser stack.

<p align="right"><a href="#top">↑ Back to top</a></p>

## Span-only semantics

A semantic stack is optional.

The ticket example uses reduction spans directly.

For a rule such as:

```abnf
name = ...
```

the callback can retain:

```cpp
event.span.begin
event.span.end
```

and later reference the original input without copying the field.

Conceptually:

```cpp
field.offset = event.span.begin;
field.len = event.span.end - event.span.begin;
```

This is useful when semantic output can be represented as views into stable input storage.

<p align="right"><a href="#top">↑ Back to top</a></p>

## User context

The second callback argument is the pointer passed when constructing `RugSession`:

```cpp
RugSession session(rug, &application_context, STACK_CAPACITY);
```

The runtime stores that pointer in `Session` and passes it unchanged to every semantic callback.

RUG does not interpret or own the object.

The application must ensure the context remains valid for the session lifetime.

<p align="right"><a href="#top">↑ Back to top</a></p>

## Build-time resolution

`Rug::bind()` initially records:

```cpp
typedef struct s_pending_binding
{
    std::string       name;
    t_reduce_callback callback;
} t_pending_binding;
```

During `Rug::prepare()`, `SemanticBindings` resolves each name through the CFG source-rule registry.

`SemanticBindings` maintains:

```cpp
std::map<t_id_symbol, uint32_t> mm_semantic_ids_;
```

and assigns one sequential `semantic_id` per bound non-terminal.

The callback pointer is placed at the same semantic ID in the build callback array.

<p align="right"><a href="#top">↑ Back to top</a></p>

## Rule metadata

When `TableBuilder` emits the packed rule table:

```cpp
typedef struct s_rule_entry
{
    uint32_t lhs;
    uint32_t rhs_count;
    uint32_t semantic_id;
} t_rule_entry;
```

it asks:

```cpp
SemanticBindings::semantic_id(rule.lhs)
```

Every production with the same bound LHS therefore receives the same `semantic_id`.

Unbound rules use:

```cpp
PARSING_NO_SEMANTIC = 0xFFFFFFFFu
```

<p align="right"><a href="#top">↑ Back to top</a></p>

## Runtime dispatch

At runtime, `Session::dispatch_reduce()` first obtains the packed `t_rule_entry`.

If:

```cpp
rule.semantic_id == PARSING_NO_SEMANTIC
```

no callback is executed.

Otherwise it builds a `t_reduce_event` and calls:

```cpp
SemanticRuntime::dispatch(semantic_id, event, user_context);
```

`SemanticRuntime` is only a lightweight view over:

```cpp
const t_reduce_callback *m_callbacks_;
size_t m_callback_count_;
```

The callback table itself lives inside the finalized `Rug` image.

<p align="right"><a href="#top">↑ Back to top</a></p>

## Semantic errors

A callback reports success with:

```cpp
return (SEMANTIC_OK);
```

A non-zero value is application-defined:

```cpp
enum e_my_error
{
    MY_OK = 0,
    MY_INVALID_VALUE = 1,
    MY_OUT_OF_RANGE = 2
};
```

Example:

```cpp
static t_semantic_result on_number(const t_reduce_event& event, void *user_context)
{
    (void)event;
    (void)user_context;

    if (/* invalid value */)
        return (MY_INVALID_VALUE);

    return (SEMANTIC_OK);
}
```

RUG stores:

```cpp
typedef struct s_semantic_error
{
    bool              has_error;
    t_semantic_result code;
    t_reduce_event    event;
} t_semantic_error;
```

and returns:

```cpp
PARSE_SEMANTIC_ERROR
```

from the current `RugSession::push()` or `finish()` call.

The error can then be retrieved with:

```cpp
session.semantic_error()
```

<p align="right"><a href="#top">↑ Back to top</a></p>

## Binding restrictions

### Bind before build

Calling:

```cpp
rug.bind(...)
```

after the `Rug` has been finalized is rejected.

Bindings affect packed `t_rule_entry::semantic_id` values, so they must be known before the runtime image is created.

### One binding per source non-terminal

`SemanticBindings` rejects a second callback for the same resolved non-terminal.

If multiple behaviors are needed for different productions of one non-terminal, dispatch them from the callback using reduction metadata.

### Unknown names

A binding name must resolve to a named CFG non-terminal.

Synthetic non-terminals introduced by `CFGBuilder` are not registered as public source-rule names.

### Current name lookup detail

Source rule names are canonicalized to lowercase when registered in the CFG.

The current `SemanticBindings::bind()` lookup uses the provided string directly against that registry.

Until that API is normalized, using lowercase rule names in `Rug::bind()` matches the current implementation reliably.

<p align="right"><a href="#top">↑ Back to top</a></p>

## No implicit semantic objects

RUG does not construct AST nodes, variants, token values, strings or application objects automatically.

This is intentional.

The runtime supplies:

```text
rule identity
LHS identity
RHS count
semantic slot position
source span
user context
```

and leaves semantic representation to the application.

This keeps the parser runtime independent of application data types and preserves the fixed-workspace runtime model.
