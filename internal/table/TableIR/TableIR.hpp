// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <vector>
#include <cstddef>
#include <stdexcept>

#include "../../grammar/CFG/CFG/CFG.hpp"
#include "../../automaton/LRGraph/LRGraph.hpp"


enum e_action_type
{
	ACTION_ERROR,
	ACTION_SHIFT,
	ACTION_REDUCE,
	ACTION_ACCEPT	
};

typedef struct		s_action
{
	e_action_type	type;
	size_t			value;
}					t_action;

static const t_id_state	PARSING_INVALID_GOTO	= (size_t)-1;

class TableIR
{
	private:
		std::vector< std::vector< t_action > >		mvv_actions_;
		std::vector< std::vector< t_id_state > >	mvv_gotos_;

	public:
		// OCF
		TableIR( size_t state_count, size_t non_terminal_count );
		~TableIR( void );

		// setters
		void			set_action( t_id_state state, t_id_symbol terminal, const t_action& action );
		void			set_goto( t_id_state state, t_id_symbol non_terminal, t_id_state target_state );

		// getters
		const t_action&	action( t_id_state state, t_id_symbol terminal ) const;
		t_id_state		goto_state( t_id_state state, t_id_symbol non_terminal ) const;

		size_t			state_count( void ) const;
		size_t			non_terminal_count( void ) const;

	private:
		size_t			non_terminal_index( t_id_symbol id ) const;

	private:
		// forbidden OCF
		TableIR( void );
		TableIR( const TableIR& other );
		TableIR& operator= ( const TableIR& other );

};