// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#pragma once

#pragma once

#include <stdint.h>
#include <cstddef>
#include <stdexcept>

#include "../TableIR/TableIR.hpp"

/*

PACKED ACTION

31                   2 1 0
+----------+----------+--+
|                     |  |
+----------+----------+--+

value : 30 bits
type  : 2 bits

00 ACTION_ERROR
01 ACTION_SHIFT
10 ACTION_REDUCE
11 ACTION_ACCEPT
*/

typedef uint32_t	t_packed_action;

static const uint32_t PACKED_ACTION_TYPE_BITS = 2;
static const uint32_t PACKED_ACTION_TYPE_MASK = 0x3u;
static const uint32_t PACKED_ACTION_VALUE_MAX = 0x3FFFFFFFu;
static const uint32_t PARSING_NO_SEMANTIC = 0xFFFFFFFFu;

/* UNPACK ACTION HELPERS */

inline t_packed_action	pack_action( e_action_type type, size_t value )
{
	if (value > PACKED_ACTION_VALUE_MAX)
		throw std::logic_error("pack_action: value does not fit in 30 bits");

	return ((uint32_t)value << PACKED_ACTION_TYPE_BITS | (uint32_t)type);
}

inline e_action_type	unpack_action_type( t_packed_action action )
{	return ((e_action_type)(action & PACKED_ACTION_TYPE_MASK));	}

inline uint32_t			unpack_action_value( t_packed_action action )
{	return (action >> PACKED_ACTION_TYPE_BITS);	}

inline t_action			unpack_packed_action( t_packed_action packed )
{
	t_action	action;

	action.type = unpack_action_type(packed);
	action.value = unpack_action_value(packed);

	return (action);
}

/*
ACTION ROWS

default_action is the most common action in the state.
the final span only containes the exceptions to default_action
*/
typedef struct		s_action_span
{
	uint32_t		offset;
	uint32_t		count;
	t_packed_action	default_action;
}					t_action_span;

/*
Generic sparse span
used for GOTO ROWS 
*/
typedef struct	s_blob_span
{
	uint32_t	offset;
	uint32_t	count;
}				t_blob_span;

/*
	ACTION Exception
*/
typedef struct	s_action_entry
{
	uint32_t		terminal;
	t_packed_action	action;
}				t_action_entry;

/*
	Valid GOTO 
	
	non_terminal is the compacti non terminal index
	final CFG symbol == CFG_FIRST_NON_TERMINAL + non_terminal
*/
typedef struct	s_goto_entry
{
	uint32_t	non_terminal;
	uint32_t	target;
}				t_goto_entry;

/*
	Rules metadata
*/
typedef struct	s_rule_entry
{
	uint32_t	lhs;
	uint32_t	rhs_count;
	uint32_t	semantic_id;
}				t_rule_entry;

/*
	Blob Layout

	header
	action_spans[state_count]
	action_entries[action_count]
	goto_spans[state_count]
	goto_entries[goto_count]

	action_count == N Action Exception entries
*/
typedef struct	s_table_blob_hdr
{
	uint32_t	total_size;

	uint32_t	state_count;
	uint32_t	non_terminal_count;
	uint32_t	rule_count;
	uint32_t	semantic_count;

	uint32_t	action_count;
	uint32_t	goto_count;

	uint32_t	action_spans_offset;
	uint32_t	action_entries_offset;

	uint32_t	goto_spans_offset;
	uint32_t	goto_entries_offset;

	uint32_t	rule_entries_offset;
}				t_table_blob_hdr;
