// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#include "CFGBuilder.hpp"

void 		CFGBuilder::lower_char_val( const t_abnf_node& node, std::vector<t_id_symbol>& v_rhs )
{
	uint8_t		byte;
	const char*	addr;

	for (size_t i = 0; i < node.char_val.span.len; i++)
	{
		addr = m_src_.data();
		byte = (uint8_t)addr[node.char_val.span.offset + i];

		if (node.char_val.is_sensitive)
			v_rhs.push_back(m_cfg_.terminal(byte));
		else
			v_rhs.push_back(make_case_insensitive_byte(byte));
	}
}

void 		CFGBuilder::lower_num_val( const t_abnf_node& node, std::vector<t_id_symbol>& v_rhs )
{
	size_t						value;
	const t_abnf_num_val&		num =  node.num_val;

	// in this code the number will be translated to terminal value
	// so the number (single or in sequence) is garanteed to be UNDER 256

	if (num.kind == ABNF_NUM_SINGLE)
	{
		value = m_ir_.num_value(node, 0);
		v_rhs.push_back(m_cfg_.terminal((uint8_t)value));
		return ;
	}

	if (num.kind == ABNF_NUM_SEQUENCE)
	{
		for (size_t i = 0; i < num.value_count; i++)
		{
			value = m_ir_.num_value(node, i);
			v_rhs.push_back(m_cfg_.terminal((uint8_t)value));
		}
		return ;
	}

	// ABNF_NUM_RANGE section
	v_rhs.push_back(make_num_range_symbol(node));
}

t_id_symbol CFGBuilder::make_case_insensitive_byte( uint8_t byte )
{
	uint8_t						lower;
	uint8_t						upper;
	t_id_symbol					id;

	// build upper and lower values for letters
	// exit the rest
	if (byte >= 'A' && byte <= 'Z')
	{
		upper = byte;
		lower = byte - 'A' + 'a';
	}
	else if (byte >= 'a' && byte <= 'z')
	{
		lower = byte;
		upper = byte - 'a' + 'A';
	}
	else
	{
		// send all other ascii to normal conversion
		return (m_cfg_.terminal(byte));
	}

	// search into cache to avoid duplication
	if (m_cache_ci_symbol_ids_[lower] != CFG_INVALID_SYMBOL)
		return (m_cache_ci_symbol_ids_[lower]);

	id = new_synthetic_symbol("ci");

	// create upper and lower rules
	m_cfg_.add_rule(id, m_cfg_.terminal(upper));
	m_cfg_.add_rule(id, m_cfg_.terminal(lower));

	// create cache entry
	m_cache_ci_symbol_ids_[lower] = id;

	return (id);
}

t_id_symbol	CFGBuilder::make_num_range_symbol( const t_abnf_node& node )
{
	const size_t	begin = m_ir_.num_value(node, 0);
	const size_t	end = m_ir_.num_value(node, 1);
	t_id_symbol		id = new_synthetic_symbol("range");

	for (size_t value = begin; value <= end; value++)
		m_cfg_.add_rule(id, m_cfg_.terminal((uint8_t)value));
	
	return (id);
}