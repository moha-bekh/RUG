# Architecture

This document describes the current RUG implementation from ABNF source to runtime parsing.

It intentionally uses the concrete class and type names from the codebase so that each concept can be located directly in the implementation.

## Table of contents

- [Overview](#overview)
- [1. Source and ABNF frontend](#1-source-and-abnf-frontend)
  - [`SourceFile`](#sourcefile)
  - [`Lexer`](#lexer)
  - [`ABNFParser`](#abnfparser)
- [2. `ABNFIR`](#2-abnfir)
- [3. ABNF validation](#3-abnf-validation)
- [4. ABNF to `CFG`](#4-abnf-to-cfg)
  - [Symbol space](#symbol-space)
  - [Rules and flattened RHS storage](#rules-and-flattened-rhs-storage)
  - [Source rule symbols](#source-rule-symbols)
  - [Lowering productions and sequences](#lowering-productions-and-sequences)
  - [Character values](#character-values)
  - [Numeric ranges](#numeric-ranges)
  - [Repetition lowering](#repetition-lowering)
- [5. CFG organization and validation](#5-cfg-organization-and-validation)
  - [Productive and reachable non-terminals](#productive-and-reachable-non-terminals)
- [6. `nullable` and `FIRST`](#6-nullable-and-first)
  - [`TerminalBitSet`](#terminalbitset)
  - [Nullable](#nullable)
  - [FIRST](#first)
- [7. Canonical LR(1)](#7-canonical-lr1)
  - [LR item representation](#lr-item-representation)
  - [Augmented start item](#augmented-start-item)
  - [Closure](#closure)
  - [GOTO](#goto)
  - [State identity](#state-identity)
- [8. LR(1) to LALR(1)](#8-lr1-to-lalr1)
  - [Same LR(0) core](#same-lr0-core)
  - [LR-to-LALR map](#lr-to-lalr-map)
  - [Transition remapping](#transition-remapping)
- [9. `TableIR`](#9-tableir)
  - [ACTION](#action)
  - [GOTO](#goto-1)
  - [Conflict detection](#conflict-detection)
- [10. Semantic binding preparation](#10-semantic-binding-preparation)
- [11. Packing](#11-packing)
- [12. Final `Rug` image](#12-final-rug-image)
- [13. Runtime parsing](#13-runtime-parsing)


## Overview

The build pipeline is:

```text
SourceFile
    │
    ▼
Lexer
    │
    ▼
t_token_buffer
    │
    ▼
ABNFParser
    │
    ▼
ABNFIR
    │
    ▼
ABNFValidator
    │
    ▼
CFGBuilder
    │
    ▼
CFG
    │
    ├── productive / reachable validation
    └── nullable / FIRST
    │
    ▼
LRBuilder
    │
    ▼
canonical LR(1) LRGraph
    │
    ▼
LALRBuilder
    │
    ▼
LALR(1) LRGraph
    │
    ▼
TableIRBuilder
    │
    ▼
TableIR
    │
    ├── ACTION[state][terminal]
    └── GOTO[state][non-terminal]
    │
    ▼
SemanticBindings
    │
    ▼
TableBuilder
    │
    ▼
packed parsing table
    │
    ▼
Rug runtime image
    │
    ├── TableView
    └── SemanticRuntime
    │
    ▼
RugSession
    │
    ▼
Session
    │
    ▼
Engine
```

The important boundary is the transition from build-time structures to the finalized `Rug` image.

`ABNFIR`, `CFG`, both `LRGraph` objects and `TableIR` exist only while the grammar is compiled. After `Rug::finalize_into()` succeeds, those structures are destroyed and runtime parsing uses only the packed table, callback table, and session workspace.

<p align="right"><a href="#top">↑ Back to top</a></p>

## 1. Source and ABNF frontend

### `SourceFile`

`SourceFile` owns the grammar file contents in one `std::string`.

It also builds a line-start index used by diagnostics:

```cpp
std::string         m_data_;
std::vector<size_t> m_v_ln_idx_;
```

Grammar nodes and tokens do not copy their original text. They refer back to `SourceFile` through `t_source_span`:

```cpp
typedef struct s_source_span
{
    size_t offset;
    size_t len;
    size_t line;
    size_t column;
} t_source_span;
```

This keeps the ABNF intermediate representation compact and allows diagnostics to point back to the original grammar.

### `Lexer`

`Lexer` tokenizes the ABNF source itself.

This lexer belongs to the **grammar compiler**. It is unrelated to application input parsing: the generated RUG runtime consumes application bytes directly.

The token stream is a:

```cpp
typedef std::vector<t_token> t_token_buffer;
```

where each `t_token` stores an `e_token_type` and a `t_source_span`.

The lexer recognizes, among others:

- rule names and repetition numbers;
- `=`, `=/`, `/`, `*`;
- groups and options;
- plain, `%i` and `%s` character values;
- `%b`, `%d` and `%x` numeric values;
- grammar whitespace and newlines;
- comments;
- `prose-val`, which is tokenized but later rejected by `ABNFValidator`.

### `ABNFParser`

`ABNFParser` is a recursive-descent parser following the ABNF expression hierarchy:

```text
rule
 └── alternation
      └── concatenation
           └── repetition
                └── element
                     ├── rulename
                     ├── char-val
                     ├── num-val
                     ├── group
                     ├── option
                     └── prose-val
```

The main functions mirror that hierarchy:

```cpp
parse_rule()
parse_alternation()
parse_concatenation()
parse_repetition()
parse_element()
```

The parser does not immediately produce a context-free grammar. It first builds `ABNFIR`.

<p align="right"><a href="#top">↑ Back to top</a></p>

## 2. `ABNFIR`

`ABNFIR` is a flattened intermediate representation of the source ABNF.

An ABNF node is identified by:

```cpp
typedef size_t t_abnf_node_id;
```

and can have one of these types:

```cpp
ABNF_REF
ABNF_CHAR_VAL
ABNF_NUM_VAL
ABNF_PROSE_VAL
ABNF_ALT
ABNF_CONCAT
ABNF_REPEAT
ABNF_OPTION
```

The node objects themselves are stored in:

```cpp
std::vector<t_abnf_node> m_nodes_;
```

Child lists are flattened separately:

```cpp
std::vector<t_abnf_node_id> m_children_ids_;
```

and each `ABNF_ALT` or `ABNF_CONCAT` stores only a slice:

```cpp
typedef struct s_abnf_child
{
    size_t begin;
    size_t count;
} t_abnf_child;
```

Numeric values use the same idea through `m_num_values_`.

This representation avoids allocating a separate C++ object for every tree edge or child vector.

Each source rule is represented by `t_abnf_rule`:

```cpp
typedef struct s_abnf_rule
{
    t_source_span name;
    t_source_span span;
    t_abnf_node_id root_id;
    bool incremental;
} t_abnf_rule;
```

`root_id` points to the root of the rule expression in `m_nodes_`.

<p align="right"><a href="#top">↑ Back to top</a></p>

## 3. ABNF validation

`ABNFValidator` runs before the context-free grammar is built.

It currently verifies:

- duplicate base rule definitions;
- `=/` definitions that have no previous base definition;
- references to undefined rules;
- unsupported `prose-val` nodes;
- numeric terminal values outside the byte range `0..255`.

Rule names are canonicalized to lowercase during validation because ABNF rule names are case-insensitive.

The validator separates syntactic acceptance of the ABNF notation from what the byte-oriented RUG runtime can actually represent.

<p align="right"><a href="#top">↑ Back to top</a></p>

## 4. ABNF to `CFG`

`CFGBuilder` lowers `ABNFIR` into the parser-generator representation `CFG`.

### Symbol space

RUG uses one `t_id_symbol` namespace:

```text
0 .. 255     byte terminals
256          CFG_END
257 ..       non-terminals
```

The constants are:

```cpp
static const t_id_symbol CFG_END = 256;
static const t_id_symbol CFG_FIRST_NON_TERMINAL = 257;
```

A byte terminal is therefore already its own symbol ID.

`CFG_END` is a logical parser end symbol. It is not a byte that can appear in application input.

### Rules and flattened RHS storage

A CFG production is:

```cpp
typedef struct s_rule
{
    t_id_symbol lhs;
    t_slice rhs_slice;
} t_rule;
```

All right-hand-side symbols are stored in one flattened vector:

```cpp
std::vector<t_id_symbol> mv_rhs_;
```

A rule owns only a `(begin, count)` slice into that storage.

An epsilon production is represented by:

```text
rhs_slice.count == 0
```

No explicit epsilon terminal exists.

### Source rule symbols

`CFGBuilder::create_rule_symbols()` creates one non-terminal for each canonical rule name.

The mapping is temporarily held in:

```cpp
std::map<std::string, t_id_symbol> mm_rule_symbols_;
```

Named source non-terminals are also registered in `CFG::mm_named_non_terminals_`, which is later used by `SemanticBindings`.

The first ABNF rule is selected as the start rule by `Rug::get_start_rule()`.

### Lowering productions and sequences

`CFGBuilder` separates two jobs:

```cpp
lower_as_productions(lhs, node_id)
lower_as_sequence(node_id, rhs)
```

`lower_as_productions()` handles expression shapes that can directly become several productions for one left-hand side, especially alternation and option.

For example:

```abnf
value = "a" / "b"
```

becomes conceptually:

```text
value -> "a"
value -> "b"
```

`lower_as_sequence()` handles nodes that must contribute symbols to one RHS.

If an alternation or option appears inside a sequence, `lower_to_symbol()` introduces a synthetic non-terminal so the complex expression can occupy one RHS position.

### Character values

Case-sensitive one-byte character values can map directly to terminals.

Case-insensitive alphabetic bytes use a synthetic non-terminal created by `make_case_insensitive_byte()`:

```text
$ci -> "A"
$ci -> "a"
```

The builder caches these symbols in:

```cpp
t_id_symbol m_cache_ci_symbol_ids_[256];
```

so the same case-insensitive byte does not repeatedly generate equivalent grammar fragments.

### Numeric ranges

A numeric range such as:

```abnf
DIGIT = %x30-39
```

is lowered to a synthetic non-terminal with one production per byte in the range.

The source validator guarantees that numeric terminals are within `0..255`.

### Repetition lowering

ABNF repetition is converted into ordinary CFG productions.

`lower_repeat()` decomposes the source repetition into three helpers:

```cpp
make_exact_repeat()
make_bounded_repeat()
make_unbounded_repeat()
```

The conceptual split is:

```text
2*5 A  -> exact(2) + bounded(3)
3*  A  -> exact(3) + unbounded
*5  A  -> bounded(5)
5   A  -> exact(5)
```

#### Exact repetition

`make_exact_repeat(unit, count, cache)` builds repeated sequences recursively.

Instead of expanding:

```text
1000000 * A
```

into one enormous RHS, the repetition is decomposed approximately by halves.

For example:

```text
exact(4) -> exact(2) exact(2)
exact(2) -> A A
```

A cache indexed by repetition count reuses previously constructed exact fragments within the lowering operation.

#### Bounded repetition

`make_bounded_repeat(unit, maximum, ...)` represents:

```text
0 .. maximum occurrences
```

using recursive decomposition around the highest power of two not greater than `maximum`.

For `maximum == 1`, it acts as an option:

```text
$upto -> ε
$upto -> unit
```

For larger bounds, the grammar is recursively composed from:

- a lower bounded part;
- an exact power-of-two part;
- an optional remaining bounded part.

This avoids creating one production for every possible repetition count in large ranges such as `1*1000000 A`.

#### Unbounded repetition

`make_unbounded_repeat()` creates:

```text
$star -> ε
$star -> $star unit
```

The synthetic symbol names such as `$exact`, `$upto`, `$star`, `$ci`, `$range`, `$alt` and `$option` are debug names. Runtime behavior depends only on symbol IDs.

<p align="right"><a href="#top">↑ Back to top</a></p>

## 5. CFG organization and validation

After lowering, `CFG::group_productions()` groups rule IDs by left-hand side.

It uses count-sort style storage:

```cpp
std::vector<t_id_rule> mv_sorted_rule_ids_;
std::vector<t_slice>   mv_same_lhs_slices_;
```

This gives `LRBuilder` fast access to all productions for a non-terminal through:

```cpp
rule_count_for_same_lhs(lhs)
rule_id_for_lhs_at(lhs, index)
```

without scanning every rule.

### Productive and reachable non-terminals

Before LR generation, `CFGBuilder::validate_cfg()` computes:

- productive non-terminals;
- reachable non-terminals.

Current validation reports errors for:

- a non-productive start symbol;
- a reachable non-productive non-terminal;
- a source non-terminal unreachable from the start symbol.

Synthetic non-terminals do not have source spans and are skipped when source diagnostics are emitted.

<p align="right"><a href="#top">↑ Back to top</a></p>

## 6. `nullable` and `FIRST`

Canonical LR(1) closure requires grammar properties derived from the CFG.

`CFGBuilder::build_null_first()` allocates and computes:

```cpp
std::vector<bool>           mv_nullable_;
std::vector<TerminalBitSet> mv_first_;
```

### `TerminalBitSet`

Because runtime terminals are exactly the 256 byte values, a terminal set is represented by four 64-bit words:

```cpp
uint64_t m_256_bits_[4];
```

This makes `add()`, `contains()` and `merge()` fixed-size bit operations.

`CFG_END` is deliberately not part of `TerminalBitSet`.

### Nullable

A non-terminal is nullable if at least one of its productions can derive epsilon.

`compute_nullable()` repeatedly scans productions until no new nullable non-terminal is discovered.

### FIRST

`compute_first()` similarly propagates terminal FIRST sets until a fixed point is reached.

`CFG::first_of_rhs(rule, begin, out)` computes FIRST for a suffix of one production.

It returns a boolean:

```text
true  -> the complete suffix is nullable
false -> the suffix cannot disappear completely
```

That return value is important during LR(1) closure.

<p align="right"><a href="#top">↑ Back to top</a></p>

## 7. Canonical LR(1)

`LRBuilder` constructs the canonical LR(1) automaton in an `LRGraph`.

### LR item representation

The LR(0) core is:

```cpp
typedef struct s_lr_item_core
{
    t_id_rule rule;
    size_t dot;
} t_lr_item_core;
```

An LR(1) item adds a lookahead set:

```cpp
typedef struct s_lr_item
{
    t_lr_item_core core;
    LookaheadSet lookaheads;
} t_lr_item;
```

`LookaheadSet` contains:

- a `TerminalBitSet` for byte terminals;
- one separate boolean for `CFG_END`.

This avoids forcing the logical end symbol into the 256-bit byte set.

### Augmented start item

LR construction starts from the synthetic augmented item:

```text
$accept -> . start    { CFG_END }
```

represented using:

```cpp
LR_AUGMENTED_RULE
```

The augmented production is not inserted into the normal `CFG` rule array.

`LRBuilder` handles it explicitly through `item_rhs_count()` and `item_rhs_symbol()`.

### Closure

For an item:

```text
A -> α . B β    {L}
```

where `B` is a non-terminal, closure adds:

```text
B -> . γ
```

for each production of `B`.

The lookahead for those new items is computed by `build_closure_lookaheads()`:

```text
FIRST(β)
```

is always added.

If `β` is nullable, the parent lookahead set `L` is added as well.

In code this is:

```cpp
nullable = m_cfg_.first_of_rhs(rule, parent.core.dot + 1, first);

out.merge(first);

if (nullable)
    out.merge(parent.lookaheads);
```

`LRState::merge_item()` merges lookahead sets when the same `(rule, dot)` core is discovered again.

`closure()` therefore iterates until no item receives any new lookahead.

### GOTO

`compute_goto(state, symbol)`:

1. selects items whose next symbol matches `symbol`;
2. advances their dot by one;
3. preserves their lookaheads;
4. computes closure on the resulting state.

For example:

```text
A -> α . X β
```

becomes:

```text
A -> α X . β
```

before closure is applied.

### State identity

Canonical LR(1) state equality is implemented by:

```cpp
LRState::same_as()
```

Two states are equal only if they contain the same LR(0) cores **and** the same lookahead sets.

`LRBuilder::find_state()` uses this equality before inserting a newly computed state.

The resulting graph contains:

```cpp
std::vector<LRState> mv_states_;
std::vector<t_lr_transition> mv_transitions_;
```

where each transition stores:

```cpp
from
symbol
to
```

<p align="right"><a href="#top">↑ Back to top</a></p>

## 8. LR(1) to LALR(1)

RUG first constructs canonical LR(1), then performs a separate LALR merge.

`LALRBuilder` does not recompute closure from the CFG. It transforms the completed canonical graph.

### Same LR(0) core

`LRState::same_core_as()` ignores lookaheads and compares only `(rule, dot)` pairs.

If two canonical LR(1) states have the same LR(0) core, they are merged into one LALR state.

For example:

```text
LR(1) state A:
    X -> α . β    { a }

LR(1) state B:
    X -> α . β    { b, c }
```

becomes:

```text
LALR state:
    X -> α . β    { a, b, c }
```

`LALRBuilder::merge_state()` performs this lookahead union through `LRState::merge_item()`.

### LR-to-LALR map

The builder stores:

```cpp
std::vector<t_id_state> mv_lr_to_lalr_;
```

For every canonical LR state:

```text
mv_lr_to_lalr_[lr_state] = lalr_state
```

This mapping is built during the state merge pass.

### Transition remapping

A second pass rewrites every canonical transition:

```text
LR from --symbol--> LR to
```

into:

```text
map[from] --symbol--> map[to]
```

Duplicate merged transitions are removed by `find_transition()`.

The output is another `LRGraph`, but now representing the LALR(1) automaton.

<p align="right"><a href="#top">↑ Back to top</a></p>

## 9. `TableIR`

`TableIRBuilder` converts the LALR graph into an explicit parsing table.

`TableIR` deliberately uses a simple dense representation before packing.

### ACTION

For every state:

```text
ACTION[state][terminal]
```

has `CFG_END + 1` entries, covering:

```text
0 .. 255     bytes
256          CFG_END
```

Every entry starts as:

```cpp
ACTION_ERROR
```

A LALR transition on a terminal creates:

```cpp
ACTION_SHIFT
```

with the target state as its value.

A completed ordinary LR item creates:

```cpp
ACTION_REDUCE
```

for every terminal in its lookahead set.

A completed augmented item creates:

```cpp
ACTION_ACCEPT
```

on `CFG_END`.

### GOTO

A LALR transition on a non-terminal creates:

```text
GOTO[state][non-terminal] = target_state
```

The dense `TableIR` stores non-terminal columns by compact index:

```text
non_terminal - CFG_FIRST_NON_TERMINAL
```

### Conflict detection

`TableIR::set_action()` detects conflicting writes.

It rejects:

- shift/reduce conflicts;
- reduce/reduce conflicts;
- other incompatible duplicate actions.

Identical duplicate writes are accepted.

`TableIR::set_goto()` similarly rejects conflicting GOTO targets.

No precedence or associativity conflict-resolution policy is currently applied: a conflicting grammar fails during table generation.

<p align="right"><a href="#top">↑ Back to top</a></p>

## 10. Semantic binding preparation

Semantic actions are attached after `TableIR` has been generated and before the packed table is finalized.

`Rug::bind()` initially stores pending `(name, callback)` pairs in `RugBuildData`.

During `Rug::prepare()`, `SemanticBindings` resolves source non-terminal names through the `CFG` registry.

Each bound non-terminal receives one `semantic_id`.

The callback array is built in parallel:

```cpp
std::vector<t_reduce_callback> mv_callback_table_;
```

When `TableBuilder` emits `t_rule_entry`, every rule whose left-hand side has a binding receives the same `semantic_id`.

This means bindings are associated with non-terminals, not individual CFG productions.

Synthetic CFG non-terminals are internal and are not registered as source binding names.

<p align="right"><a href="#top">↑ Back to top</a></p>

## 11. Packing

`TableBuilder` converts the dense `TableIR` and rule metadata into the final packed parsing-table blob.

At this boundary:

```text
generation-friendly representation
             ↓
runtime-oriented representation
```

ACTION rows are compressed with one default action plus sparse exceptions, GOTO rows store only valid transitions, and parser actions are encoded as `t_packed_action`.

The exact memory layout and lookup algorithms are documented in [`packed-table.md`](packed-table.md).

<p align="right"><a href="#top">↑ Back to top</a></p>

## 12. Final `Rug` image

The packed parsing table is not the complete `Rug` runtime image.

`Rug::finalize_into()` creates:

```text
[ packed parsing table ]
[ alignment padding    ]
[ callback table       ]
```

The callback table is aligned to `sizeof(void*)`.

The finalized image can either be:

- allocated by `Rug::build()` using `mmap()`;
- provided by the caller through `Rug::build_into()`.

`TableView` is placement-constructed in internal aligned storage and points at the packed table.

`SemanticRuntime` is similarly placement-constructed and points at the callback table.

After these views have been created successfully:

```cpp
delete m_build_data_;
m_build_data_ = NULL;
```

All STL-heavy build structures are therefore gone from the runtime representation.

<p align="right"><a href="#top">↑ Back to top</a></p>

## 13. Runtime parsing

`RugSession` owns the mutable state for one parse.

Its internal runtime chain is:

```text
RugSession
    │
    ▼
Session
    │
    ▼
Engine
    │
    ▼
TableView
```

`Engine` knows only about:

- the packed parsing table;
- the parser state stack.

`Session` adds:

- source spans;
- semantic callback dispatch;
- syntax and semantic error state.

`RugSession` adds:

- incremental byte input;
- workspace ownership;
- global input offset;
- the public session API.

The detailed runtime memory layout and parse cycle are documented in [`runtime.md`](runtime.md).
