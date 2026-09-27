// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#include "CFGBuilder.hpp"

void 			CFGBuilder::build( void )
{
	build_base();
	validate_cfg();
	if (m_diag_.has_errors())
		return ;
	build_null_first();
}

size_t			CFGBuilder::non_terminal_index( t_id_symbol id ) const
{	return (id - CFG_FIRST_NON_TERMINAL);	}