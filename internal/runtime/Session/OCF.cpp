// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#include "Session.hpp"

Session::Session( const TableView& table,
				  t_id_state *state_stack, size_t state_capacity,
				 t_parse_span *span_stack, size_t span_capacity )
:	m_table_(table),
	m_engine_(table, state_stack, state_capacity),
	m_span_stack_(span_stack),
	m_span_capacity_(span_capacity),
	m_span_size_(0),
	m_semantics_(NULL),
	m_user_ctx_(NULL),
	m_parse_error_(),
	m_semantic_error_()
{	reset_session();	}

Session::Session( const TableView& table,
				 t_id_state *state_stack, size_t state_capacity,
				 t_parse_span *span_stack, size_t span_capacity,
				 const SemanticRuntime& semantics, void *user_ctx)
:	m_table_(table),
	m_engine_(table, state_stack, state_capacity),
	m_span_stack_(span_stack),
	m_span_capacity_(span_capacity),
	m_span_size_(0),
	m_semantics_(&semantics),
	m_user_ctx_(user_ctx),
	m_parse_error_(),
	m_semantic_error_()
{	reset_session();	}

Session::~Session( void )
{	}