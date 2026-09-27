// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#include "ABNFParser.hpp"

ABNFParser::ABNFParser( const SourceFile& src, const t_token_buffer& tokens, \
						DiagnosticEngine& diag, ABNFIR& ir )
:	m_src_(src), m_tokens_(tokens),
	m_diag_(diag), m_ir_(ir),
	m_pos_(0)
{	}

ABNFParser::~ABNFParser( void )
{	}