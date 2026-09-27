// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <map>
#include <string>

#include "../../SourceFile/SourceFile.hpp"
#include "../ABNFIR/ABNFIR.hpp"
#include "../../Diagnostic/Diagnostic.hpp"

class ABNFValidator
{
	private:
		/* borrowed */
		const SourceFile&	m_src_;
		const ABNFIR&		m_ir_;
		DiagnosticEngine&	m_diag_;

	public:
		ABNFValidator( const SourceFile& src, const ABNFIR& ir, DiagnosticEngine& diag );
		~ABNFValidator( void );

		bool		validate( void );
	
	private:
		// validation
		bool		validate_rules( std::map<std::string, size_t>& rules );
		bool		validate_references( const std::map<std::string, size_t>& rules );
		bool		validate_node_references( t_abnf_node_id id, const std::map<std::string, size_t>& rules );
		bool		validate_supported_nodes( void );
		// tools 
		bool		is_node_supported( t_abnf_node_id id );
		std::string	canonical_name( const t_source_span& span ) const;

	private:
		/* forbiden OCF*/
		ABNFValidator( void );
		ABNFValidator( const ABNFValidator& other );
		ABNFValidator& operator= ( const ABNFValidator& other );

};