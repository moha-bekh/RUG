// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#include "LRBuilder.hpp"

/*
		LRBuilder( const CFG& cfg, LRGraph& lr_graph );
		LRBuilder::~LRBuilder( void );
*/

LRBuilder::LRBuilder( const CFG& cfg, LRGraph& lr_graph )
:	m_cfg_(cfg), m_lr_graph_(lr_graph)
{	}

LRBuilder::~LRBuilder( void )
{	}