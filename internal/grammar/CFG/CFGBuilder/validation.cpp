// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#include "CFGBuilder.hpp"

void	CFGBuilder::validate_cfg( void )
{
	// init mem
	mv_productive_.assign(m_cfg_.non_terminal_count(), false);
	mv_reachable_.assign(m_cfg_.non_terminal_count(), false);

	compute_productive();
	compute_reachable();

	emit_diagnostics();
}

bool	CFGBuilder::rule_is_productive( const t_rule& rule ) const
{
	t_id_symbol	symbol;

	for (size_t i = 0; i < rule.rhs_slice.count; i++)
	{
		symbol = m_cfg_.rhs_symbol(rule, i);

		if (m_cfg_.is_terminal(symbol))
			continue ;

		if (!is_productive(symbol))
			return (false);
	}

	return (true);
}

void	CFGBuilder::compute_productive( void )
{
	t_id_symbol	lhs;
	size_t		rule_count;
	t_id_rule	rule_id;
	bool		changed = true;

	while (changed)
	{
		changed = false;

		for (size_t i = 0; i < m_cfg_.non_terminal_count(); i++)
		{
			lhs = CFG_FIRST_NON_TERMINAL + i;

			if (mv_productive_[i])
				continue ;
			
			rule_count = m_cfg_.rule_count_for_same_lhs(lhs);

			for (size_t j = 0; j < rule_count; j++)
			{
				rule_id = m_cfg_.rule_id_for_lhs_at(lhs, j);

				const t_rule& rule = m_cfg_.rule(rule_id);

				if (rule_is_productive(rule))
				{
					mv_productive_[i] = true;
					changed = true;
					break ;
				}
			}
		}
	}
}

void	CFGBuilder::compute_reachable( void )
{
	t_id_symbol					lhs;
	size_t						rule_count;
	t_id_rule					rule_id;
	t_id_symbol					symbol;
	std::vector<t_id_symbol>	stack;
	const t_id_symbol			start = m_cfg_.start_symbol();

	mv_reachable_[non_terminal_index(start)] = true;
	stack.push_back(start);

	while (!stack.empty())
	{
		lhs = stack.back();
		stack.pop_back();
		rule_count = m_cfg_.rule_count_for_same_lhs(lhs);

		for (size_t i = 0; i < rule_count; i++)
		{
			rule_id = m_cfg_.rule_id_for_lhs_at(lhs, i);
			const t_rule& rule = m_cfg_.rule(rule_id);

			for (size_t j = 0; j < rule.rhs_slice.count; j++)
			{
				symbol = m_cfg_.rhs_symbol(rule, j);
				if (!m_cfg_.is_non_terminal(symbol))
					continue ;
				if (mv_reachable_[non_terminal_index(symbol)])
					continue ;
				
				mv_reachable_[non_terminal_index(symbol)] = true;
				stack.push_back(symbol);
			}
		}
	}
}


bool	CFGBuilder::is_productive( t_id_symbol id ) const
{	return (mv_productive_[non_terminal_index(id)]);	}

bool	CFGBuilder::is_reachable( t_id_symbol id ) const
{	return (mv_reachable_[non_terminal_index(id)]);	}

void	CFGBuilder::emit_diagnostics( void )
{
	t_id_symbol	id;

	for (size_t i = 0; i < m_cfg_.non_terminal_count(); i++)
	{
		id = CFG_FIRST_NON_TERMINAL + i;

		if (!m_cfg_.has_source_span(id))
			continue ;

		if (id == m_cfg_.start_symbol() && !mv_productive_[i])
		{
			m_diag_.error(m_cfg_.source_span(id), "start symbol is non-productive");
			continue ;
		}

		if (mv_reachable_[i] && !mv_productive_[i])
		{
			m_diag_.error(m_cfg_.source_span(id), "reachable non-terminal is non-productive");
			continue ;
		}

		if (!mv_reachable_[i])
		{
			m_diag_.error(m_cfg_.source_span(id), "non-terminal is unreachable from start symbol");
			continue ;
		}
	}
}