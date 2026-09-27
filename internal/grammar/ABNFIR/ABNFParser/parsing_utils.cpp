// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#include "ABNFParser.hpp"

/*
in the concatenation process we will launch repetion process such as defined in the rfc
so to do so we need to determine what is a repetion and :
a repetion can not start with the following tokens :
	T_EQUAL,
	T_EQUAL_SLASH,
	T_SLASH,
	T_RPAREN
	T_RBRACKET,
	T_WSP,
	T_NL,
	T_END	
*/
bool	ABNFParser::type_starts_repetition(e_token_type type) const
{
	return (type == T_NUMBER || \
			type == T_STAR || \
			type == T_RULENAME || \
			type == T_CHAR_VAL || \
			type == T_I_CHAR_VAL || \
			type == T_S_CHAR_VAL || \
			type == T_D_NUM_VAL || \
			type == T_X_NUM_VAL || \
			type == T_B_NUM_VAL || \
			type == T_PROSE_VAL || \
			type == T_LPAREN || \
			type == T_LBRACKET);
}

/*
	SIZE_MAX / (size_t)-1 is the sentinell value
*/
size_t	ABNFParser::parse_number_token( const t_token& tok )
{
	size_t	value;
	size_t	tmp;
	size_t	digit;
	size_t	i;
	size_t	end;

	value = 0;
	i = tok.span.offset;
	end = i + tok.span.len;

	while (i < end)
	{
		digit = (size_t)(m_src_.data()[i] - '0');

		if (__builtin_mul_overflow(value, (size_t)10, &tmp) || __builtin_add_overflow(tmp, digit, &value))
		{
			value = ABNF_INVALID_NUMBER;
			break ;
		}

		i++;
	}

	if (value == ABNF_INVALID_NUMBER)
		m_diag_.error(tok.span, "ABNF repetion value overflow (size_t)-1");
	
	return (value);
}

size_t	ABNFParser::parse_num_base( size_t offset, size_t len, size_t base )
{
	size_t	value;
	size_t	tmp;
	size_t	digit;
	size_t	i;
	size_t	end;
	char	c;

	value = 0;
	i = offset;
	end = offset + len;

	while (i < end)
	{
		c = m_src_.data()[i];

		if (c >= '0' && c <= '9')
			digit = (size_t)(c - '0');
		else if (c >= 'a' && c <= 'f')
			digit = (size_t)(c - 'a' + 10);
		else if (c >= 'A' && c <= 'F')
			digit = (size_t)(c - 'A' + 10);
		else
			return (ABNF_INVALID_NUMBER);
		
		if (digit >= base)
			return (ABNF_INVALID_NUMBER);

		if (__builtin_mul_overflow(value, (size_t)base, &tmp) || __builtin_add_overflow(tmp, digit, &value))
			return (ABNF_INVALID_NUMBER);
		
		i++;
	}

	return (value);
}

void	ABNFParser::skip_c_wsp( void )
{
	while (match_c_wsp())
		;
}

bool	ABNFParser::match_c_wsp( void )
{
	if (match(T_WSP))
		return (true);
	
	if (check(T_NL) && peek(1).type == T_WSP)
	{
		advance(); // nl
		advance(); // wsp
		return (true);
	}

	return (false);
}

bool	ABNFParser::match_concat_separator( void )
{
	size_t saved_pos = m_pos_;

	if (!match_c_wsp())
		return (false);
	
	skip_c_wsp();
	
	if (eof() || !type_starts_repetition(current().type))
	{
		m_pos_ = saved_pos;
		return (false);
	}

	return (true);
}

bool	ABNFParser::match_alternation_separator( void )
{
	size_t saved_pos = m_pos_;
	
	skip_c_wsp();
	
	if (!match(T_SLASH))
	{
		m_pos_ = saved_pos;
		return (false);
	}
	
	skip_c_wsp();

	return (true);
}