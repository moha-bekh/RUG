// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#include "CFGBuilder.hpp"

void 		CFGBuilder::lower_as_productions( t_id_symbol lhs, t_abnf_node_id node_id )
{
	const t_abnf_node&			node = m_ir_.node(node_id);
	std::vector<t_id_symbol>	v_rhs;

	if (node.type == ABNF_ALT)
	{
		for (size_t i = 0; i < node.child.count; i++)
			lower_as_productions(lhs, m_ir_.child(node, i));
		return ;
	}

	if (node.type == ABNF_OPTION)
	{
		// create rule with span.count = 0 meaning EPSILON
		m_cfg_.add_empty_rule(lhs);
		// then create the option
		lower_as_productions(lhs, node.unary.child);
		return ;
	}

	lower_as_sequence(node_id, v_rhs);

	if (v_rhs.empty())
		m_cfg_.add_empty_rule(lhs);
	else
		m_cfg_.add_rule(lhs, v_rhs);
}

void 		CFGBuilder::lower_as_sequence( t_abnf_node_id node_id,std::vector<t_id_symbol>& v_rhs )
{
	const t_abnf_node&	node = m_ir_.node(node_id);

	switch (node.type)
	{
		// simple : add directly to v_rhs
		case (ABNF_REF):
			v_rhs.push_back(find_rule_symbol(node.span));
			return ;
		
		case (ABNF_CHAR_VAL):
			lower_char_val(node, v_rhs);
			return ;
		
		case (ABNF_NUM_VAL):
			lower_num_val(node, v_rhs);
			return ;

		// complex type : devllop and push ot v_rhs
		case (ABNF_CONCAT):
			for (size_t i = 0; i < node.child.count; i++)
				lower_as_sequence(m_ir_.child(node, i), v_rhs);
			return ;

		case (ABNF_REPEAT):
			lower_repeat(node, v_rhs);
			return ;

		// unrecognize type : impossible to push to rhs, 
		// need to create synthetique node via lower_to_symbol
		case (ABNF_ALT):
			// falltrough
		case (ABNF_OPTION):
			v_rhs.push_back(lower_to_symbol(node_id));
			return ;
		
		// no "case (ABNF_PROSE_VAL)" 
		// as its supposed to be refused at the IR Parsing
		case ABNF_PROSE_VAL:
			throw std::logic_error("CFGBuilder::lower_as_sequence: prose-val reached builder");
	}
}

t_id_symbol CFGBuilder::lower_to_symbol( t_abnf_node_id node_id )
{
	const t_abnf_node&	node = m_ir_.node(node_id);
	t_id_symbol			id;
	size_t				value;

	// if a rule is in the rhs 
	// it must have been declared as a lhs somewhere
	// so we search its existing id
	if (node.type == ABNF_REF)
		return (find_rule_symbol(node.span));

	// can only retreive num val under 256 
	// (already checkd by IRParsing)
	// because now we treat num value that are bytes
	if (node.type == ABNF_NUM_VAL)
	{
		if (node.num_val.kind == ABNF_NUM_SINGLE)
		{
			value = m_ir_.num_value(node, 0);
			// we transfer the value to the corresponding terminal
			return (m_cfg_.terminal((uint8_t)value));
		}
		if (node.num_val.kind == ABNF_NUM_RANGE)
			return (make_num_range_symbol(node));
	}

	// same as in ABNF_NUM_VAL but at least its explicit
	if (node.type == ABNF_CHAR_VAL && \
		node.char_val.is_sensitive && \
		node.char_val.span.len == 1)
		return (m_cfg_.terminal((uint8_t)m_src_.data()[node.char_val.span.offset]));
	
	// default synthetic node creator
	// will be applied to OPTION and ALT nodes
	if (node.type == ABNF_ALT)
		id = new_synthetic_symbol("alt");
	else if (node.type == ABNF_OPTION)
		id = new_synthetic_symbol("option");
	else
		id = new_synthetic_symbol("unknown_node");
	
	lower_as_productions(id, node_id);

	return (id);
}