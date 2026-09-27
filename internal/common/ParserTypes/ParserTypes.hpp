// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstddef>

#include "../../grammar/CFG/CFG/CFG.hpp"
#include "../../automaton/LRGraph/LRTypes.hpp"

typedef struct	s_parse_span
{
	size_t		begin;
	size_t		end;
}				t_parse_span;


typedef struct		s_parse_error
{
	bool			has_error;

	size_t			offset;

	t_id_state		state;
	t_id_symbol		terminal;

	t_parse_span	span;
}					t_parse_error;


typedef struct		s_reduce_event
{
	size_t			rule_id;

	t_id_symbol		lhs;
	size_t			rhs_count;
	size_t			rhs_begin;
	
	t_parse_span	span;
}					t_reduce_event;


// 0     -> success
// != 0  -> user-defined semantic error

typedef int32_t		t_semantic_result;

static const t_semantic_result SEMANTIC_OK = 0;


typedef struct			s_semantic_error
{
	bool				has_error;
	t_semantic_result	code;
	t_reduce_event		event;
}						t_semantic_error;


typedef t_semantic_result	(*t_reduce_callback)( const t_reduce_event& event, void *user_context );

enum e_parse_result
{
	PARSE_CONSUMED,
	PARSE_ACCEPTED,
	PARSE_ERROR,
	PARSE_STACK_OVERFLOW,
	PARSE_SEMANTIC_ERROR
};