// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#include "ABNFIR.hpp"

void	ABNFIR::print_indent( size_t depth ) const
{
	for (size_t i = 0; i < depth; i++)
		std::cout << "  ";
}

const char*	ABNFIR::node_type_to_str(e_abnf_node_type type) const
{
	switch (type)
	{
		case ABNF_REF:
			return ("REF");
		case ABNF_CHAR_VAL:
			return ("CHAR_VAL");
		case ABNF_NUM_VAL:
			return ("NUM_VAL");
		case ABNF_PROSE_VAL:
			return ("PROSE_VAL");
		case ABNF_ALT:
			return ("ALT");
		case ABNF_CONCAT:
			return ("CONCAT");
		case ABNF_REPEAT:
			return ("REPEAT");
		case ABNF_OPTION:
			return ("OPTION");
	}

	return ("UNKNOWN");
}

static std::string	span_to_string( const SourceFile& src, const t_source_span& span )
{	return (std::string(src.data() + span.offset, span.len));	}

void	ABNFIR::print_node( const SourceFile& src, t_abnf_node_id id, size_t depth) const
{
	const t_abnf_node& node = this->node(id);

	print_indent(depth);

	std::cout << "[" << id << "] " << node_type_to_str(node.type);

	if (node.type == ABNF_REF)
	{
		std::cout << " " << span_to_string(src, node.span) << std::endl;
		return ;
	}

	if (node.type == ABNF_PROSE_VAL)
	{
		std::cout << " " << span_to_string(src, node.span) << std::endl;
		return ;
	}

	if (node.type == ABNF_CHAR_VAL)
	{
		std::cout	<< " " << (node.char_val.is_sensitive ? "sensitive" : "insensitive") \
					<< " \"" << span_to_string(src, node.char_val.span) << "\"" << std::endl;
		return ;
	}

	if (node.type == ABNF_NUM_VAL)
	{
		std::cout << " base=" << node.num_val.base;

		if (node.num_val.kind == ABNF_NUM_SINGLE)
			std::cout << " SINGLE";
		else if (node.num_val.kind == ABNF_NUM_RANGE)
			std::cout << " RANGE";
		else if (node.num_val.kind == ABNF_NUM_SEQUENCE)
			std::cout << " SEQUENCE";

		std::cout << " values=[";

		for (size_t i = 0; i < node.num_val.value_count; i++)
		{
			if (i != 0)
				std::cout << ", ";
			std::cout << num_value(node, i);
		}

		std::cout << "]" << std::endl;

		return ;
	}

	if (node.type == ABNF_REPEAT)
	{
		std::cout << " min=" << node.repeat.min;

		if (node.repeat.unbounded)
			std::cout << " max=INF";
		else
			std::cout << " max=" << node.repeat.max;

		std::cout << std::endl;

		print_node(src, node.repeat.child, depth + 1);

		return ;
	}

	if (node.type == ABNF_OPTION)
	{
		std::cout << std::endl;

		print_node(src, node.unary.child, depth + 1);

		return ;
	}

	if (node.type == ABNF_ALT || node.type == ABNF_CONCAT)
	{
		std::cout << " children=" << node.child.count << std::endl;

		for (size_t i = 0; i < node.child.count; i++)
			print_node(src, child(node, i), depth + 1);

		return ;
	}

	std::cout << std::endl;
}

void	ABNFIR::print(const SourceFile& src) const
{
	std::cout << "===== ABNF IR =====" << std::endl;

	for (size_t i = 0; i < m_rules_.size(); i++)
	{
		const t_abnf_rule& rule = m_rules_[i];

		std::cout << std::endl << "RULE [" << i << "] " << span_to_string(src, rule.name);

		if (rule.incremental)
			std::cout << " (incremental)";

		std::cout << " root=" << rule.root_id << std::endl;

		print_node(src, rule.root_id, 1);
	}

	std::cout << std::endl << "nodes: " << m_nodes_.size() << std::endl;
	std::cout << "rules: " << m_rules_.size() << std::endl;
	std::cout << "===================" << std::endl;
}