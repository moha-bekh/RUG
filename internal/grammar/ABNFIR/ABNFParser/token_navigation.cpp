// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#include "ABNFParser.hpp"

bool			ABNFParser::eof( void ) const
{	return (m_pos_ >= m_tokens_.size() || m_tokens_[m_pos_].type == T_END);	}

const t_token&	ABNFParser::current( void ) const
{	return (m_tokens_[m_pos_]);	}

const t_token&	ABNFParser::peek( size_t offset ) const
{
	size_t	pos = m_pos_ + offset;

	if (pos >= m_tokens_.size())
		return (m_tokens_[m_tokens_.size() - 1]);
	
	return (m_tokens_[pos]);
}

bool			ABNFParser::check( e_token_type type ) const
{	return (m_pos_ < m_tokens_.size() && current().type == type);	}

bool			ABNFParser::match( e_token_type type )
{
	if (!check(type))
		return (false);
	
	advance();

	return (true);
}

void			ABNFParser::advance( void )
{
	if (!eof())
		m_pos_++;
}
