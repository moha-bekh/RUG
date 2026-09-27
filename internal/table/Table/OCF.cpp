// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#include "TableBuilder.hpp"

TableBuilder::TableBuilder( const CFG& cfg, const TableIR& ir )
:	m_cfg_(cfg),
	m_ir_(ir),
	m_semantics_(NULL),
	m_action_count_(0),
	m_goto_count_(0),
	m_action_spans_offset_(0),
	m_action_entries_offset_(0),
	m_goto_spans_offset_(0),
	m_goto_entries_offset_(0),
	m_required_size_(0)
{
	compute_layout();
	validate_layout();
}

TableBuilder::TableBuilder( const CFG& cfg, const TableIR& ir, const SemanticBindings& sematics )
:	m_cfg_(cfg),
	m_ir_(ir),
	m_semantics_(&sematics),
	m_action_count_(0),
	m_goto_count_(0),
	m_action_spans_offset_(0),
	m_action_entries_offset_(0),
	m_goto_spans_offset_(0),
	m_goto_entries_offset_(0),
	m_required_size_(0)
{
	compute_layout();
	validate_layout();
}

TableBuilder::~TableBuilder( void )
{	}