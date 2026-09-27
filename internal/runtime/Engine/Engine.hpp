// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstddef>
#include <stdexcept>

#include "../../table/TableView/TableView.hpp"

enum e_engine_result_type
{
	ENGINE_SHIFT,
	ENGINE_REDUCE,
	ENGINE_ACCEPT,
	ENGINE_ERROR,
	ENGINE_STACK_OVERFLOW
};

typedef struct				s_engine_result
{
	e_engine_result_type	type;
	size_t					value;
}							t_engine_result;

class Engine
{
	private:
		const TableView&	m_table_;

		t_id_state			*m_stack_;
		size_t				m_stack_capacity_;
		size_t				m_stack_size_;

	public:
		Engine( const TableView& table, t_id_state *stack, size_t stack_capacity );
		~Engine( void );

		void				reset( void );

		t_engine_result		step( t_id_symbol terminal );

		t_id_state			top_state( void ) const;
		size_t				stack_size( void ) const;

	private:
		t_engine_result		do_shift( t_id_state target );
		t_engine_result		do_reduce(size_t rule_id );

	private:
		Engine( void );
		Engine( const Engine& other );
		Engine& operator=( const Engine& other );
};