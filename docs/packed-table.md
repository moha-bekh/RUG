# Packed table

This document describes the current packed parsing-table representation produced by `TableBuilder` and read by `TableView`.

It is an in-memory runtime representation, not yet a declared portable serialization format.

The exact type and field names below match the current codebase.

## Table of contents

- [From `TableIR` to runtime table](#from-tableir-to-runtime-table)
- [Relevant packed types](#relevant-packed-types)
- [Packed ACTION encoding](#packed-action-encoding)
- [ACTION row compression](#action-row-compression)
  - [Example](#example)
- [Choosing the default action](#choosing-the-default-action)
- [ACTION entry ordering](#action-entry-ordering)
- [ACTION lookup](#action-lookup)
- [Sparse GOTO storage](#sparse-goto-storage)
- [GOTO entry ordering](#goto-entry-ordering)
- [Rule metadata](#rule-metadata)
  - [`lhs`](#lhs)
  - [`rhs_count`](#rhscount)
  - [`semantic_id`](#semanticid)
- [Blob header](#blob-header)
- [Blob layout](#blob-layout)
- [Layout computation](#layout-computation)
  - [Count packed entries](#count-packed-entries)
  - [Compute section offsets](#compute-section-offsets)
- [Building into memory](#building-into-memory)
- [Arithmetic checks](#arithmetic-checks)
- [`TableView`](#tableview)
- [Validation](#validation)
  - [Header and total size](#header-and-total-size)
  - [Alignment](#alignment)
  - [Section bounds](#section-bounds)
  - [Non-overlap](#non-overlap)
  - [ACTION spans](#action-spans)
  - [Packed action targets](#packed-action-targets)
  - [GOTO spans](#goto-spans)
  - [Rule entries](#rule-entries)
- [Packed table versus complete `Rug` image](#packed-table-versus-complete-rug-image)
- [Memory/performance trade-off](#memoryperformance-trade-off)
  - [Dense `TableIR`](#dense-tableir)
  - [Packed runtime table](#packed-runtime-table)
- [Current portability status](#current-portability-status)


## From `TableIR` to runtime table

`TableIR` is deliberately simple and dense.

For every parser state it contains:

```text
ACTION[state][terminal]
GOTO[state][non-terminal]
```

ACTION has one column for each:

```text
0 .. 255     byte terminal
256          CFG_END
```

GOTO has one column for each CFG non-terminal.

This representation is convenient during table generation and conflict detection, but much of it is repetitive or invalid.

`TableBuilder` converts it into a compact blob optimized for runtime lookup.

<p align="right"><a href="#top">↑ Back to top</a></p>

## Relevant packed types

The packed format is defined by:

```cpp
t_packed_action
t_action_span
t_action_entry
t_blob_span
t_goto_entry
t_rule_entry
t_table_blob_hdr
```

<p align="right"><a href="#top">↑ Back to top</a></p>

## Packed ACTION encoding

Parser actions are:

```cpp
enum e_action_type
{
    ACTION_ERROR,
    ACTION_SHIFT,
    ACTION_REDUCE,
    ACTION_ACCEPT
};
```

The enum values fit in two bits.

RUG stores one action in:

```cpp
typedef uint32_t t_packed_action;
```

using:

```text
31                               2 1 0
+--------------------------------+---+
|             value              |type|
+--------------------------------+---+
            30 bits               2
```

The helpers are:

```cpp
pack_action()
unpack_action_type()
unpack_action_value()
unpack_packed_action()
```

Encoding is:

```cpp
((uint32_t)value << 2) | (uint32_t)type
```

The masks are:

```cpp
PACKED_ACTION_TYPE_BITS = 2
PACKED_ACTION_TYPE_MASK = 0x3
PACKED_ACTION_VALUE_MAX = 0x3FFFFFFF
```

Therefore SHIFT targets and REDUCE rule IDs stored in an action must fit in 30 bits.

`TableBuilder::validate_layout()` validates every dense `TableIR` action with `pack_action()` before finalizing the blob.

<p align="right"><a href="#top">↑ Back to top</a></p>

## ACTION row compression

A dense ACTION row contains 257 actions.

Most parser states often repeat the same action across many terminals.

Instead of storing all 257 entries, RUG chooses one **default action** for each state and stores only exceptions.

The per-state descriptor is:

```cpp
typedef struct s_action_span
{
    uint32_t        offset;
    uint32_t        count;
    t_packed_action default_action;
} t_action_span;
```

Exceptions are:

```cpp
typedef struct s_action_entry
{
    uint32_t        terminal;
    t_packed_action action;
} t_action_entry;
```

### Example

A conceptual dense row:

```text
terminal    action

0           ERROR
1           ERROR
2           ERROR
...
'+'         SHIFT 8
...
255         ERROR
CFG_END     ERROR
```

becomes:

```text
default_action = ERROR

exceptions:
    '+' -> SHIFT 8
```

The state `t_action_span` points to the state's contiguous exception range inside the global `t_action_entry[]` array.

<p align="right"><a href="#top">↑ Back to top</a></p>

## Choosing the default action

`find_default_action()` scans the full row and chooses the most common action.

The current implementation is intentionally simple because there are only 257 possible terminal columns.

It runs at generation time, never during parsing.

Conceptually:

```text
for every candidate action in the row:
    count how many terminals use it

choose the candidate with the highest count
```

Ties retain the first action that reached the current best count because the comparison updates only on a strictly larger count.

This policy affects packing size, not parser semantics.

<p align="right"><a href="#top">↑ Back to top</a></p>

## ACTION entry ordering

`TableBuilder::build_into()` scans terminals in ascending order:

```text
0
1
...
255
CFG_END
```

Only exceptions are appended.

Therefore each state's exception subrange is sorted strictly by `terminal`.

`TableView::validate()` verifies this property.

<p align="right"><a href="#top">↑ Back to top</a></p>

## ACTION lookup

`TableView::action(state, terminal)`:

1. obtains `t_action_span` for the state;
2. binary-searches the state's `t_action_entry` exception slice;
3. returns the matching exception if present;
4. otherwise returns `default_action`.

Conceptually:

```text
                 terminal
                    │
                    ▼
         binary search exceptions
             │               │
           found           missing
             │               │
             ▼               ▼
       entry.action     default_action
```

This trades dense constant-index lookup for a smaller table plus a binary search over only the exceptions of one state.

<p align="right"><a href="#top">↑ Back to top</a></p>

## Sparse GOTO storage

Dense `TableIR` GOTO rows contain many:

```cpp
PARSING_INVALID_GOTO
```

values.

The packed representation stores only valid transitions.

The per-state slice is:

```cpp
typedef struct s_blob_span
{
    uint32_t offset;
    uint32_t count;
} t_blob_span;
```

A valid entry is:

```cpp
typedef struct s_goto_entry
{
    uint32_t non_terminal;
    uint32_t target;
} t_goto_entry;
```

`non_terminal` is not the full `t_id_symbol`.

It is the compact index:

```text
full symbol = CFG_FIRST_NON_TERMINAL + non_terminal
```

This keeps the stored GOTO key independent of the fixed byte-terminal prefix.

<p align="right"><a href="#top">↑ Back to top</a></p>

## GOTO entry ordering

During packing, `TableBuilder` iterates non-terminal indices in ascending order and appends only valid targets.

Each state's `t_goto_entry` slice is therefore strictly sorted by `non_terminal`.

`TableView::goto_state()` uses binary search.

If no matching entry exists, it returns:

```cpp
PARSING_INVALID_GOTO
```

<p align="right"><a href="#top">↑ Back to top</a></p>

## Rule metadata

The runtime does not need full CFG RHS contents during a reduction.

It only needs:

- which non-terminal is produced;
- how many states must be popped;
- which semantic callback, if any, must run.

That data is stored in:

```cpp
typedef struct s_rule_entry
{
    uint32_t lhs;
    uint32_t rhs_count;
    uint32_t semantic_id;
} t_rule_entry;
```

The array preserves CFG rule numbering exactly:

```text
rule_entries[rule_id]
```

corresponds to:

```text
CFG rule rule_id
```

This direct indexing is why `t_reduce_event::rule_id` can be passed through the runtime without remapping.

### `lhs`

Stored as the full CFG `t_id_symbol` value.

### `rhs_count`

Number of symbols to remove during reduction.

The packed table does **not** store the RHS symbol sequence because runtime reduction does not need it.

### `semantic_id`

Index into the `Rug` callback table.

An unbound rule stores:

```cpp
PARSING_NO_SEMANTIC = 0xFFFFFFFFu
```

<p align="right"><a href="#top">↑ Back to top</a></p>

## Blob header

The current header is:

```cpp
typedef struct s_table_blob_hdr
{
    uint32_t total_size;

    uint32_t state_count;
    uint32_t non_terminal_count;
    uint32_t rule_count;
    uint32_t semantic_count;

    uint32_t action_count;
    uint32_t goto_count;

    uint32_t action_spans_offset;
    uint32_t action_entries_offset;

    uint32_t goto_spans_offset;
    uint32_t goto_entries_offset;

    uint32_t rule_entries_offset;
} t_table_blob_hdr;
```

All offsets are relative to the beginning of the packed-table blob.

<p align="right"><a href="#top">↑ Back to top</a></p>

## Blob layout

The table layout is:

```text
base
 │
 ▼
+-----------------------------------+
| t_table_blob_hdr                  |
+-----------------------------------+
| alignment                         |
+-----------------------------------+
| t_action_span[state_count]        |
+-----------------------------------+
| alignment                         |
+-----------------------------------+
| t_action_entry[action_count]      |
+-----------------------------------+
| alignment                         |
+-----------------------------------+
| t_blob_span[state_count]          |
+-----------------------------------+
| alignment                         |
+-----------------------------------+
| t_goto_entry[goto_count]          |
+-----------------------------------+
| alignment                         |
+-----------------------------------+
| t_rule_entry[rule_count]          |
+-----------------------------------+
| final alignment                   |
+-----------------------------------+
```

`TableBuilder::align4()` aligns each section to a 4-byte boundary.

The final `m_required_size_` is also aligned to 4 bytes.

<p align="right"><a href="#top">↑ Back to top</a></p>

## Layout computation

`TableBuilder::compute_layout()` performs two major tasks before writing memory.

### Count packed entries

It computes:

```cpp
m_action_count_
m_goto_count_
```

where:

```text
action_count = number of ACTION exceptions across all states
goto_count   = number of valid GOTO transitions across all states
```

### Compute section offsets

Starting after `sizeof(t_table_blob_hdr)`, it assigns:

```cpp
m_action_spans_offset_
m_action_entries_offset_
m_goto_spans_offset_
m_goto_entries_offset_
m_rule_entries_offset_
```

All size arithmetic uses overflow checks.

The packed format stores counts and offsets as `uint32_t`, so layout validation rejects values that do not fit.

<p align="right"><a href="#top">↑ Back to top</a></p>

## Building into memory

`TableBuilder::build_into(memory, capacity)` requires:

```text
memory != NULL
capacity >= required_size()
```

It first zeroes the complete packed-table region.

Then it writes:

1. `t_table_blob_hdr`;
2. ACTION spans and exceptions;
3. GOTO spans and valid entries;
4. rule metadata.

The build function checks that the number of entries written matches the counts computed by `compute_layout()`.

<p align="right"><a href="#top">↑ Back to top</a></p>

## Arithmetic checks

Helpers such as:

```cpp
checked_u32()
checked_u64_mul()
checked_u64_add()
```

prevent silent layout overflow.

Despite the helper names, the arithmetic type is the host `size_t`; the important property is explicit overflow detection before the result is written into the 32-bit packed format.

<p align="right"><a href="#top">↑ Back to top</a></p>

## `TableView`

`TableView` is a non-owning read-only view over the packed table.

It stores:

```cpp
const uint8_t          *m_base_;
const t_table_blob_hdr *m_hdr_;
size_t                  m_capacity_;
```

Section pointers are derived from:

```text
m_base_ + header offset
```

through helpers such as:

```cpp
action_spans_ptr()
action_entries_ptr()
goto_spans_ptr()
goto_entries_ptr()
rule_entries_ptr()
```

No relocation of internal pointers is stored inside the blob.

<p align="right"><a href="#top">↑ Back to top</a></p>

## Validation

`TableView` validates the complete packed table when constructed.

### Header and total size

It checks:

- blob is at least large enough for the header;
- declared `total_size` is valid;
- declared size does not exceed mapped capacity.

### Alignment

Section offsets must satisfy the expected 4-byte alignment.

### Section bounds

For each section, validation checks:

```text
offset
count
element size
```

against `total_size`.

### Non-overlap

The expected section ordering is enforced:

```text
header
< action spans
< action entries
< goto spans
< goto entries
< rule entries
<= total_size
```

### ACTION spans

For every state:

- span offset/count must fit inside `action_count`;
- default action must decode to a valid action;
- every exception terminal must be `<= CFG_END`;
- exception actions must be valid;
- exception terminals must be strictly increasing.

### Packed action targets

`validate_packed_action()` checks:

```text
SHIFT  target < state_count
REDUCE rule   < rule_count
ACCEPT value == 0
ERROR  value == 0
```

### GOTO spans

For every state:

- span offset/count must fit inside `goto_count`;
- compact non-terminal index must be in range;
- target state must be in range;
- keys must be strictly increasing.

### Rule entries

For every rule:

- `lhs` must be a valid non-terminal symbol;
- its compact index must be within `non_terminal_count`;
- `semantic_id` must either be `PARSING_NO_SEMANTIC` or less than `semantic_count`.

Once this validation has succeeded and the image remains immutable, runtime lookups can rely on these invariants.

<p align="right"><a href="#top">↑ Back to top</a></p>

## Packed table versus complete `Rug` image

The packed table described above is only the first part of the finalized `Rug`.

`Rug::finalize_into()` creates:

```text
+------------------------------+
| packed parsing table         |
+------------------------------+
| pointer-alignment padding    |
+------------------------------+
| t_reduce_callback[]          |
+------------------------------+
```

The callback table is outside `t_table_blob_hdr::total_size`.

`TableView` sees only the packed parsing table.

`SemanticRuntime` sees only the callback array.

`Rug` owns or references the complete containing image.

<p align="right"><a href="#top">↑ Back to top</a></p>

## Memory/performance trade-off

The packed representation deliberately trades some lookup work for lower table size.

### Dense `TableIR`

Pros:

```text
simple construction
direct indexing
easy conflict detection
```

Cons:

```text
257 ACTION entries per state
one GOTO slot per non-terminal per state
many repeated ERROR/REDUCE values
many invalid GOTO slots
```

### Packed runtime table

Pros:

```text
one default ACTION per state
only ACTION exceptions stored
only valid GOTOs stored
32-bit action encoding
no RHS symbol arrays needed at runtime
single contiguous table region
```

Cost:

```text
binary search in each state's ACTION exception slice
binary search in each state's GOTO slice
```

RUG performs the expensive packing work at grammar-generation time, not while application input is being parsed.

<p align="right"><a href="#top">↑ Back to top</a></p>

## Current portability status

The current blob should be treated as a runtime representation for the same build/runtime environment, not as a stable on-disk interchange format.

In particular, the current header does not contain:

```text
magic
format version
endianness marker
ABI marker
```

and `TableView` directly interprets typed structures from the blob.

If persistent cross-version serialization becomes a goal, those properties should be defined explicitly before declaring the packed table a stable binary format.
