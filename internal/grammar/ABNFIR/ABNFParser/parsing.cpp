// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#include "ABNFParser.hpp"

bool	ABNFParser::parse( void )
{
	m_pos_ = 0;

	while (!eof())
	{
		if (match(T_NL))
			continue ;
		
		if (!parse_rule())
			recover_rule();
	}

	return (!m_diag_.has_errors());
}

t_abnf_node_id	ABNFParser::parse_option( void )
{
	t_source_span	span;
	t_abnf_node_id	child;

	span = current().span;

	advance(); // skip [

	skip_c_wsp();

	child = parse_alternation();
	if (child == ABNF_INVALID_NODE)
		return (ABNF_INVALID_NODE);

	skip_c_wsp();

	if (!check(T_RBRACKET))
	{
		error_current("expected ']'");
		return (ABNF_INVALID_NODE);
	}

	span.len =	current().span.offset \
				+ current().span.len \
				- span.offset;
	
	advance(); // skip ]

	return (m_ir_.make_option(span, child));
}

t_abnf_node_id	ABNFParser::parse_group( void )
{
	t_abnf_node_id child;

	advance(); // skip (

	skip_c_wsp();

	child = parse_alternation();
	if (child == ABNF_INVALID_NODE)
		return (ABNF_INVALID_NODE);

	skip_c_wsp();

	if (!match(T_RPAREN))
	{
		error_current("expected ')'");
		return (ABNF_INVALID_NODE);
	}

	return (child);
}

t_abnf_node_id	ABNFParser::parse_char_val( void )
{
	const t_token&	tok = current();
	t_abnf_char_val	val;

	if (tok.type == T_S_CHAR_VAL)
		val.is_sensitive = true;
	else
		val.is_sensitive = false;

	val.span = tok.span;

	if (tok.type == T_CHAR_VAL)
	{
		// trim ""
		val.span.offset += 1;
		val.span.column += 1;
		val.span.len -= 2;
	}
	else
	{
		// trim %x""
		val.span.offset += 3;
		val.span.column += 3;
		val.span.len -= 4;
	}

	advance();

	return (m_ir_.make_char_val(tok.span, val));
}

t_abnf_node_id	ABNFParser::parse_num_val( void )
{
	const t_token&		tok = current();
	t_abnf_num_val		num_val;
	std::vector<size_t>	values;
	size_t				pos;
	size_t				end;
	size_t				start;
	size_t				value;

	if (tok.type == T_B_NUM_VAL)
		num_val.base = 2;
	else if (tok.type == T_D_NUM_VAL)
		num_val.base = 10;
	else
		num_val.base = 16;
	
	pos = tok.span.offset + 2; // skip %x
	end = tok.span.offset + tok.span.len;

	start = pos;

	while (pos < end && m_src_.data()[pos] != '-' &&  m_src_.data()[pos] != '.')
		pos++;
	
	value = parse_num_base(start, pos - start, num_val.base);
	if (value == ABNF_INVALID_NUMBER)
	{
		m_diag_.error(tok.span, "ABNF numeric value exceeds supported range");
		return (ABNF_INVALID_NODE);
	}

	values.push_back(value);

	if (pos == end)
		num_val.kind = ABNF_NUM_SINGLE;
	else if (m_src_.data()[pos] == '-')
	{
		num_val.kind = ABNF_NUM_RANGE;
		pos++;
		
		value = parse_num_base(pos, end - pos, num_val.base);
		if (value == ABNF_INVALID_NUMBER)
		{
			m_diag_.error(tok.span, "ABNF numeric value exceeds supported range");
			return (ABNF_INVALID_NODE);
		}

		if (values[0] > value)
		{
			m_diag_.error(tok.span, "ABNF numeric range start is greater than end");
			return (ABNF_INVALID_NODE);
		}

		values.push_back(value);
	}
	else 
	{
		num_val.kind = ABNF_NUM_SEQUENCE;

		while (pos < end)
		{
			pos++;

			start = pos;

			while (pos < end && m_src_.data()[pos] != '.')
				pos++;
			
			value = parse_num_base(start, pos - start, num_val.base);
			if (value == ABNF_INVALID_NUMBER)
			{
				m_diag_.error(tok.span, "ABNF numeric value exceeds supported range");
				return (ABNF_INVALID_NODE);
			}

			values.push_back(value);
		}
	}

	advance();

	return (m_ir_.make_num_val(tok.span, num_val, values));

}

t_abnf_node_id	ABNFParser::parse_alternation(void)
{
	std::vector<t_abnf_node_id>	children;
	t_abnf_node_id				child;
	t_source_span				span;

	child = parse_concatenation();
	if (child == ABNF_INVALID_NODE)
		return (ABNF_INVALID_NODE);
	
	children.push_back(child);
	span = m_ir_.node(child).span;

	while (match_alternation_separator())
	{

		child = parse_concatenation();
		if (child == ABNF_INVALID_NODE)
			return (ABNF_INVALID_NODE);
		
		children.push_back(child);
	}

	if (children.size() == 1)
		return (children[0]);

	span.len =	m_ir_.node(child).span.offset \
				+ m_ir_.node(child).span.len \
				- span.offset;

	return (m_ir_.make_alt(span, children));
}


t_abnf_node_id	ABNFParser::parse_concatenation(void)
{
	std::vector<t_abnf_node_id>	children;
	t_abnf_node_id				child;
	t_source_span				span;

	child = parse_repetition();
	if (child == ABNF_INVALID_NODE)
		return (ABNF_INVALID_NODE);
	
	children.push_back(child);
	span = m_ir_.node(child).span;

	while (match_concat_separator())
	{
		child = parse_repetition();
		if (child == ABNF_INVALID_NODE)
			return (ABNF_INVALID_NODE);
		children.push_back(child);
	}

	if (children.size() == 1)
		return (children[0]);
	
	// child is already the last element so we can calculate new span from here

	span.len =	m_ir_.node(child).span.offset \
				+ m_ir_.node(child).span.len \
				- span.offset;
	
	return (m_ir_.make_concat(span, children));
}

/*
repetition handles following conversion:
DIGIT
3DIGIT
1*5DIGIT
1*DIGIT
*5DIGIT
*DIGIT

verbose tyranslation:

RULENAME
NUMBER RULENAME
NUMBER STAR NUMBER RULENAME
NUMBER STAR RULENAME
STAR NUMBER RULENAME
STAR RULENAME
*/
t_abnf_node_id	ABNFParser::parse_repetition(void)
{
	t_source_span	span = current().span;
	bool			has_repeat = false;
	t_abnf_repeat	rep;

	rep.min = 1;
	rep.max = 1;
	rep.unbounded = false;

	if (check(T_NUMBER))
	{
		has_repeat = true;
		rep.min = parse_number_token(current());
		if (rep.min == ABNF_INVALID_NUMBER)
			return (ABNF_INVALID_NODE);
		rep.max = rep.min;

		advance();

		if (match(T_STAR))
		{
			rep.unbounded = true;

			if (check(T_NUMBER))
			{
				rep.max = parse_number_token(current());
				if (rep.max == ABNF_INVALID_NUMBER)
					return (ABNF_INVALID_NODE);
				rep.unbounded = false;
				advance();
			}
		}
	}
	else if (match(T_STAR))
	{
		has_repeat = true;
		rep.min = 0;
		rep.unbounded = true;

		if (check(T_NUMBER))
		{
			rep.max = parse_number_token(current());
			if (rep.max == ABNF_INVALID_NUMBER)
					return (ABNF_INVALID_NODE);
			rep.unbounded = false;
			advance();
		}
	}

	rep.child = parse_element();
	if (rep.child == ABNF_INVALID_NODE)
		return (ABNF_INVALID_NODE);
	
	if (!has_repeat)
		return (rep.child);

	if (!rep.unbounded && rep.min > rep.max)
	{
		m_diag_.error(span, "ABNF repetition minimum is greater than maximum");
		return (ABNF_INVALID_NODE);
	}
	
	span.len = 	m_ir_.node(rep.child).span.offset \
				+ m_ir_.node(rep.child).span.len \
				- span.offset;

	return (m_ir_.make_repeat(span, rep));
}

bool	ABNFParser::parse_rule( void )
{
	t_source_span	name;
	t_source_span	rule_span;
	t_abnf_node_id	root_id;
	bool			incremental;

	if (!check(T_RULENAME))
	{
		error_current("expected rulename");
		return (false);
	}

	name = current().span;
	rule_span = name;

	advance();

	skip_c_wsp();

	if (match(T_EQUAL))
		incremental = false;
	else if (match(T_EQUAL_SLASH))
		incremental = true;
	else
	{
		error_current("expected '=' or '=/' after rulename");
		return (false);
	}

	skip_c_wsp();

	root_id = parse_alternation();

	if (root_id == ABNF_INVALID_NODE)
		return (false);
	
	skip_c_wsp();

	if (!check(T_NL) && !check(T_END))
	{
		error_current("expected end of rule");
		return (false);
	}

	if (check(T_NL))
		rule_span.len = current().span.offset - rule_span.offset;
	else
		rule_span.len = m_src_.size() - rule_span.offset;
	
	m_ir_.add_rule(name, rule_span, root_id, incremental);

	match(T_NL);

	return (true);
}

t_abnf_node_id	ABNFParser::parse_element( void )
{
	t_source_span	span;

	if (eof())
	{
		error_current("expected ABNF element");
		return (ABNF_INVALID_NODE);
	}

	span = current().span;

	if (check(T_CHAR_VAL) || check(T_I_CHAR_VAL) || check(T_S_CHAR_VAL))
		return (parse_char_val());
	else if (check(T_D_NUM_VAL) || check(T_X_NUM_VAL) || check(T_B_NUM_VAL))
		return (parse_num_val());
	else if (match(T_RULENAME))
		return (m_ir_.make_leaf(span, ABNF_REF));
	else if (check(T_LPAREN))
		return (parse_group());
	else if (check(T_LBRACKET))
		return (parse_option());
	else if (match(T_PROSE_VAL))
		return (m_ir_.make_leaf(span, ABNF_PROSE_VAL));
	
	error_current("expected ABNF element");
	return (ABNF_INVALID_NODE);
}
