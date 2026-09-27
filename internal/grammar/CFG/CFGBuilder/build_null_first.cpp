// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#include "CFGBuilder.hpp"

void	CFGBuilder::build_null_first( void )
{
	m_cfg_.build_properties_storage();

	compute_nullable();
	compute_first();
}

void	CFGBuilder::compute_nullable( void )
{
	t_id_symbol	lhs;
	size_t		count;
	bool		changed = true;

	while (changed)
	{
		changed = false;

		// as a any new nullable ca  change the precedency
		// we need to re-check the whole graph again at each change
		// so we run an infinite loop unti no chaged happened
		for (size_t i = 0; i < m_cfg_.non_terminal_count(); i++)
		{
			lhs = CFG_FIRST_NON_TERMINAL + i;

			if (m_cfg_.is_nullable(lhs))
				continue ;
			
			count = m_cfg_.rule_count_for_same_lhs(lhs);

			for (size_t j = 0; j < count; j++)
			{
				const t_rule& rule = m_cfg_.rule(m_cfg_.rule_id_for_lhs_at(lhs, j));

				if (rule_is_nullable(rule))
				{
					if (m_cfg_.mark_nullable(lhs))
						changed = true;

					break ;
				}
			}
		}
	}
}

bool	CFGBuilder::rule_is_nullable( const t_rule& rule ) const
{
	t_id_symbol	symbol;

	if (rule.rhs_slice.count == 0)
		return (true);
	
	for (size_t i = 0; i < rule.rhs_slice.count; i++)
	{
		symbol = m_cfg_.rhs_symbol(rule, i);

		if (m_cfg_.is_terminal(symbol))
			return (false);

		if (!m_cfg_.is_nullable(symbol))
			return (false);
	}

	return (true);
}

void	CFGBuilder::compute_first( void )
{
	bool	changed = true;

	while (changed)
	{
		changed = false;

		for (size_t i = 0; i < m_cfg_.rule_count(); i++)
		{
			const t_rule&	rule = m_cfg_.rule(i);

			if (propagate_first_from_rule(rule))
				changed = true;
		}
	}
}

bool	CFGBuilder::propagate_first_from_rule( const t_rule& rule )
{
	t_id_symbol	symbol;
	bool		changed = false;

	for (size_t i = 0; i < rule.rhs_slice.count; i++)
	{
		symbol = m_cfg_.rhs_symbol(rule, i);

		if (m_cfg_.is_terminal(symbol))
		{
			if (m_cfg_.add_first(rule.lhs, (uint8_t)symbol))
				changed = true;

			break ;
		}

		if (m_cfg_.merge_first(rule.lhs, m_cfg_.first_set(symbol)))
			changed = true;
		
		if (!m_cfg_.is_nullable(symbol))
			break ;
		}

		return (changed);
}

