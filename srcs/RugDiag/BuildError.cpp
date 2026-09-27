// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#include "../../inc/RugDiag.hpp"

RugBuildError::RugBuildError( const RugDiag& diagnostics, const std::string& message )
:	std::runtime_error(message),
	m_diagnostics_(diagnostics)
{	}

RugBuildError::~RugBuildError() throw()
{	}

const RugDiag&	RugBuildError::diagnostics( void ) const
{	return (m_diagnostics_);	}

void			RugBuildError::print(std::ostream& out, bool color) const
{	m_diagnostics_.print(out, color);	}
