// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#include "LRGraph.hpp"

LRGraph::LRGraph( void )
:	mv_states_(),
	mv_transitions_()
{	}

LRGraph::~LRGraph( void )
{	}

size_t					LRGraph::state_count( void ) const
{	return (mv_states_.size());	}

size_t					LRGraph::transition_count( void ) const
{	return (mv_transitions_.size());	}

LRState&				LRGraph::state_mut( t_id_state id )
{	return (mv_states_[id]);	}

const LRState&			LRGraph::state( t_id_state id ) const
{	return (mv_states_[id]);	}

const t_lr_transition&	LRGraph::transition( size_t index ) const
{	return (mv_transitions_[index]);	}


t_id_state				LRGraph::add_state( const LRState& state )
{
	mv_states_.push_back(state);
	return (mv_states_.size() - 1);
}

void					LRGraph::add_transition( const t_lr_transition& transition )
{	mv_transitions_.push_back(transition);	}

void					LRGraph::add_transition( t_id_state from, t_id_symbol symbol, t_id_state to )
{
	t_lr_transition	t;

	t.from = from;
	t.symbol = symbol;
	t.to = to;

	mv_transitions_.push_back(t);
}
