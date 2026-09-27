// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#include "LALRBuilder.hpp"

LALRBuilder::LALRBuilder( const LRGraph& lr, LRGraph& lalr )
:	m_lr_(lr), m_lalr_(lalr)
{	}

LALRBuilder::~LALRBuilder(void)
{	}