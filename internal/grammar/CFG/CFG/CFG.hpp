// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <vector>
#include <string>
#include <cstddef>
#include <stdint.h>
#include <stdexcept>
#include <map>
#include <string>

#include "../../SourceFile/SourceFile.hpp"
#include "../../../common/BitSet/BitSet.hpp"

#include "../../../common/CFGTypes/CFGTypes.hpp"
// typedef size_t t_id_symbol;
// typedef size_t t_id_rule;

// static const t_id_symbol	CFG_INVALID_SYMBOL			= (size_t)-1;
// static const t_id_rule		CFG_INVALID_RULE			= (size_t)-1;
// static const t_id_symbol	CFG_END						= 256; // logical end / this is not real socket/file EOF 
// static const t_id_symbol	CFG_FIRST_NON_TERMINAL		= 257;

typedef struct	s_slice
{
	size_t		begin;
	size_t		count;
}				t_slice;

typedef struct	s_rule
{
	t_id_symbol	lhs;
	t_slice		rhs_slice;
}				t_rule;

typedef struct 	s_debug_name
{
	t_id_symbol	id;
	std::string	name;
}				t_debug_name;

class CFG
{
	private:
		// flattened productions
		// epsilon is represneted by rhs_slice.count = 0
		std::vector<t_rule>			mv_rules_;

		// rhs symbol flat storage 
		// each rule owns a slice in this vector
		// terminal are from 0 to 256
		// non terminal are all id > 256
		// CFG_END is 256
		std::vector<t_id_symbol>	mv_rhs_;

		// grouping production of a same rule
		// for fast access during LR(1)/LALR build
		std::vector<t_id_rule>		mv_sorted_rule_ids_; // sorted by lhs
		std::vector<t_slice>		mv_same_lhs_slices_; // slices in the sorted vector

		/* Derived Propeties */
		std::vector<bool>			mv_nullable_;
		std::vector<TerminalBitSet>	mv_first_;

		// helpers for compiler error output
		std::vector<t_source_span>	mv_source_spans_;
		std::vector<bool>			mv_has_source_span_;

		// non essential : create debug names for synthtic non-terminal symbol
		std::vector<t_debug_name>	mv_debug_names_;

		// metadata
		size_t						m_non_terminal_count_;
		t_id_symbol					m_start_symbol_;

		// for semantic binding (dont worry its at Parser Section / much much much later)
		std::map<std::string, t_id_symbol>	mm_named_non_terminals_;

	public:
		/* OCF */
		CFG( void );
		~CFG( void );

		// build
		void					group_productions( void ); // finalize
		// once group_productions is called, the CFG becomes immutable
		// do not call any of the following build functrion after it
		void					set_start_symbol( t_id_symbol id );
		t_id_symbol 			add_non_terminal( void );
		t_id_rule 				add_rule( t_id_symbol lhs, const std::vector<t_id_symbol>& v_rhs );
		t_id_rule 				add_rule( t_id_symbol lhs, t_id_symbol unary_rhs_id );
		t_id_rule				add_empty_rule( t_id_symbol lhs );

		// symbol tools
		t_id_symbol 			terminal( uint8_t byte ) const;
		t_id_symbol 			end_symbol( void ) const;
		bool 					is_terminal( t_id_symbol id ) const;
		bool 					is_non_terminal( t_id_symbol id ) const;
		bool 					is_end( t_id_symbol id ) const;
		bool 					is_valid_symbol( t_id_symbol id ) const;

		// navigation CFG
		t_id_symbol				start_symbol( void ) const;
		const t_rule&			rule( t_id_rule id ) const;
		t_id_symbol				rhs_symbol( const t_rule& rule, size_t index ) const;

		size_t					rule_count( void ) const;
		size_t					non_terminal_count( void ) const;

		// fast access getters for grouped rule productions
		size_t					rule_count_for_same_lhs( t_id_symbol lhs ) const;
		t_id_rule				rule_id_for_lhs_at( t_id_symbol lhs, size_t index ) const;

		// set derived properties
		void					build_properties_storage( void );
		bool					mark_nullable( t_id_symbol non_terminal_id );
		bool					add_first( t_id_symbol non_terminal_id, uint8_t terminal );
		bool					merge_first( t_id_symbol non_terminal_id, const TerminalBitSet& set );

		// get derived properties
		const TerminalBitSet&	first_set( t_id_symbol non_terminal_id ) const;
		bool					is_nullable( t_id_symbol id ) const;
		bool					first_contains( t_id_symbol non_terminal, t_id_symbol terminal ) const;
		bool					first_of_rhs( const t_rule& rule, size_t begin, TerminalBitSet& out ) const;

		// source metadata
		void 					set_source_span( t_id_symbol id, const t_source_span& span );
		bool 					has_source_span( t_id_symbol id ) const;
		const t_source_span& 	source_span( t_id_symbol id ) const;

		// debug mode : create a synthtique name to synthetique rule
		void 					set_debug_name( t_id_symbol id, const std::string& name );
		bool 					has_debug_name( t_id_symbol id ) const;
		const std::string& 		debug_name( t_id_symbol id ) const;

		// semanic registry
		void	register_non_terminal_name( const std::string& name, t_id_symbol id );
		bool	find_non_terminal( const std::string& name, t_id_symbol& id ) const;

	private:
		/* forbiden OCF */
		CFG( const CFG& other );
		CFG& operator=( const CFG& other );

};
