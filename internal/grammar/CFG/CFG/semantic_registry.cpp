// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#include "CFG.hpp"

void	CFG::register_non_terminal_name( const std::string& name, t_id_symbol id )
{
	std::pair< std::map<std::string, t_id_symbol>::iterator, bool> result;

	result = mm_named_non_terminals_.insert(std::make_pair(name, id));
}


bool	CFG::find_non_terminal( const std::string& name, t_id_symbol& id ) const
{
	std::map<std::string, t_id_symbol>::const_iterator it;

	it = mm_named_non_terminals_.find(name);

	if (it == mm_named_non_terminals_.end())
		return (false);

	id = it->second;

	return (true);
}