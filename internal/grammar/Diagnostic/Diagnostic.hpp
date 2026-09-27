// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#pragma once

# include <cstddef>
# include <iosfwd>
# include <string>

# include "../SourceFile/SourceFile.hpp"
# include "../../../inc/RugDiag.hpp"

class DiagnosticEngine
{
	private:
		RugDiag	m_report_;

	public:
		explicit DiagnosticEngine( const SourceFile& src, const std::string& name = "grammar.abnf" );
		~DiagnosticEngine();

		void	error( const t_source_span& span, const std::string& message );
		void	warning( const t_source_span& span, const std::string& message );
		void	note( const t_source_span& span, const std::string& message );

		bool			has_errors( void ) const;
		size_t			error_count( void ) const;
		size_t			warning_count( void ) const;
		const RugDiag&	report( void ) const;

		void	throw_if_errors( const std::string& message = "RUG: build failed" ) const;
		void	print_all( void ) const;
		void	print_all( std::ostream& out, bool color = false ) const;

	private:
		DiagnosticEngine( void );
		DiagnosticEngine( const DiagnosticEngine& );
		DiagnosticEngine&	operator=( const DiagnosticEngine& );
};