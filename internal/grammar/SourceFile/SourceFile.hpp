// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <stdexcept>
#include <string>
#include <fstream>
#include <iostream>
#include <vector>

/* 
	lets remove this line and col from the source span 
	we can find the orgin from the line start vector in srcFile
*/
typedef struct	s_source_span
{
	size_t		offset;
	size_t		len;
	size_t		line;
	size_t		column;
}				t_source_span;

class SourceFile
{
	private:
		/* owned */
		std::string			m_data_;
		std::vector<size_t>	m_v_ln_idx_;
	
	public:
		/* Orthodox Canonical Form */
		SourceFile( const char *filename );
		~SourceFile( void );

		size_t		line_start( size_t line ) const;
		size_t		line_end( size_t line ) const;
	
		const char*	data( void ) const;
		size_t		size( void ) const;
	
	private:
		void		build_line_idx_vec( void );
	
	private:
		/* forbiden OCF */
		SourceFile( void );
		SourceFile( const SourceFile& other ) ;
		SourceFile&	operator= ( const SourceFile& other );
	
};

