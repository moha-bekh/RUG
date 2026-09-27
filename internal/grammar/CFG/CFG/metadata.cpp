// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#include "CFG.hpp"

void 					CFG::set_source_span( t_id_symbol id, const t_source_span& span )
{
	size_t index = id - CFG_FIRST_NON_TERMINAL;

	mv_source_spans_[index] = span;
	mv_has_source_span_[index] = true;
}

bool 					CFG::has_source_span( t_id_symbol id ) const
{
	size_t index = id - CFG_FIRST_NON_TERMINAL;

	return (mv_has_source_span_[index]);
}

const t_source_span&	CFG::source_span( t_id_symbol id ) const
{
	size_t index = id - CFG_FIRST_NON_TERMINAL;

	return (mv_source_spans_[index]);
}