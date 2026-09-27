// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#include "ABNFIR.hpp"

size_t				ABNFIR::num_value( const t_abnf_node& node, size_t index ) const
{	return (m_num_values_[node.num_val.value_begin + index]);	}

const t_abnf_node&	ABNFIR::node( t_abnf_node_id id ) const
{	return (m_nodes_[id]);	}

const t_abnf_rule&	ABNFIR::rule( size_t id ) const
{	return (m_rules_[id]);	}

size_t				ABNFIR::node_count( void ) const
{	return (m_nodes_.size());	}

size_t				ABNFIR::rule_count( void ) const
{	return (m_rules_.size());	}

t_abnf_node_id		ABNFIR::child( const t_abnf_node& node, size_t index ) const
{	return (m_children_ids_[node.child.begin + index]);	}
