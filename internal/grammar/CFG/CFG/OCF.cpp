// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#include "CFG.hpp"

CFG::CFG( void )
:	mv_rules_(),
	mv_rhs_(),
	mv_sorted_rule_ids_(),
	mv_same_lhs_slices_(),
	mv_nullable_(),
	mv_first_(),
	mv_source_spans_(),
	mv_has_source_span_(),
	mv_debug_names_(),
	m_non_terminal_count_(0),
	m_start_symbol_(CFG_INVALID_SYMBOL),
	mm_named_non_terminals_()
{	}

CFG::~CFG( void )
{	}
