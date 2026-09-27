// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstddef>
#include <stdexcept>

#include "../../grammar/CFG/CFG/CFG.hpp"
#include "../../automaton/LRGraph/LRGraph.hpp"
#include "../TableIR/TableIR.hpp"

class TableIRBuilder
{
	private:
		const CFG&			m_cfg_;
		const LRGraph&		m_lalr_;
		TableIR&			m_table_;

	public:
		// OCF
		TableIRBuilder( const CFG& cfg, const LRGraph& lalr, TableIR& table );
		~TableIRBuilder( void );

		void	build( void );

	private:
		void	build_transitions( void );
		void	build_reductions( void );

		bool	item_is_complete( const t_lr_item& item ) const;

	private:
		// fiorbiden OCF
		TableIRBuilder( void );
		TableIRBuilder( const TableIRBuilder& other );
		TableIRBuilder&  operator= ( const TableIRBuilder& other );

};
