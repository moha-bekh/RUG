// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#include "CFGBuilder.hpp"

t_id_symbol CFGBuilder::find_rule_symbol( const t_source_span& span ) const
{
	std::string											name;
	std::map<std::string, t_id_symbol>::const_iterator	it;

	name = canonical_name(span);
	it = mm_rule_symbols_.find(name);

	return (it->second);
}

t_id_symbol CFGBuilder::new_synthetic_symbol( const std::string& prefix )
{
	t_id_symbol	id;

	id = m_cfg_.add_non_terminal();

	if (mb_debug_mode_)
	{
		std::ostringstream name;

		name << "$" << prefix << m_synthetic_count_;
		m_cfg_.set_debug_name(id, name.str());
	}

	m_synthetic_count_++;

	return (id);
}

std::string CFGBuilder::canonical_name( const t_source_span& span ) const
{
	std::string name(m_src_.data() + span.offset, span.len);

	for (size_t i = 0; i < name.size(); i++)
	{
		if (name[i] >= 'A' && name[i] <= 'Z')
			name[i] = name[i] - 'A' + 'a';
	}

	return (name);
}

std::string CFGBuilder::canonical_name( const std::string& input ) const
{
	std::string name = input;

	for (size_t i = 0; i < name.size(); i++)
	{
		if (name[i] >= 'A' && name[i] <= 'Z')
			name[i] = name[i] - 'A' + 'a';
	}

	return (name);
}

std::string CFGBuilder::source_string( const t_source_span& span ) const
{	return (std::string(m_src_.data() + span.offset, span.len));	}