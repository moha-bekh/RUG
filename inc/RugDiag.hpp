// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstddef>
#include <iosfwd>
#include <stdexcept>
#include <string>
#include <vector>
#include <algorithm>
#include <ostream>
#include <sstream>

#include "../internal/includes/common.hpp"

#define	DIAG_TAB_WIDTH 4
#define	DIAG_WINDOW_WIDTH 100

enum e_rug_diag_level
{
	RUG_DIAG_ERROR,
	RUG_DIAG_WARNING,
	RUG_DIAG_NOTE
};

typedef struct			s_rug_diag
{
	e_rug_diag_level	level;
	std::string			message;

	// if the source is lost such as in a streamed input 
	// (eg. HTTP Stream)
	// has_span is set  to false and the error output is minimal
	bool				has_span;
	size_t				offset;
	size_t				lenght;
}						t_rug_diag;

class RugDiag
{
	private:
		std::string						m_name_;
		std::string						m_source_;
		bool							mb_has_source_;
		std::vector<size_t>				mv_line_starts_;
		std::vector<t_rug_diag>			mv_entries_;
		size_t							m_error_count_;
		size_t							m_warning_count_;

	public:
		// OCF
		explicit RugDiag( const std::string& name = "" );
		RugDiag( const std::string& name, const char *data, size_t size );

		// std::vector<t_rug_diag>	mv_entries_ setters
		void				error( size_t offset, size_t length, const std::string& message );
		void				error( const std::string& message );
		void				warning( size_t offset, size_t length, const std::string& message );
		void				warning( const std::string& message );
		void				note( size_t offset, size_t length, const std::string& message );
		void				note( const std::string& message );

		// getters
		bool				has_errors( void ) const;
		size_t				error_count( void ) const;
		size_t				warning_count( void ) const;
		size_t				count( void ) const;
		const t_rug_diag&	entry( size_t index ) const;

		/* Error Factories */
		static RugDiag		failure( const std::string& message, const std::string& name = "" );

		static RugDiag		syntax( const t_parse_error& error, const char *data = NULL, size_t size = 0,
									const std::string& name = "input" );

		static RugDiag		semantic( const t_semantic_error& error, const std::string& message = "semantic callback failed",
									  const char *data = NULL, size_t size = 0, const std::string& name = "input" );

		static RugDiag		stack_overflow( size_t offset, size_t capacity, const char *data = NULL, size_t size = 0,
											const std::string& name = "input" );

		void				print( std::ostream& out, bool color = false ) const;

	private:
		// will add a t_rug_diag to mv_entries_
		void				add( e_rug_diag_level level, bool has_span, size_t offset, size_t length, const std::string& message );
		// helper to access line for pretty print
		void				build_line_index( void );
		void				print_entry( std::ostream& out, const t_rug_diag& diag, bool color ) const;

};

class RugBuildError : public std::runtime_error
{
	private:
		RugDiag	m_diagnostics_;

	public:
		explicit RugBuildError(const RugDiag& diagnostics, const std::string& message = "RUG: build failed");
		virtual ~RugBuildError() throw();

		const RugDiag&	diagnostics(void) const;
		void			print(std::ostream& out, bool color = false) const;
};

/* helpers */
std::string	to_decimal( size_t value );
std::string	to_hex_byte( unsigned char value );
std::string	safe_text( const std::string& text );
size_t		display_width( unsigned char c, size_t column );
std::string	expand_line( const std::string& src, size_t begin, size_t end, 
						 size_t mark_begin, size_t mark_end, 
						 size_t& col_begin, size_t& col_end );
const char	*level_name( e_rug_diag_level level );
const char	*level_color( e_rug_diag_level level );
void		write_text( std::ostream& out, const std::string& text );
void		write_colored( std::ostream& out, const std::string& text, const char *color, bool enabled );
RugDiag		report_with_source( const std::string& name, const char *data, size_t size );
std::string	terminal_name( t_id_symbol terminal );