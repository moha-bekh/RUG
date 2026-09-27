// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#include "Engine.hpp"

void			Engine::reset( void )
{
	m_stack_size_ = 1;
	m_stack_[0] = 0;
}

t_id_state		Engine::top_state( void ) const
{	return (m_stack_[m_stack_size_ - 1]);	}

size_t			Engine::stack_size( void ) const
{	return (m_stack_size_);	}

t_engine_result	Engine::step( t_id_symbol terminal )
{
	t_action		action;
	
	action = m_table_.action(top_state(), terminal);

	if (action.type == ACTION_SHIFT)
		return (do_shift((t_id_state)action.value));
	if (action.type == ACTION_REDUCE)
		return (do_reduce(action.value));
	
	t_engine_result res;

	res.value = 0;

	if (action.type == ACTION_ACCEPT)
	{
		res.type = ENGINE_ACCEPT;
		return (res);
	}

	// if (action.type == ACTION_ERROR)
	// {
	// 	res.type = ENGINE_ERROR;
	// 	return (res);
	// }

	// a tion type can only be ACTION_ERROR
	res.type = ENGINE_ERROR;
	return (res);
}

/*
	SHIFT

	push a new state on the state stack

	the parser will understand this terminal can be consume 
*/
t_engine_result	Engine::do_shift( t_id_state target )
{
	t_engine_result	res;

	if (m_stack_size_ >= m_stack_capacity_)
	{
		res.type = ENGINE_STACK_OVERFLOW;
		res.value = 0;

		return (res);
	}

	m_stack_[m_stack_size_] = target;
	m_stack_size_++;

	res.type = ENGINE_SHIFT;
	res.value = target;

	return (res);
}

/*
	REDUCE

	pop the number of rhs
	and add next reduction prod on the state stack via GOTO
	and return us the result of this pass
*/
t_engine_result	Engine::do_reduce( size_t rule_id )
{
	const t_rule_entry& rule = m_table_.rule(rule_id);
	t_engine_result		res;

	// compute the stack size after popping all rhs
	// dont modufy stack yet
	size_t reduced_size = m_stack_size_ - (size_t)rule.rhs_count;

	// ceck stack overlow
	if (reduced_size >= m_stack_capacity_)
	{
		res.type = ENGINE_STACK_OVERFLOW;
		res.value = 0;

		return (res);
	}

	// find stack state before reducted section
	t_id_state base_state = m_stack_[reduced_size - 1];

	// goto aftr reduction
	t_id_state tgt = m_table_.goto_state(base_state, (t_id_symbol)rule.lhs);

	// exec reduction
	m_stack_size_ = reduced_size;
	m_stack_[m_stack_size_] = tgt;
	m_stack_size_++;

	res.type = ENGINE_REDUCE;
	res.value = rule_id;

	return (res);
}