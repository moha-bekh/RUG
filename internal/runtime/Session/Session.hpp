// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstddef>
#include <stdexcept>

#include "../Engine/Engine.hpp"
#include "../../common/ParserTypes/ParserTypes.hpp"
#include "../../semantics/SemanticRuntime/SemanticRuntime.hpp"

class SemanticRuntime;

class Session
{
	private:
		const TableView&		m_table_;
		Engine					m_engine_;

		t_parse_span			*m_span_stack_;
		size_t					m_span_capacity_;
		size_t					m_span_size_;

		const SemanticRuntime	*m_semantics_;
		void					*m_user_ctx_;

		t_parse_error			m_parse_error_;
		t_semantic_error		m_semantic_error_;

	public:
		Session( const TableView& table,
				 t_id_state *state_stack, size_t state_capacity,
				 t_parse_span *span_stack, size_t span_capacity );

		Session( const TableView& table,
				 t_id_state *state_stack, size_t state_capacity,
				 t_parse_span *span_stack, size_t span_capacity,
				 const SemanticRuntime& semantics, void *user_ctx);

		~Session( void );

		void					reset_session( void );
		e_parse_result			push_terminal( t_id_symbol terminal, const t_parse_span& span );
		e_parse_result			finish( size_t end_position );

		// tools
		size_t					span_size( void ) const;
		const t_parse_span&		top_span( void ) const;
		const t_parse_error&	error( void ) const;
		
		// semantics
		t_semantic_result		dispatch_reduce(size_t rule_id, const t_parse_span& span);
		const t_semantic_error&	semantic_error(void) const;
	
	private:
		void					push_shift_span( const t_parse_span& span );
		t_parse_span			apply_reduce_span( size_t rule_id, size_t empty_position );

	private:
		Session( void );
		Session( const Session& other );
		Session& operator=( const Session& other );
	
};