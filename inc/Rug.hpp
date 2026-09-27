// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstddef>
#include <vector>
#include <cstring>
#include <new>
#include <stdexcept>
#include <iostream>
#include <cctype>

#include <sys/mman.h>
#include "../internal/includes/automaton.hpp"
#include "../internal/includes/grammar.hpp"
#include "../internal/includes/runtime.hpp"
#include "../internal/includes/table.hpp"

#include "../inc/RugBuildData.hpp"

class RugSession;

/* sorry */
/*
	this union syntax is a "hack" to get aligned
	raw storage for delayed placement contructions

	c++98 doesnt support alignas/alignof

	the element is constructed in data[] with placement-new
	and must therefore be destroyed explicitly
*/
union u_rug_table
{
	long double	align_double_long;
	void		*align_ptr;
	char		data[sizeof(TableView)];
};

union u_rug_semantic
{
	long double	align_double_long;
	void		*align_ptr;
	char		data[sizeof(SemanticRuntime)];
};

class Rug
{
	private:
		/* class RugSession will access private elements of Rug */
		friend class RugSession;

		/*
			Necessry for build, will be destructed after build
			Runtime is unafected
		*/
		RugBuildData	*m_build_data_;

		/*
			final immutable runtime image layout:

			[ parsing table ]
			[ allignment padding ]
			[ callback table ]
		*/
		void			*m_image_;
		size_t			m_image_size_;
		size_t			m_parsing_table_size_;
		size_t			m_callbacks_offset_;
		size_t			m_callback_count_;

		/* status */
		bool			mb_owns_memory_; // mode 1 auto allocated
		// bool			mb_prepared_; // RugBuildData IR is ready
		bool			mb_built_; // finalized
		
		// raw storage
		u_rug_table		m_table_storage_;
		u_rug_semantic	m_semantic_storage_;

		/*
			once the storage uniosn are built 
			we can assign this accession pointers
		*/
		TableView		*m_table_;
		SemanticRuntime	*m_semantics_;

	public:
		/* OCF */
		Rug( const std::string& abnf_file )
		:	m_build_data_(new RugBuildData(abnf_file)),
			m_image_(NULL),
			m_image_size_(0),
			m_parsing_table_size_(0),
			m_callbacks_offset_(0),
			m_callback_count_(0),
			mb_owns_memory_(false),
			mb_built_(false),
			m_table_storage_(),
			m_semantic_storage_(),
			m_table_(NULL),
			m_semantics_(NULL)
		{	}

		~Rug( void )
		{
			destroy_runtime_views();

			if (mb_owns_memory_ && m_image_)
				munmap(m_image_, m_image_size_);

			delete m_build_data_;
		}

		/*
			semantic bindings must be called before build
		*/
		void		bind( const std::string& rule_name, t_reduce_callback function );

		/*
			Mode 1 : 
			auto allocated
		*/
		void		build( void );
		
		/*
			Mode 2 : 
			the user must get required_alloc_size() 
			and assign it, with build_into(void *addr, size_t capacity )

			required_alloc_size() func will compile the IR (if necessary)
		*/
		size_t		required_alloc_size( void );
		void		build_into( void *addr, size_t capacity );

		/* getters */

		bool		is_built( void ) const;
		bool		owns_memory( void ) const;

	private:
		size_t		image_size( void ) const;
		size_t		parsing_table_size( void ) const;
		size_t		semantic_count( void ) const;

		void		prepare( void );
		void		finalize_into( void *addr, size_t size, bool owns_workspace );
		void		destroy_runtime_views( void );

		// accessible by RugSession (friend kw)
		const TableView&		table(void) const;
		const SemanticRuntime&	semantics(void) const;

	private:
		/* forbiden OCF */

		Rug(void);
		Rug(const Rug& other);
		Rug& operator=(const Rug& other);
};

// helpers static inline
static inline size_t		align_up( size_t value, size_t alignement )
{
	size_t	remainder = value % alignement;

	if (remainder == 0)
		return (value);

	size_t	padding = alignement - remainder;

	if (value > (size_t)-1 - padding)
		throw std::overflow_error("align_up: size overflow");

	return (value + padding);
}

static inline std::string	get_start_rule( const SourceFile& src, const ABNFIR& ir)
{
	if (ir.rule_count() == 0)
		throw std::logic_error("Rug: grammar contains no rules");

	const t_source_span& span = ir.rule(0).name;

	return (std::string(src.data() + span.offset, span.len));
}

static inline void check_diagnostics( DiagnosticEngine& diag, const char *message)
{	diag.throw_if_errors(message);	}

// printer
static inline void	print_terminal(std::ostream& out, t_id_symbol terminal)
{
	if (terminal == '\r')
	{
		out << "'\\r'";
		return ;
	}

	if (terminal == '\n')
	{
		out << "'\\n'";
		return ;
	}

	if (terminal == '\t')
	{
		out << "'\\t'";
		return ;
	}

	if (terminal <= 255 && std::isprint((unsigned char)terminal))
	{
		out << "'" << (char)terminal << "'";
		return ;
	}

	out << "<" << terminal << ">";
}


static inline void	print_parse_error( std::ostream& out, const t_parse_error& error )
{
	out << "Syntax Error : Unexpected Token" << std::endl;
	out << "  offset   : " << error.offset << std::endl;
	out << "  state    : " << error.state << std::endl;
	out << "  got terminal : ";

	print_terminal(out, error.terminal);

	out << " (" << error.terminal << ")" << std::endl;
	out << "  span     : [" << error.span.begin;
	out << ", " << error.span.end << ")" << std::endl;
}