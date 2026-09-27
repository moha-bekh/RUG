// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#include "Lexer.hpp"

/*
	rulename = ALPHA *(ALPHA / DIGIT / "-")

	first char is alpha the rest is alphja duigit or - 
	minimum 1 character
*/
void	Lexer::lex_rulename( void )
{
	t_source_span span = current_span(0);

	// skip first alpha char (was checked before in the loop)
	advance();

	// skip all character fitting : (ALPHA / DIGIT / "-")
	while (!eof() && (is_alpha(current()) || is_digit(current()) || current() == '-'))
		advance();

	span.len = m_pos_ - span.offset;

	push_token(T_RULENAME, span);
}

/*
	number = 1 * DIGIT

	minimum 1 numeric character
*/
void	Lexer::lex_number( void )
{
	t_source_span span = current_span(0);

	advance();

	while (!eof() && is_digit(current()))
		advance();

	span.len = m_pos_ - span.offset;

	push_token(T_NUMBER, span);
}

void	Lexer::lex_wsp( void )
{
	t_source_span span = current_span(0);

	advance();

	while (!eof() && is_wsp(current()))
		advance();
	
	span.len = m_pos_ - span.offset;
	
	push_token(T_WSP, span);
}

void	Lexer::lex_single( e_token_type type )
{
	t_source_span span = current_span(1);

	advance();

	push_token(type, span);
}

void	Lexer::lex_equal( void )
{
	t_source_span span = current_span(0);

	advance();

	if (!eof() && current() == '/')
	{
		advance();
		span.len = 2;
		push_token(T_EQUAL_SLASH, span);
		return ;
	}
	span.len = 1;
	push_token(T_EQUAL, span);
}

void	Lexer::lex_newline( void )
{
	t_source_span	span = current_span(0);

	if (current() == '\r')
	{
		if (peek(1) != '\n')
		{
			m_diag_.error(current_span(1), "CR must be followed by LF");
			advance();
			return ;
		}
		advance();
	}
	if (!eof() && current() == '\n')
		advance();
	
	span.len = m_pos_ - span.offset;

	push_token(T_NL, span);
}

void	Lexer::skip_comment( void )
{
	advance();

	while (!eof() && current() != '\n' && current() != '\r')
		advance();
}

void	Lexer::lex_char_val( void )
{
	t_source_span	span = current_span(0);
	unsigned char	c;

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

	push_token(T_CHAR_VAL, span);
}

void	Lexer::lex_prose_val( void )
{
	t_source_span	span = current_span(0);
	unsigned char	c;

	advance(); // skip <

	while (!eof() && current() != '>' && current() != '\n' && current() != '\r')
	{
		c = (unsigned char)current();

		if (!((c >= 0x20 && c <= 0x3d) || (c >= 0x3f && c <= 0x7e)))
		{
			m_diag_.error(current_span(1), "invalid character in prose val");
			advance();
			continue ;
		}

		advance();
	}

	if (eof() || current() == '\n' || current() == '\r')
	{
		span.len = m_pos_ - span.offset;
		m_diag_.error(span, "unterminated prose-val");
		return ;
	}

	advance(); // skip '>'

	span.len = m_pos_ - span.offset;

	push_token(T_PROSE_VAL, span);
}
