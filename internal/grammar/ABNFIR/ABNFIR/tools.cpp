// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#include "ABNFIR.hpp"

t_abnf_node_id	ABNFIR::make_children_node( e_abnf_node_type type, \
											const t_source_span& span, \
											const std::vector<t_abnf_node_id>& child_ids )
{
	t_abnf_node	node;

	node.type = type;
	node.span = span;

	node.child.begin = m_children_ids_.size();
	node.child.count = child_ids.size();

	for (size_t i = 0; i < child_ids.size(); i++)
		m_children_ids_.push_back(child_ids[i]);

	m_nodes_.push_back(node);

	return (m_nodes_.size() - 1);
}

t_abnf_node_id	ABNFIR::make_leaf( const t_source_span& span, e_abnf_node_type type )
{
	t_abnf_node	node;

	node.type = type;
	node.span = span;

	m_nodes_.push_back(node);

	return (m_nodes_.size() - 1);
}

t_abnf_node_id	ABNFIR::make_num_val( const t_source_span& span, const t_abnf_num_val& val, \
									  const std::vector<size_t>& values )
{
	t_abnf_node	node;

	node.type = ABNF_NUM_VAL;
	node.span = span;
	node.num_val = val;

	node.num_val.value_begin = m_num_values_.size();
	node.num_val.value_count = values.size();

	for (size_t i = 0; i < values.size(); i++)
		m_num_values_.push_back(values[i]);
	
	m_nodes_.push_back(node);

	return (m_nodes_.size() -1);
}

t_abnf_node_id	ABNFIR::make_char_val( const t_source_span& span, const t_abnf_char_val& val )
{
	t_abnf_node node;

	node.type = ABNF_CHAR_VAL;
	node.span = span;

	node.char_val = val;

	m_nodes_.push_back(node);

	return (m_nodes_.size() - 1);
}

t_abnf_node_id	ABNFIR::make_repeat( const t_source_span& span, const t_abnf_repeat& rep )
{
	t_abnf_node node;

	node.type = ABNF_REPEAT;
	node.span = span;

	node.repeat = rep;

	m_nodes_.push_back(node);

	return (m_nodes_.size() - 1);
}

t_abnf_node_id	ABNFIR::make_repeat( const t_source_span& span, \
									 t_abnf_node_id child_id, \
									 size_t min, size_t max, \
									 bool unbounded )
{
	t_abnf_node	node;

	node.type = ABNF_REPEAT;
	node.span = span;

	node.repeat.child = child_id;
	node.repeat.min = min;
	node.repeat.max = max;
	node.repeat.unbounded = unbounded;

	m_nodes_.push_back(node);

	return (m_nodes_.size() - 1);
}



t_abnf_node_id	ABNFIR::make_option( const t_source_span& span, \
								 t_abnf_node_id child_id )
{
	t_abnf_node node;

	node.type = ABNF_OPTION;
	node.span = span;
	
	node.unary.child = child_id;

	m_nodes_.push_back(node);

	return (m_nodes_.size() - 1);
}


t_abnf_node_id	ABNFIR::make_concat( const t_source_span& span, \
									 const std::vector<t_abnf_node_id>& child_ids )
{	return (make_children_node(ABNF_CONCAT, span, child_ids));	}

t_abnf_node_id	ABNFIR::make_alt( const t_source_span& span, \
								  const std::vector<t_abnf_node_id>& child_ids )
{	return (make_children_node(ABNF_ALT, span, child_ids));	}

void	ABNFIR::add_rule( const t_source_span& name, \
						  const t_source_span& span, \
						  t_abnf_node_id root_id, \
						  bool incremental )
{
	t_abnf_rule	rule;

	rule.name = name;
	rule.span = span;
	rule.root_id = root_id;
	rule.incremental = incremental;

	m_rules_.push_back(rule);
}
