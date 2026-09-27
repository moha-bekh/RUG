// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "cstddef"

#include "Rug.hpp"
#include "../internal/includes/runtime.hpp"

union u_rug_session
{
	long double	align_double_long;
	void		*align_ptr;
	char		data[sizeof(Session)];
};

class RugSession
{
	private:
		const Rug&		m_rug_;

		void			*m_workspace_;
		size_t			m_workspace_size_;
		size_t			m_stack_capacity_;

		bool			mb_owns_workspace_;

		// how many bytes where consumed
		size_t			m_input_offset_;

		u_rug_session	m_session_storage_;
		Session			*m_session_;

	public:
		/* Mode 1: auto allocated */
		RugSession( const Rug& rug, void *user_ctx, size_t stack_capacity );

		/* Mode 2: externally allocated memory */
		RugSession( const Rug& rug, void *user_ctx, void *workspace, size_t workspace_size );

		~RugSession( void );

		// exact memory size needed for a wanted stack capacity
		static size_t			workspace_size( size_t stack_capacity );


		// frontend byte-oriented
		e_parse_result			push( const unsigned char *data, size_t size);
		/* a token frontend could be imagined in the future */

		e_parse_result			finish( void );
		void					reset( void );

		// getters 
		size_t					input_offset( void ) const;
		size_t					stack_capacity( void ) const;

		const t_parse_error&	syntax_error( void ) const;
		const t_semantic_error&	semantic_error( void ) const;

	private:
		void					init( void *user_ctx, void *workspace, size_t workspace_size );
		static size_t			capacity_from_workspace( size_t available );

	private:
		RugSession(const RugSession& other);
		RugSession& operator=(const RugSession& other);


};