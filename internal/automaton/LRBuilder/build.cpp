// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#include "LRBuilder.hpp"

void LRBuilder::build( void )
{
	LRState						initial;
	LRState						current;
	LRState						target;

	t_lr_item_core				core;
	LookaheadSet				lookaheads;
	std::vector<t_id_symbol>	symbols;
	t_id_state					target_id;

	/*
		inital LR(1) item:
		$accept -> . start {$END}
	*/
	core.rule = LR_AUGMENTED_RULE;
	core.dot = 0;

	lookaheads.add(CFG_END);
	initial.merge_item(core, lookaheads);

	closure(initial);

	m_lr_graph_.add_state(initial);

	// state_count() grows while new state s are discovered
	for (t_id_state state_id = 0; state_id < m_lr_graph_.state_count(); state_id++)
	{
		// copy no ref / because add_satate can reallocate
		
		current = m_lr_graph_.state(state_id);
		collect_next_symbols(current, symbols);

		for (size_t i = 0; i < symbols.size(); i++)
		{
			target = compute_goto(current, symbols[i]);

			if (target.item_count() == 0)
				continue ;

			target_id = find_state(target);
			if (target_id == LR_INVALID_STATE)
				target_id = m_lr_graph_.add_state(target);

			m_lr_graph_.add_transition(state_id, symbols[i], target_id);
		}
	}
}

/*
		for (size_t i = 0; i < symbols.size(); i++)
		{
			target = compute_goto(
				current,
				symbols[i]
			);

			if (target.item_count() == 0)
				continue ;

			target_id = find_state(target);

			if (target_id == LR_INVALID_STATE)
				target_id = m_out_.add_state(target);

			m_out_.add_transition(
				state_id,
				symbols[i],
				target_id
			);
		}
*/

void LRBuilder::build_closure_lookaheads( const t_lr_item& parent, LookaheadSet& out ) const
{
	TerminalBitSet	first;
	bool			nullable;

	out.clear();

	// LR_AUGMENTED_RULE (size_t)-2
	if (parent.core.rule == LR_AUGMENTED_RULE)
	{
		out.merge(parent.lookaheads);
		return ;
	}

	const t_rule& rule = m_cfg_.rule(parent.core.rule);

	nullable = m_cfg_.first_of_rhs(rule, parent.core.dot + 1, first);

	out.merge(first);

	if (nullable)
		out.merge(parent.lookaheads);
}

/*
	closure will devellop all the possible item of one state
	to do so once we enriched one non-terminal we need to recheck the whole 
	stae for potential new enrichement to create (bool changed is her for that)
*/
void LRBuilder::closure( LRState& state )
{
	t_id_symbol		next;
	LookaheadSet	lookaheads;
	t_lr_item_core	core;
	size_t			count;
	t_lr_item		current;
	bool			changed = true;

	while (changed)
	{
		changed = false;

		for (size_t i = 0; i < state.item_count(); i++)
		{
			// copie dont do reference
			// because merge_item can push_back in vector
			// and invalidate ref
			current = state.item(i);

			// ecompleted item, nmothing after the dot
			if (!next_symbol(current, next))
				continue ;
			
			// cloisure only expands non-terminal 
			if (!m_cfg_.is_non_terminal(next))
				continue ;

			build_closure_lookaheads(current, lookaheads);

			count = m_cfg_.rule_count_for_same_lhs(next);

			for (size_t j = 0; j < count; j++)
			{
				core.rule = m_cfg_.rule_id_for_lhs_at(next, j);
				core.dot = 0;

				if (state.merge_item(core, lookaheads))
					changed = true;
			}
		}
	}
}

/*
	advance dot in item if next match "symbol" passed in param
*/
LRState	LRBuilder::compute_goto( const LRState& state, t_id_symbol symbol )
{
	LRState			out;
	t_id_symbol		next;
	t_lr_item_core	core;

	// here as we compyute the count on the run 
	// i think we dont have to do this cause 
	// the main algorithm we reevaluate any changed graph
	for (size_t i = 0; i < state.item_count(); i++)
	{
		const t_lr_item& item = state.item(i);

		if (!next_symbol(item, next))
			continue ;
		
		if (next != symbol)
			continue ;

		core = item.core;
		core.dot ++;

		out.merge_item(core, item.lookaheads);
	}

	closure(out);

	return (out);
}