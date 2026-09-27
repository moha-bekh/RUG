// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#include "CFG.hpp"

t_id_symbol	CFG::terminal( uint8_t byte ) const
{	return ((t_id_symbol)byte);	}

t_id_symbol	CFG::end_symbol( void ) const
{	return (CFG_END);	}

bool CFG::is_terminal( t_id_symbol id ) const
{	return (id <= CFG_END);	}

bool CFG::is_non_terminal( t_id_symbol id ) const
{	return (id >= CFG_FIRST_NON_TERMINAL && id < CFG_FIRST_NON_TERMINAL + m_non_terminal_count_);	}

bool CFG::is_end( t_id_symbol id ) const
{	return (id == CFG_END);	}

bool CFG::is_valid_symbol( t_id_symbol id ) const
{	return (is_terminal(id) || is_non_terminal(id));	}
