// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#include "TableIR.hpp"

TableIR::TableIR( size_t state_count, size_t non_terminal_count )
:	mvv_actions_(),
	mvv_gotos_()
{
	// set default action type
	t_action	error;

	error.type = ACTION_ERROR;
	error.value = 0;

	mvv_actions_.resize(state_count);
	// fill all actions with default error type
	for (size_t i = 0; i < state_count; i++)
		mvv_actions_[i].resize(CFG_END + 1, error);

	mvv_gotos_.resize(state_count);
	// fill all gotos with default error type
	for (size_t i = 0; i < state_count; i++)
		mvv_gotos_[i].resize(non_terminal_count, PARSING_INVALID_GOTO);
}

TableIR::~TableIR( void )
{	}