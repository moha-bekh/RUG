// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstddef>
#include <stdint.h>
#include <stdexcept>
#include <cstring>

#include "../TableIR/TableIR.hpp"
#include "../../common/CFGTypes/CFGTypes.hpp"

#include "TableTypes.hpp"

#include "../../semantics/SemanticBindings/SemanticBindings.hpp"

class TableBuilder
{
	private:
		const CFG&				m_cfg_;
		const TableIR&			m_ir_;
		const SemanticBindings	*m_semantics_;

		size_t			m_action_count_;
		size_t			m_goto_count_;

		size_t			m_action_spans_offset_;
		size_t			m_action_entries_offset_;

		size_t			m_goto_spans_offset_;
		size_t			m_goto_entries_offset_;

		size_t			m_rule_entries_offset_;

		size_t			m_required_size_;

	public:
		TableBuilder( const CFG& cfg, const TableIR& ir );
		TableBuilder( const CFG& cfg, const TableIR& ir, const SemanticBindings& semantics );
		~TableBuilder( void );

		size_t	required_size( void ) const;

		void	build_into( void* memory, size_t capacity ) const;

	private:
		void	compute_layout( void );
		void	validate_layout( void ) const;

		size_t	align4( size_t value ) const;

	private:
		TableBuilder( void );
		TableBuilder( const TableBuilder& other );
		TableBuilder& operator= ( const TableBuilder& other );

};

static inline bool same_action( const t_action& a, const t_action& b )
{	return (a.type == b.type && a.value == b.value );	}

/*
	Find the most common action in one complete ACTION ROW

	there are only 256 terminals (CFG_END + 1) so this simple search algo is acceptable
	this runs at generation, never at parser runtime
*/

static inline t_action find_default_action( const TableIR& ir, size_t state )
{
	t_action	best;
	size_t		best_count;

	best = ir.action(state, 0);
	best_count = 0;

	// sym == candidate
	for (t_id_symbol sym = 0; sym <= CFG_END; sym++)
	{
		const t_action&	action = ir.action(state, sym);
		size_t			count = 0;

		for (t_id_symbol terminal = 0; terminal <= CFG_END; terminal++)
		{
			if (same_action(action, ir.action(state, terminal)))
				count++;
		}
		if (count > best_count)
		{
			best = action;
			best_count = count;
		}
	}

	return (best);
}


/* Arithmetic checks */

static inline uint32_t	checked_u32( size_t val, const char *what )
{
	if (val > UINT32_MAX)
		throw std::overflow_error(what);
	return ((uint32_t)val);
}

static inline size_t	checked_u64_mul( size_t a, size_t b, const char *what )
{
	if (a != 0 && b > SIZE_MAX / a)
		throw std::overflow_error(what);
	return (a * b);
}

static inline size_t	checked_u64_add( size_t a, size_t b, const char *what )
{
	if (b > SIZE_MAX - a)
		throw std::overflow_error(what);
	return (a + b);
}