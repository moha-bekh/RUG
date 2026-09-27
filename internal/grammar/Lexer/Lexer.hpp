// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <iomanip>
#include <sstream>

#include "../SourceFile/SourceFile.hpp"
#include "../Diagnostic/Diagnostic.hpp"

enum e_token_type
{
	T_RULENAME,		
	T_NUMBER,		// 0-9 // use in repetitions eg: 3DIGIT 1*5DIGIT
	
	T_EQUAL,		// =
	T_EQUAL_SLASH,	// =/

	T_SLASH,		// /
	T_STAR,			// *

	T_LPAREN,		// (
	T_RPAREN,		// )
	T_LBRACKET,		// [
	T_RBRACKET,		// ]

	T_CHAR_VAL,		// default case_insensitive : "abc"
	T_I_CHAR_VAL,	// %i"abc"
	T_S_CHAR_VAL,	// %s"abc"

	T_D_NUM_VAL,	// %d13		// CR (ascii val)
	T_X_NUM_VAL,	// %x0D		// CR
	T_B_NUM_VAL,	// %b1101	// CR

	T_WSP,			// white spaces
	T_NL,			// new-line

	T_PROSE_VAL,	// <external def ...> // not accepted by the parser but recognized as a token

	T_END			// EOF
};

typedef struct		s_token
{
	e_token_type	type;
	t_source_span	span;
}					t_token;

typedef std::vector<t_token> t_token_buffer;

class Lexer
{
	private:
		/* owned */
		size_t					m_pos_;
		size_t					m_line_;
		size_t					m_column_;
		
		/* borrowed */
		const SourceFile&		m_src_;
		t_token_buffer&			m_buf_;
		DiagnosticEngine&		m_diag_;

	public:
		/* OCF */
		Lexer( const SourceFile& src, t_token_buffer& buf, DiagnosticEngine& diag );
		~Lexer( void );

		void			tokenize( void );

	private:
		/*tools*/
		bool			eof( void ) const;
		char			current( void ) const;
		char			peek( size_t offset ) const;
		void			advance( void );
		void			push_token( e_token_type type, const t_source_span& span );

		bool			is_alpha( char c ) const;
		bool			is_digit( char c ) const;
		bool			is_wsp( char c ) const;

		t_source_span	current_span( size_t len ) const;

		/* lexing */
		void			lex_rulename( void );
		void			lex_number( void );
		void			lex_wsp( void );
		void			lex_single(e_token_type type);
		void			lex_equal( void );
		void			lex_newline( void );
		void			skip_comment( void );
		void			lex_char_val( void );
		void			lex_prose_val( void );

			// lex percent hard part
		void			lex_percent( void );
		void			lex_prefixed_char_val( e_token_type type );
		bool			is_base_digit( char c, int base ) const;
		void			recover_element( void );
		void			lex_num_val( e_token_type type, int base );

		/* error */
		void			report_unknown_character( void );

	private:
		/* forbiden OCF */
		Lexer( void );
		Lexer( const Lexer& other );
		Lexer&	operator= ( const Lexer& other );

};

// printers
void	print_tokens( const SourceFile& src, const t_token_buffer& buf );
