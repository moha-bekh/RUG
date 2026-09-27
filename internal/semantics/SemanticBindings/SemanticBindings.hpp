// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstddef>
#include <stdint.h>
#include <map>
#include <string>
#include <stdexcept>

#include "../../grammar/CFG/CFG/CFG.hpp"
#include "../../common/ParserTypes/ParserTypes.hpp"
#include "../../table/Table/TableTypes.hpp"


class SemanticBindings
{
	private:
		const CFG&			m_cfg_;

		// array for function pointers
		t_reduce_callback	*m_callbacks_;

		size_t				m_callback_capacity_;
		size_t				m_callback_count_;

		// build time only no runtime
		std::map<t_id_symbol, uint32_t> mm_semantic_ids_;


	public:
		SemanticBindings( const CFG& cfg, t_reduce_callback *callbacks, size_t callback_capacity );
		~SemanticBindings( void );

		uint32_t	bind( const std::string& non_terminal, t_reduce_callback callback );
		uint32_t	semantic_id( t_id_symbol lhs ) const;
		size_t		semantic_count( void ) const;

	private:
		SemanticBindings( void );
		SemanticBindings( const SemanticBindings& other );
		SemanticBindings& operator=( const SemanticBindings& other );
};