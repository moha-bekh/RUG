// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#include "CFG.hpp"

void CFG::set_debug_name( t_id_symbol id, const std::string& name )
{
	t_debug_name debug_name;

	for (size_t i = 0; i < mv_debug_names_.size(); i++)
	{
		if (mv_debug_names_[i].id == id)
		{
			mv_debug_names_[i].name = name;
			return ;
		}
	}

	debug_name.id = id;
	debug_name.name = name;

	mv_debug_names_.push_back(debug_name);
}

bool CFG::has_debug_name( t_id_symbol id ) const
{
	for (size_t i = 0; i < mv_debug_names_.size(); i++)
	{
		if (mv_debug_names_[i].id == id)
			return (true);
	}

	return (false);
}

const std::string& CFG::debug_name( t_id_symbol id ) const
{
	for (size_t i = 0; i < mv_debug_names_.size(); i++)
	{
		if (mv_debug_names_[i].id == id)
			return (mv_debug_names_[i].name);
	}

	throw std::logic_error("CFG::debug_name: symbol has no debug name");
}