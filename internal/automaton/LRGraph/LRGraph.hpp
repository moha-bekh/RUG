// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../../grammar/CFG/CFG/CFG.hpp"
#include "LRTypes.hpp"
#include "LRState.hpp"

class LRGraph
{
	private:
		std::vector<LRState>			mv_states_;
		std::vector<t_lr_transition>	mv_transitions_;
	
	public:
		LRGraph( void );
		~LRGraph( void );

		// getters
		size_t					state_count( void ) const;
		size_t					transition_count( void ) const;
		LRState&				state_mut( t_id_state id );
		const LRState&			state( t_id_state id ) const;
		const t_lr_transition&	transition( size_t index ) const;

		// setters
		t_id_state				add_state( const LRState& state );
		void					add_transition( const t_lr_transition& transition );
		void					add_transition( t_id_state from, t_id_symbol symbol, t_id_state to );

		void					print_meta_data( void );
	private:
		// forbidden OCF
		LRGraph( const LRGraph& other );
		LRGraph& operator= ( const LRGraph& other );
	
};
