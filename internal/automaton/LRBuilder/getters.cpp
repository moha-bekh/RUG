// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#include "LRBuilder.hpp"

size_t		LRBuilder::item_rhs_count( const t_lr_item_core& core ) const
{
	if (core.rule == LR_AUGMENTED_RULE)
		return (1);
	
	return (m_cfg_.rule(core.rule).rhs_slice.count);
}

t_id_symbol	LRBuilder::item_rhs_symbol( const t_lr_item_core& core, size_t index ) const
{
	if (core.rule == LR_AUGMENTED_RULE)
		return (m_cfg_.start_symbol());
	
	const t_rule& rule = m_cfg_.rule(core.rule);
	return (m_cfg_.rhs_symbol(rule, index));
}

bool		LRBuilder::next_symbol( const t_lr_item& item, t_id_symbol& symbol ) const
{
	if (item.core.dot >= item_rhs_count(item.core))
		return (false);

	symbol = item_rhs_symbol(item.core, item.core.dot);
	
	return (true);
}

t_id_state	LRBuilder::find_state( const LRState& state ) const
{
	for (size_t i = 0; i < m_lr_graph_.state_count(); i++)
	{
		if (m_lr_graph_.state(i).same_as(state))
			return ((t_id_state)i);
	}

	return (LR_INVALID_STATE);
}

void		LRBuilder::collect_next_symbols( const LRState& state, std::vector<t_id_symbol>& out) const
{
	t_id_symbol	symbol;
	bool		found;

	out.clear();

	for (size_t i = 0; i < state.item_count(); i++)
	{
		if (!next_symbol(state.item(i), symbol))
			continue ;
		
		found = false;

		for (size_t j = 0; j < out.size(); j++)
		{
			if (out[j] == symbol)
			{
				found = true;
				break ;
			}
		}

		if (!found)
			out.push_back(symbol);
	}
}