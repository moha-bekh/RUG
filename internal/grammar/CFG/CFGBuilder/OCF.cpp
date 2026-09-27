// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#include "CFGBuilder.hpp"

CFGBuilder::CFGBuilder( const SourceFile& src, const ABNFIR& ir, DiagnosticEngine& diag, CFG& cfg, const std::string& start_rule, bool debug_mode )
:	m_src_(src),
	m_ir_(ir),
	m_diag_(diag),
	m_cfg_(cfg),
	m_start_name_(start_rule),
	mm_rule_symbols_(),
	m_synthetic_count_(0),
	mb_debug_mode_(debug_mode),
	mv_productive_(),
	mv_reachable_()
{
	for (size_t i = 0; i < 256; i++)
		m_cache_ci_symbol_ids_[i] = CFG_INVALID_SYMBOL;
}

CFGBuilder::~CFGBuilder( void )
{	}