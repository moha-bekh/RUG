// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#include "Engine.hpp"

Engine::Engine( const TableView& table, t_id_state *stack, size_t stack_capacity )
:	m_table_(table),
	m_stack_(stack),
	m_stack_capacity_(stack_capacity),
	m_stack_size_(0)
{	reset();	}

Engine::~Engine( void )
{	}