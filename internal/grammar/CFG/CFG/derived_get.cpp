// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#include "CFG.hpp"

const TerminalBitSet&	CFG::first_set( t_id_symbol non_terminal_id ) const
{	return (mv_first_[non_terminal_id - CFG_FIRST_NON_TERMINAL]);	}

bool	CFG::is_nullable( t_id_symbol id ) const
{	return (mv_nullable_[id - CFG_FIRST_NON_TERMINAL]);	}

bool	CFG::first_contains( t_id_symbol non_terminal, t_id_symbol terminal ) const
{	return (first_set(non_terminal).contains((uint8_t)terminal));	}


/*
	if TRUE all the sufix can disapear
	if FALSE the suffix can not produce EPSILON
*/
bool	CFG::first_of_rhs( const t_rule& rule, size_t begin, TerminalBitSet& out ) const
{
	t_id_symbol symbol_id;

	out.clear();

	for (size_t i = begin; i < rule.rhs_slice.count; i++)
	{
		symbol_id  = rhs_symbol(rule, i);

		// real byte term
		if (symbol_id < CFG_END)
		{
			out.add((uint8_t)symbol_id);
			return (false);
		}

		out.merge(first_set(symbol_id));

		// if this non-terminal is not NULLABLE
		// nothing after it can be FIRST
		if (!is_nullable(symbol_id))
			return (false);
	}

	// end reached without encountering any non-nullable
	// includes begin == rhs.count (so end of rhs)
	return (true);
}