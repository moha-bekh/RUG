// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#include "CFG.hpp"

void	CFG::build_properties_storage( void )
{
	mv_nullable_.assign(m_non_terminal_count_, false);

	mv_first_.clear();
	mv_first_.resize(m_non_terminal_count_);
}

bool	CFG::mark_nullable( t_id_symbol non_terminal_id )
{
	size_t index = non_terminal_id - CFG_FIRST_NON_TERMINAL;

	if (mv_nullable_[index])
		return (false);

	mv_nullable_[index] = true;

	return (true);
}

bool	CFG::add_first( t_id_symbol non_terminal_id, uint8_t terminal )
{	return (mv_first_[non_terminal_id - CFG_FIRST_NON_TERMINAL].add(terminal));	}

bool	CFG::merge_first( t_id_symbol non_terminal_id, const TerminalBitSet& set )
{	return (mv_first_[non_terminal_id - CFG_FIRST_NON_TERMINAL].merge(set));	}