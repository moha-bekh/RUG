// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <map>
#include <vector>
#include <string>
#include <sstream>
#include <cstddef>
#include <stdint.h>
#include <stdexcept>

#include "../../SourceFile/SourceFile.hpp"
#include "../../ABNFIR/ABNFIR/ABNFIR.hpp"
#include "../../Diagnostic/Diagnostic.hpp"
#include "../CFG/CFG.hpp"

class CFGBuilder
{
	private:
		// borrowed
		const SourceFile&					m_src_;
		const ABNFIR&						m_ir_;
		DiagnosticEngine&					m_diag_;
		CFG&								m_cfg_;

		// owned
			// base
		std::string							m_start_name_;
		std::map<std::string, t_id_symbol>	mm_rule_symbols_;
		size_t								m_synthetic_count_;
		bool								mb_debug_mode_;
		t_id_symbol							m_cache_ci_symbol_ids_[256];

			// validation
		std::vector<bool>					mv_productive_;
		std::vector<bool>					mv_reachable_;

	public:
		CFGBuilder( const SourceFile& src, const ABNFIR& ir, DiagnosticEngine& diag, CFG& cfg, const std::string& start_rule, bool debug_mode );
		~CFGBuilder( void );

		void 			build( void );

		// helper
		size_t			non_terminal_index( t_id_symbol id ) const;

	private: // build base
		void 			build_base( void );
		// build
		void 			create_rule_symbols( void );
		void 			set_start_symbol( void );
		void 			lower_rules( void );

		// lowering
		void 			lower_as_productions( t_id_symbol lhs, t_abnf_node_id node_id );
		void 			lower_as_sequence( t_abnf_node_id node_id, std::vector<t_id_symbol>& v_rhs );
		t_id_symbol 	lower_to_symbol( t_abnf_node_id node_id );
		
		// literal
		void 			lower_char_val( const t_abnf_node& node, std::vector<t_id_symbol>& v_rhs );
		void 			lower_num_val( const t_abnf_node& node, std::vector<t_id_symbol>& v_rhs );
		t_id_symbol 	make_case_insensitive_byte( uint8_t byte );
		t_id_symbol		make_num_range_symbol( const t_abnf_node& node );

		// repeat
		void 			lower_repeat( const t_abnf_node& node, std::vector<t_id_symbol>& v_rhs );
		t_id_symbol 	make_unbounded_repeat( t_id_symbol unit );
		t_id_symbol 	make_exact_repeat( t_id_symbol unit, size_t count, std::map<size_t, t_id_symbol>& m_cache );
		t_id_symbol 	make_bounded_repeat( t_id_symbol unit, size_t maximum, std::map<size_t, t_id_symbol>& m_exact_cache, std::map<size_t, t_id_symbol>& m_bounded_cache );
		size_t 			highest_power_of_two( size_t value ) const;

		// tools
		t_id_symbol 	find_rule_symbol( const t_source_span& span ) const;
		t_id_symbol 	new_synthetic_symbol( const std::string& prefix );
		std::string 	canonical_name( const t_source_span& span ) const;
		std::string 	canonical_name( const std::string& name ) const;
		std::string 	source_string( const t_source_span& span ) const;

	private: // null first : derived properties 
		void			build_null_first( void );

		void			compute_nullable( void );
		void			compute_first( void );

		bool			rule_is_nullable( const t_rule& rule ) const;
		bool			propagate_first_from_rule( const t_rule& rule );

	private: // validation
		void			validate_cfg( void );

		bool			rule_is_productive( const t_rule& rule ) const;

		void			compute_productive( void );
		void			compute_reachable( void );

		// getters
		bool			is_productive( t_id_symbol id ) const;
		bool			is_reachable( t_id_symbol id ) const;

		// error mgmt
		void			emit_diagnostics( void );

	private:
		// forbiden OCF
		CFGBuilder( void );
		CFGBuilder( const CFGBuilder& other );
		CFGBuilder& 	operator=( const CFGBuilder& other );
	
};
