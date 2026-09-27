// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <vector>

#include "../LRGraph/LRGraph.hpp"

class LALRBuilder
{
	private:
		// borrowed
		const LRGraph&				m_lr_;
		LRGraph&					m_lalr_;

		// owned
		std::vector<t_id_state>		mv_lr_to_lalr_;

	public:
		LALRBuilder( const LRGraph& lr, LRGraph& lalr );
		~LALRBuilder( void );

		void build( void );
	
	private:
		// search an already create LALR state
		// having the same LR(0) core
		t_id_state	find_same_core( const LRState& state ) const;

		// merge all lookaheads of src
		// into the corresponing LR(0) cores ind dst
		void		merge_state( LRState& dst, const LRState& src );

		// first pass
		// create/merge LALR States and fill mv_lr_to_lalr_
		void		build_states( void );

		// second pass:
		// remap lr transitions through mv_lr_to_lalr_
		void		build_transitions( void );

		// search an already create LALR transition
		// if a transition is found we return it s id,
		// if not we return LR_INVALID_STATE
		// and the we can create it as it doesnt have duplicates
		t_id_state	find_transition( t_id_state from, t_id_symbol symbol ) const;

	private:
		// forbiden OCF
		LALRBuilder( void );
		LALRBuilder(const LALRBuilder& other);
		LALRBuilder& operator=(const LALRBuilder& other);
};