// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#include "TableIRBuilder.hpp"

TableIRBuilder::TableIRBuilder( const CFG& cfg, const LRGraph& lalr, TableIR& table )
:	m_cfg_(cfg),
	m_lalr_(lalr),
	m_table_(table)
{	}

TableIRBuilder::~TableIRBuilder( void )
{	}