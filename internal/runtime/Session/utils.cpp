// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#include "Session.hpp"

/* private tools */

void			Session::push_shift_span( const t_parse_span& span )
{
	if (m_span_size_ >= m_span_capacity_)
		throw std::logic_error("ParserSession: span stack overflow");
	
	m_span_stack_[m_span_size_] = span;
	m_span_size_++;
}

t_parse_span	Session::apply_reduce_span( size_t rule_id, size_t empty_position )
{
	const t_rule_entry&	rule_entry = m_table_.rule(rule_id);
	size_t				rhs_count = rule_entry.rhs_count;
	t_parse_span		reduced_span;
	size_t				tmp; // used as first

	/*
		handle span for reduction of epsilon rule (no rhs so no span)
		then handle regular reduced span
	*/
	if (rhs_count == 0)
	{
		reduced_span.begin = empty_position;
		reduced_span.end = empty_position;
	}
	else
	{
		// get stack idx of first element of the reduced rule
		tmp = m_span_size_ - rhs_count;
		// use this idx to attribute bgin and end
		reduced_span.begin = m_span_stack_[tmp].begin;
		reduced_span.end = m_span_stack_[m_span_size_ - 1].end;
	}

	// handle span stack, remove old elements 
	m_span_size_ -= rhs_count;

	// push new reduced span
	m_span_stack_[m_span_size_] = reduced_span;
	m_span_size_++;

	return (reduced_span);
}

/* public tools */

size_t					Session::span_size( void ) const
{	return (m_span_size_);	}

const t_parse_span&		Session::top_span( void ) const
{
	if (m_span_size_ == 0)
		throw std::logic_error("ParserSession: empty span stack");

	return (m_span_stack_[m_span_size_ - 1]);
}

const t_parse_error&	Session::error( void ) const
{
	if (!m_parse_error_.has_error)
		throw std::logic_error("ParserSession::error: no error");
	
	return (m_parse_error_);
}

/* semantics tools */

t_semantic_result		Session::dispatch_reduce(size_t rule_id, const t_parse_span& span)
{
	const t_rule_entry& rule = m_table_.rule(rule_id);
	t_reduce_event		event;
	t_semantic_result	semantic_res;

	if (!m_semantics_ || rule.semantic_id == PARSING_NO_SEMANTIC)
		return (SEMANTIC_OK);
	
	// prepare event to be handled by the callback function
	event.rule_id = rule_id;
	event.lhs = (t_id_symbol)rule.lhs;
	event.rhs_count = (size_t)rule.rhs_count;
	event.rhs_begin = m_span_size_ - 1;
	event.span = span;

	// call to func ptr, via SemanticRuntime member class
	semantic_res = m_semantics_->dispatch(rule.semantic_id, event, m_user_ctx_);

	// in case  of error we just announce it
	// the error code is managed by the user,
	// everyting out of 0 is an error
	if (semantic_res != SEMANTIC_OK)
	{
		m_semantic_error_.has_error = true;
		m_semantic_error_.code = semantic_res;
		m_semantic_error_.event = event;
	}

	// same return for ERROR and SUCCESS
	return (semantic_res);
}

const t_semantic_error&	Session::semantic_error(void) const
{
	if (!m_semantic_error_.has_error)
		throw std::logic_error("ParserSession::semantic_error: no error");
	
	return (m_semantic_error_);
}
