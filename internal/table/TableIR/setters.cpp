// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#include "TableIR.hpp"

void	TableIR::set_action( t_id_state state, t_id_symbol terminal, const t_action& action )
{
	t_action&	current = mvv_actions_[state][terminal];

	if (current.type != ACTION_ERROR)
	{
		// if same we continue 
		if (current.type == action.type && current.value == action.value)
			return ;

		// all the rest we refuse for conflict avoidance
		if ((current.type == ACTION_SHIFT && action.type == ACTION_REDUCE) || \
			(current.type == ACTION_REDUCE && action.type == ACTION_SHIFT))
			throw std::logic_error("TableIR::set_action: shift/reduce conflict");

		if (current.type == ACTION_REDUCE && action.type == ACTION_REDUCE)
			throw std::logic_error("TableIR::set_action: reduce/reduce conflict");

		throw std::logic_error("TableIR::set_action: conflict found");
	}

	current = action;
}

void	TableIR::set_goto( t_id_state state, t_id_symbol non_terminal, t_id_state target_state )
{
	size_t	index = non_terminal_index(non_terminal);
	t_id_state& cur = mvv_gotos_[state][index];

	if (cur != PARSING_INVALID_GOTO)
	{
		if (cur == target_state)
			return ;
		throw std::logic_error("TableIR::set_goto: conflicting goto");
	}
	cur = target_state;
}

