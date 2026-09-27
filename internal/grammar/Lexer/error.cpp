// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#include "Lexer.hpp"

void	Lexer::report_unknown_character( void )
{
	std::ostringstream	msg;
	unsigned char		c;

	c = (unsigned char)current();

	if (c >= 32 && c <= 126)
		msg << "unexpected character '" << (char)c << "'";
	else
		msg << "unexpected byte '" << (unsigned int)c << "'";

	m_diag_.error(current_span(1), msg.str());
}