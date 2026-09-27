// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#include "LRState.hpp"

LRState::LRState( void )
:	mv_items_()
{	}

LRState::~LRState( void )
{	}

LRState::LRState( const LRState& other )
{	*this = other;	}

LRState& LRState::operator= ( const LRState& other )
{
	if (this != &other)
		mv_items_ = other.mv_items_;
	return (*this);
}

size_t	LRState::item_count( void ) const
{	return (mv_items_.size());	}

const t_lr_item&	LRState::item( size_t index ) const
{	return (mv_items_[index]);	}

size_t	LRState::find_core( const t_lr_item_core& core ) const
{
	for (size_t i = 0; i < mv_items_.size(); i++)
	{
		if (mv_items_[i].core.rule == core.rule && \
			mv_items_[i].core.dot == core.dot)
			return (i);
	}

	// return sentinell not found
	return (mv_items_.size());
}

/*
	can pushback to mv_items_ if item doesn exist
*/
bool	LRState::merge_item( const t_lr_item_core& core, const LookaheadSet& lookaheads )
{
	t_lr_item	item;
	size_t		index = find_core(core);

	// not found SO we create new item
	if (index == mv_items_.size())
	{
		item.core = core;
		item.lookaheads = lookaheads;

		mv_items_.push_back(item);

		return (true);
	}

	// else we merge with corresponding core
	return (mv_items_[index].lookaheads.merge(lookaheads));
}

bool	LRState::same_as( const LRState& other ) const
{
	size_t	index;

	if (mv_items_.size() != other.mv_items_.size())
		return (false);

	for (size_t i = 0; i < mv_items_.size(); i++)
	{
		index = other.find_core(mv_items_[i].core);

		if (index == other.mv_items_.size())
			return (false);
		
		// operator== overload
		if (!(mv_items_[i].lookaheads == other.mv_items_[index].lookaheads))
			return (false);
	}

	return (true);
}

bool	LRState::same_core_as( const LRState& other ) const
{
	if (mv_items_.size() != other.mv_items_.size())
		return (false);

	for (size_t i = 0; i < mv_items_.size(); i++)
	{
		// if not found the core elemt are not the same
		if (other.find_core(mv_items_[i].core) == other.mv_items_.size())
			return (false);
	}

	return (true);
}
