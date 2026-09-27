// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#include "ABNFParser.hpp"

void			ABNFParser::error_current( const std::string& message )
{
	if (m_pos_ < m_tokens_.size())
		m_diag_.error(current().span, message);
}

void			ABNFParser::recover_rule( void )
{
	while (!eof() && !check(T_NL))
		advance();

	if (check(T_NL))
		advance();
}