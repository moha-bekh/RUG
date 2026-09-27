// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#include "Lexer.hpp"

void	Lexer::tokenize( void )
{
	char c;

	m_buf_.clear();

	m_pos_ = 0;
	m_line_ = 1;
	m_column_ = 1;

	while (!eof())
	{
		c = current();

		if (is_alpha(c))
			lex_rulename();
		else if (is_digit(c))
			lex_number();
		else if (is_wsp(c))
			lex_wsp();
		else if (c == '/')
			lex_single(T_SLASH);
		else if (c == '*')
			lex_single(T_STAR);
		else if (c == '(')
			lex_single(T_LPAREN);
		else if (c == ')')
			lex_single(T_RPAREN);
		else if (c == '[')
			lex_single(T_LBRACKET);
		else if (c == ']')
			lex_single(T_RBRACKET);
		else if (c == '\n' || c == '\r')
			lex_newline();
		else if (c == '=')
			lex_equal();
		else if (c == '\"')
			lex_char_val();
		else if (c == '%')
			lex_percent();
		else if (c == '<')
			lex_prose_val();
		else if (c == ';')
			skip_comment();
		else
		{
			report_unknown_character();
			advance();
		}
	}

	push_token(T_END, current_span(0));
}