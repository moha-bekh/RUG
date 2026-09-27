// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstddef>
#include <stdint.h>
#include <stdexcept>

#include "../Table/TableTypes.hpp"

class TableView
{
	private:
		const uint8_t			*m_base_;
		const t_table_blob_hdr	*m_hdr_;
		size_t					m_capacity_;

	public:
		// OCF
		TableView( const void *blob, size_t capacity );
		~TableView( void );

		// metadata
		const t_table_blob_hdr&	header( void ) const;
		size_t					blob_size( void ) const;
		size_t					state_count( void ) const;
		size_t					non_terminal_count( void ) const;
		size_t					action_count( void ) const;
		size_t					goto_count( void ) const;
		size_t					rule_count( void ) const;
		size_t					semantic_count(void) const;

		// runtime lookup
		t_action				action( t_id_state state, t_id_symbol terminal ) const;
		t_id_state				goto_state( t_id_state state, t_id_symbol non_terminal ) const;
		const t_rule_entry&		rule( size_t rule_id ) const;

		// sparse access
		const t_action_span&	action_span( t_id_state state ) const;
		const t_action_entry&	action_entry( size_t index ) const;
		const t_blob_span&		goto_span( t_id_state state ) const;
		const t_goto_entry&		goto_entry( size_t index ) const;

	private:
		// section pointers
		const t_action_span		*action_spans_ptr( void ) const;
		const t_action_entry	*action_entries_ptr( void ) const;
		const t_blob_span		*goto_spans_ptr( void ) const;
		const t_goto_entry		*goto_entries_ptr( void ) const;
		const t_rule_entry		*rule_entries_ptr( void ) const;

		// validation (optionnal)
		void					validate( void ) const;
		void					validate_state( t_id_state state ) const;
};