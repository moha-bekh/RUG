// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstddef>
#include <string>

#include "../../SourceFile/SourceFile.hpp"
#include "../../Lexer/Lexer.hpp"
#include "../ABNFIR/ABNFIR.hpp"
#include "../../Diagnostic/Diagnostic.hpp"

static const size_t ABNF_INVALID_NUMBER = (size_t)-1;

class ABNFParser
{
	private:
		// borrowed
		const SourceFile&		m_src_;
		const t_token_buffer&	m_tokens_;
		DiagnosticEngine&		m_diag_;
		ABNFIR&					m_ir_;

		// owned
		size_t					m_pos_;

	public:
		/* OCF */
		ABNFParser( const SourceFile& src, const t_token_buffer& tokens, DiagnosticEngine& diag, ABNFIR& ir );
		~ABNFParser( void );

		bool	parse( void );
	
	private:
		/* token navigation tools*/
		bool			eof( void ) const;
		const t_token&	current( void ) const;
		const t_token&	peek( size_t offset ) const;
		bool			check( e_token_type type ) const;
		bool			match( e_token_type  type );
		void			advance( void );

		/* parsing */
		bool			parse_rule( void );
		t_abnf_node_id	parse_alternation( void );
		t_abnf_node_id	parse_concatenation( void );
		t_abnf_node_id	parse_repetition( void );
		t_abnf_node_id	parse_element( void );
		t_abnf_node_id	parse_num_val( void );
		t_abnf_node_id	parse_char_val( void );
		t_abnf_node_id	parse_group( void );
		t_abnf_node_id	parse_option( void );

		/* parsing tools */
		bool			type_starts_repetition(e_token_type type) const;
		size_t			parse_number_token( const t_token& tok );
		size_t			parse_num_base( size_t offset, size_t len, size_t base );
		void			skip_c_wsp( void );
		bool			match_c_wsp( void );
		bool			match_concat_separator( void );
		bool			match_alternation_separator( void );
			
		/* errors*/
		void			error_current( const std::string& message );
		void			recover_rule( void );

	private:
		/* forbiden OCF*/
		ABNFParser( void );
		ABNFParser( const ABNFParser& other );
		ABNFParser& operator= ( const ABNFParser& other );

};