// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#include "Lexer.hpp"

void	Lexer::lex_percent( void )
{
	t_source_span	span = current_span(0);
	char			prefix;

	if (m_pos_ + 1 >= m_src_.size())
	{
		advance();
		span.len = m_pos_ - span.offset;
		m_diag_.error(span, "expected prefix after '%'");
		return ;
	}

	prefix = peek(1);

	if (prefix == 'b' || prefix == 'B')
		lex_num_val(T_B_NUM_VAL, 2);
	else if (prefix == 'd' || prefix == 'D')
		lex_num_val(T_D_NUM_VAL, 10);
	else if (prefix == 'x' || prefix == 'X')
		lex_num_val(T_X_NUM_VAL, 16);
	else if (prefix == 'i' || prefix == 'I')
		lex_prefixed_char_val(T_I_CHAR_VAL);
	else if (prefix == 's' || prefix == 'S')
		lex_prefixed_char_val(T_S_CHAR_VAL);
	else
	{
		recover_element();
		span.len = m_pos_ - span.offset;
		m_diag_.error(span, "invalid '%' prefix, expected b, d, x, i or s");
	}
}

void	Lexer::lex_prefixed_char_val( e_token_type type )
{
	t_source_span	span = current_span(0);
	unsigned char	c;

	// skip % and prefix
	advance();
	advance();

	if (eof() || current() != '\"')
	{
		recover_element();
		span.len = m_pos_ - span.offset;
		m_diag_.error(span,"expected '\"' after character value prefix");
		return ;
	}

	advance(); // skip "

	while (!eof() && current() != '\"' && current() != '\n' && current() != '\r')
	{
		c = (unsigned char)current();

		if (c < 0x20 || c > 0x7e)
		{
			m_diag_.error(current_span(1), "invalid character in char-val");
			advance();
			continue ;
		}

		advance();
	}

	if (eof() || current() == '\n' || current() == '\r')
	{
		span.len = m_pos_ - span.offset;
		m_diag_.error(span, "unterminated char-val");
		return ;
	}

	advance(); // close "

	span.len = m_pos_ - span.offset;

	push_token(type, span);
}

bool	Lexer::is_base_digit( char c, int base ) const
{
	if (base == 2)
		return (c == '0' || c == '1');
	else if (base == 10)
		return (c >= '0' && c <= '9');
	else if (base == 16)
		return ((c >= '0' && c <= '9') || \
				(c >= 'a' && c <= 'f') || \
				(c >= 'A' && c <= 'F'));
	else
		return (false);
}

void	Lexer::recover_element( void )
{
	char c;

	while (!eof())
	{
		c = current();

		if (is_wsp(c)	|| c == '/' || c == ')' || c == ']' \
						|| c == '\n' || c == '\r' || c == ';')
			return ;
		advance();
	}
}

void	Lexer::lex_num_val( e_token_type type, int base )
{
	t_source_span span = current_span(0);

	// skip % and prefix
	advance();
	advance();
	
	// at least one digit is mandatory
	if (eof() || !is_base_digit(current(), base))
	{
		span.len = m_pos_ - span.offset;
		m_diag_.error(span, "expected digit after numeric prefix");
		recover_element();
		return ;
	}

	// first value
	while (!eof() && is_base_digit(current(), base))
		advance();
	
	// range
	if (!eof() && current() == '-')
	{
		advance();
		if (eof() || !is_base_digit(current(), base))
		{
			span.len = m_pos_ - span.offset;
			m_diag_.error(span, "expected value after '-'");
			recover_element();
			return ;
		}

		while (!eof() && is_base_digit(current(), base))
			advance();

		// a rtange connot become a sequence : %x30-39.40 forbidden
		if (!eof() && current() == '.')
		{
			m_diag_.error(current_span(1), "numeric value cannot mix range and sequence");
			recover_element();
			return ;
		}
	}
	else // sequence
	{
		while (!eof() && current() == '.')
		{
			advance();
			if (eof() || !is_base_digit(current(), base))
			{
				span.len = m_pos_ - span.offset;
				m_diag_.error(span, "expected value after '.'");
				recover_element();
				return ;
			}

			while (!eof() && is_base_digit(current(), base))
				advance();
		}
		if (!eof() && current() == '-')
		{
			m_diag_.error(current_span(1), "numeric value cannot mix sequence and range");
			recover_element();
			return ;
		}
	}

	span.len = m_pos_ - span.offset;

	push_token(type, span);
}