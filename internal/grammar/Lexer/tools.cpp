// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#include "Lexer.hpp"

bool	Lexer::eof( void ) const
{	return (m_pos_ >= m_src_.size());	}

char	Lexer::current( void ) const
{	return (m_src_.data()[m_pos_]);	}

char	Lexer::peek( size_t offset ) const
{
	if (m_pos_ + offset >= m_src_.size())
	 	return ('\0');
	return (m_src_.data()[m_pos_ + offset]);
}

void	Lexer::advance( void )
{
	if (eof())
		return ;
	
	if (current() == '\n')
	{
		m_line_++;
		m_column_ = 1;
	}
	else
		m_column_++;
	
	m_pos_++;
}

void	Lexer::push_token( e_token_type type, const t_source_span& span )
{
	t_token tok;

	tok.type = type;
	tok.span = span;

	m_buf_.push_back(tok);
}

bool	Lexer::is_alpha( char c ) const
{	return ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'));	}

bool	Lexer::is_digit( char c ) const
{	return (c >= '0' && c <= '9');	}

bool	Lexer::is_wsp( char c ) const
{	return (c == ' ' || c == '\t');	}

t_source_span	Lexer::current_span( size_t len ) const
{
	t_source_span span;
	
	span.offset = m_pos_;
	span.len = len;
	span.line = m_line_;
	span.column = m_column_;

	return (span);
}
