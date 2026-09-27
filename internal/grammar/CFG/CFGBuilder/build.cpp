// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#include "CFGBuilder.hpp"

void CFGBuilder::build_base( void )
{
	create_rule_symbols();
	set_start_symbol();
	lower_rules();

	m_cfg_.group_productions();

	mm_rule_symbols_.clear();
}

/*
	create the map of lhs 
*/
void CFGBuilder::create_rule_symbols( void )
{
	std::string										name;
	t_id_symbol										id;
	std::map<std::string, t_id_symbol>::iterator	it;

	for (size_t i = 0; i < m_ir_.rule_count(); i++)
	{
		const t_abnf_rule& rule = m_ir_.rule(i);

		// search case insensititv name in map
		name = canonical_name(rule.name);
		it = mm_rule_symbols_.find(name);

		// if lhs name is already on the map the we dont need to manage it
		if (it != mm_rule_symbols_.end())
			continue ;

		// here we create a new unique symbol id
		id = m_cfg_.add_non_terminal();
		// and assign it to the name in the map
		mm_rule_symbols_[name] = id;

		m_cfg_.register_non_terminal_name(name, id);

		m_cfg_.set_source_span(id, rule.name);

		if (mb_debug_mode_)
			m_cfg_.set_debug_name(id, source_string(rule.name));
	}
}

void CFGBuilder::set_start_symbol( void )
{
	std::string											name;
	std::map<std::string, t_id_symbol>::const_iterator	it;

	name = canonical_name(m_start_name_);
	it = mm_rule_symbols_.find(name);

	if (it == mm_rule_symbols_.end())
		throw std::logic_error("CFGBuilder::set_start_symbol: start rule does not exist");

	m_cfg_.set_start_symbol(it->second);
}

void CFGBuilder::lower_rules( void )
{
	t_id_symbol	lhs;

	for (size_t i = 0; i < m_ir_.rule_count(); i++)
	{
		const t_abnf_rule& rule = m_ir_.rule(i);
		lhs = find_rule_symbol(rule.name);
		lower_as_productions(lhs, rule.root_id);
	}
}
