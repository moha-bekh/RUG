// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#include "LALRBuilder.hpp"

void		LALRBuilder::build( void )
{
	// init mem
	mv_lr_to_lalr_.clear();
	mv_lr_to_lalr_.resize(m_lr_.state_count(), LR_INVALID_STATE);

	build_states();
	build_transitions();
}

t_id_state	LALRBuilder::find_same_core( const LRState& state ) const
{
	for (size_t i = 0; i < m_lalr_.state_count(); i++)
	{
		if (m_lalr_.state(i).same_core_as(state))
			return ((t_id_state)i);
	}
	
	return (LR_INVALID_STATE);
}

void		LALRBuilder::merge_state( LRState& dst, const LRState& src )
{
	for (size_t i = 0; i < src.item_count(); i++)
	{
		const t_lr_item& item = src.item(i);
		dst.merge_item(item.core, item.lookaheads);
	}
}

void 		LALRBuilder::build_states( void )
{
	t_id_state	lalr_id;

	for (size_t i = 0; i < m_lr_.state_count(); i++)
	{
		const LRState& lr_state = m_lr_.state(i);
		lalr_id = find_same_core(lr_state);

		// if not found we create a new state
		if (lalr_id == LR_INVALID_STATE)
			lalr_id = m_lalr_.add_state(lr_state);
		else // else we merge with the corresponding state
			merge_state(m_lalr_.state_mut(lalr_id), lr_state);

		mv_lr_to_lalr_[i] = lalr_id;
	}
}

void	LALRBuilder::build_transitions( void )
{
	t_id_state	from;
	t_id_state	to;
	t_id_state	existing;

	for (size_t i = 0; i < m_lr_.transition_count(); i++)
	{
		const t_lr_transition& transition = m_lr_.transition(i);
		from = mv_lr_to_lalr_[transition.from];
		to = mv_lr_to_lalr_[transition.to];
		existing = find_transition(from, transition.symbol);

		// only if transition is not found we create it
		if (existing == LR_INVALID_STATE)
			m_lalr_.add_transition(from, transition.symbol, to);
	}
}

t_id_state	LALRBuilder::find_transition( t_id_state from, t_id_symbol symbol ) const
{
	for (size_t i = 0; i < m_lalr_.transition_count(); i++)
	{
		const t_lr_transition& transition = m_lalr_.transition(i);

		if (transition.from == from && transition.symbol == symbol)
			return (transition.to);
	}

	return (LR_INVALID_STATE);
}
