// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#include "Lexer.hpp"

Lexer::Lexer( const SourceFile& src, t_token_buffer& buf, DiagnosticEngine& diag )
:	m_pos_(0), m_line_(1),
	m_column_(1),
	m_src_(src), m_buf_(buf),
	m_diag_(diag)
{	}

Lexer::~Lexer( void )
{	}