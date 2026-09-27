// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#include "ABNFValidator.hpp"

ABNFValidator::ABNFValidator( const SourceFile& src, const ABNFIR& ir, DiagnosticEngine& diag )
:	m_src_(src), m_ir_(ir), m_diag_(diag)
{	}

ABNFValidator::~ABNFValidator( void )
{	}