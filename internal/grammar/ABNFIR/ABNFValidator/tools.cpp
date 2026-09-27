// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#include "ABNFValidator.hpp"

std::string	ABNFValidator::canonical_name( const t_source_span& span ) const
{
	std::string name(m_src_.data() + span.offset, span.len);

	for (size_t i = 0; i < name.size(); i++)
	{
		if (name[i] >= 'A' && name[i] <= 'Z')
			name[i] = name[i] - 'A' + 'a';
	}

	return (name);
}

bool		ABNFValidator::is_node_supported(t_abnf_node_id id)
{
	const t_abnf_node&	node = m_ir_.node(id);
	bool				valid;
	
	valid = true;

	if (node.type == ABNF_PROSE_VAL)
	{
		m_diag_.error(node.span, "ABNF prose-val is not supported");
		return (false);
	}

	if (node.type == ABNF_NUM_VAL)
	{
		for (size_t i = 0; i < node.num_val.value_count; i++)
		{
			if (m_ir_.num_value(node, i) > 255)
			{
				m_diag_.error(node.span, "ABNF numeric terminal is outside byte range");
				return (false);
			}
		}
		return (true);
	}

	if (node.type == ABNF_REPEAT)
		return (is_node_supported(node.repeat.child));

	if (node.type == ABNF_OPTION)
		return (is_node_supported(node.unary.child));

	if (node.type == ABNF_ALT || node.type == ABNF_CONCAT)
	{
		for (size_t i = 0; i < node.child.count; i++)
		{
			if (!is_node_supported(m_ir_.child(node, i)))
				valid = false;
		}
	}

	return (valid);
}
