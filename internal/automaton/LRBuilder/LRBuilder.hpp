// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../LRGraph/LRGraph.hpp"
#include "../../grammar/CFG/CFG/CFG.hpp"


class LRBuilder
{
	private:
		const CFG&						m_cfg_;
		LRGraph&						m_lr_graph_;

	public:
		LRBuilder( const CFG& cfg, LRGraph& lr_graph );
		~LRBuilder( void );

		void		build( void );

	private:
		// build
		void		closure( LRState& state );
		void		build_closure_lookaheads( const t_lr_item& parent, LookaheadSet& out ) const;
		LRState		compute_goto( const LRState& state, t_id_symbol);

		// getters
		size_t		item_rhs_count( const t_lr_item_core& core ) const;
		t_id_symbol	item_rhs_symbol( const t_lr_item_core& core, size_t index ) const;
		bool		next_symbol( const t_lr_item& item, t_id_symbol& symbol ) const;
		t_id_state	find_state( const LRState& state ) const;
		void		collect_next_symbols( const LRState& state, std::vector<t_id_symbol>& out) const;
	private:
		// forbidden OCF
		LRBuilder( void );
		LRBuilder( const LRBuilder& other );
		LRBuilder& operator= ( const LRBuilder& other );
};