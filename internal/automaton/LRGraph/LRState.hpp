// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <vector>

#include "LRTypes.hpp"

class LRState
{
	private:
		std::vector<t_lr_item>	mv_items_;

	public:
		// OCF
		LRState( void );
		~LRState( void );
		LRState( const LRState& other );
		LRState& operator= ( const LRState& other );
		
		size_t				item_count( void ) const;
		const t_lr_item&	item( size_t index ) const;

		size_t				find_core( const t_lr_item_core& core ) const;

		bool				merge_item( const t_lr_item_core& core, const LookaheadSet& lookaheads );
		bool				same_as( const LRState& other ) const;
		bool				same_core_as( const LRState& other ) const;

};
