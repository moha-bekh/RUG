// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#include "TableIR.hpp"

const t_action&	TableIR::action( t_id_state state, t_id_symbol terminal ) const
{	return (mvv_actions_[state][terminal]);	}

t_id_state		TableIR::goto_state( t_id_state state, t_id_symbol non_terminal ) const
{
	size_t	index = non_terminal_index(non_terminal);

	return (mvv_gotos_[state][index]);
}

size_t			TableIR::state_count( void ) const
{	return (mvv_actions_.size());	}

size_t			TableIR::non_terminal_count( void ) const
{
	if (mvv_gotos_.empty())
		return (0);

	return (mvv_gotos_[0].size());
}

size_t			TableIR::non_terminal_index( t_id_symbol id ) const
{	return (id - CFG_FIRST_NON_TERMINAL);	}