// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#include "CFG.hpp"

t_id_symbol		CFG::start_symbol( void ) const
{	return (m_start_symbol_);	}

const t_rule&	CFG::rule( t_id_rule id ) const
{	return (mv_rules_[id]);	}

t_id_symbol		CFG::rhs_symbol( const t_rule& rule, size_t index ) const
{	return (mv_rhs_[rule.rhs_slice.begin + index]);	}

size_t			CFG::rule_count( void ) const
{	return (mv_rules_.size());	}

size_t			CFG::non_terminal_count( void ) const
{	return (m_non_terminal_count_);	}
