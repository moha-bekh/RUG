// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#include "Session.hpp"

void	Session::reset_session( void )
{
	m_engine_.reset();
	m_span_size_ = 0;

	m_parse_error_.has_error = false;
	m_parse_error_.offset = 0;
	m_parse_error_.state = 0;
	m_parse_error_.terminal = 0;
	m_parse_error_.span.begin = 0;
	m_parse_error_.span.end = 0;

	m_semantic_error_.has_error = false;
	m_semantic_error_.code = SEMANTIC_OK;
	m_semantic_error_.event.rule_id = 0;
	m_semantic_error_.event.lhs = CFG_INVALID_SYMBOL;
	m_semantic_error_.event.rhs_count = 0;
	m_semantic_error_.event.span.begin = 0;
	m_semantic_error_.event.span.end = 0;
}

e_parse_result			Session::push_terminal( t_id_symbol terminal, const t_parse_span& span )
{
	t_engine_result	res;
	t_parse_span	reduced_span;

	while (true)
	{
		res = m_engine_.step(terminal);

		if (res.type == ENGINE_REDUCE)
		{
			reduced_span = apply_reduce_span(res.value, span.begin);
			if (dispatch_reduce(res.value, reduced_span) != SEMANTIC_OK)
				return (PARSE_SEMANTIC_ERROR);

			continue ;
		}

		if (res.type == ENGINE_SHIFT)
		{
			// consume terminal
			push_shift_span(span);
			return (PARSE_CONSUMED);
		}

		if (res.type == ENGINE_ACCEPT)
			return (PARSE_ACCEPTED);

		if (res.type == ENGINE_ERROR)
		{
			// set error struct
			m_parse_error_.has_error = true;
			m_parse_error_.state = m_engine_.top_state();
			m_parse_error_.terminal = terminal;
			m_parse_error_.offset = span.begin;
			m_parse_error_.span = span;

			return (PARSE_ERROR);
		}

		// invariant
		return (PARSE_STACK_OVERFLOW);
	}
}

e_parse_result			Session::finish( size_t end_position )
{
	t_engine_result	res;
	t_parse_span	reduced_span;

	while (true)
	{
		res = m_engine_.step(CFG_END);

		if (res.type == ENGINE_REDUCE)
		{
			reduced_span = apply_reduce_span(res.value, end_position);
			if (dispatch_reduce(res.value, reduced_span) != SEMANTIC_OK)
				return (PARSE_SEMANTIC_ERROR);

			continue ;
		}

		if (res.type == ENGINE_ACCEPT)
			return (PARSE_ACCEPTED);

		if (res.type == ENGINE_ERROR)
		{
			// set error struct
			m_parse_error_.has_error = true;
			m_parse_error_.state = m_engine_.top_state();
			m_parse_error_.terminal = CFG_END;
			m_parse_error_.offset = end_position;
			m_parse_error_.span.begin = end_position;
			m_parse_error_.span.end = end_position;

			return (PARSE_ERROR);
		}

		// default invariant
		return (PARSE_STACK_OVERFLOW);
	}
}