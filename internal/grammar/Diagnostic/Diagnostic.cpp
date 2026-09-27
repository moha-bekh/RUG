// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#include "Diagnostic.hpp"

#include <iostream>

DiagnosticEngine::DiagnosticEngine( const SourceFile& src, const std::string& name )
:	m_report_(name, src.size() ? src.data() : "", src.size())
{	}

DiagnosticEngine::~DiagnosticEngine()
{	}

void	DiagnosticEngine::error( const t_source_span& span, const std::string& message )
{	m_report_.error(span.offset, span.len, message);	}

void	DiagnosticEngine::warning( const t_source_span& span, const std::string& message )
{	m_report_.warning(span.offset, span.len, message);	}

void	DiagnosticEngine::note( const t_source_span& span, const std::string& message )
{	m_report_.note(span.offset, span.len, message);	}

bool	DiagnosticEngine::has_errors( void ) const
{	return (m_report_.has_errors());	}

size_t	DiagnosticEngine::error_count( void ) const
{	return (m_report_.error_count());	}

size_t	DiagnosticEngine::warning_count( void ) const
{	return (m_report_.warning_count());	}

const RugDiag&	DiagnosticEngine::report( void ) const
{	return (m_report_);	}

void	DiagnosticEngine::throw_if_errors( const std::string& message ) const
{
	if (has_errors())
		throw RugBuildError(m_report_, message);
}

void	DiagnosticEngine::print_all( void ) const
{	m_report_.print(std::cerr);	}

void	DiagnosticEngine::print_all( std::ostream& out, bool color ) const
{	m_report_.print(out, color);	}