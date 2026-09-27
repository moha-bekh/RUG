// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#include "TableIRBuilder.hpp"

void	TableIRBuilder::build( void )
{
	build_transitions();
	build_reductions();
}

void	TableIRBuilder::build_transitions( void )
{
	t_action	action;

	for (size_t i = 0; i < m_lalr_.transition_count(); i++)
	{
		const t_lr_transition& trans = m_lalr_.transition(i);

		if (m_cfg_.is_terminal(trans.symbol))
		{
			action.type = ACTION_SHIFT;
			action.value = trans.to;
			m_table_.set_action(trans.from, trans.symbol, action);
		}
		else if (m_cfg_.is_non_terminal(trans.symbol))
			m_table_.set_goto(trans.from, trans.symbol, trans.to);
		else
			throw std::logic_error("ParsingTableBuilder::build_transitions: invalid transition symbol");
	}
}

void	TableIRBuilder::build_reductions( void )
{
	t_action	action;

	for (size_t state_id = 0; state_id < m_lalr_.state_count(); state_id++)
	{
		const LRState& state = m_lalr_.state(state_id);

		for (size_t i = 0; i < state.item_count(); i++)
		{
			const t_lr_item& item = state.item(i);

			// non terminal
			if (!item_is_complete(item))
				continue ;

			// start
			if (item.core.rule == LR_AUGMENTED_RULE)
			{
				action.type = ACTION_ACCEPT;
				action.value = 0;

				m_table_.set_action(state_id, CFG_END, action);

				continue ;
			}

			// bytes terminals 0 - 255
			action.type = ACTION_REDUCE;
			action.value = item.core.rule;
			for (t_id_symbol terminal = 0; terminal < CFG_END; terminal++)
			{
				if (!item.lookaheads.contains(terminal))
					continue ;
				
				m_table_.set_action(state_id, terminal, action);
			}

			// END
			if (item.lookaheads.contains(CFG_END))
				m_table_.set_action(state_id, CFG_END, action);
		}
	}
}

bool	TableIRBuilder::item_is_complete( const t_lr_item& item ) const
{
	if (item.core.rule == LR_AUGMENTED_RULE)
		return (item.core.dot == 1);
	
	const t_rule& rule = m_cfg_.rule(item.core.rule);

	return (item.core.dot == rule.rhs_slice.count);
}
