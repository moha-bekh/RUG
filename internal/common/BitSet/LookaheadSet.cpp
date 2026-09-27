// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#include "BitSet.hpp"

bool	LookaheadSet::add( t_id_symbol terminal )
{
	if (terminal == CFG_END)
	{
		if (m_end_)
			return (false);
		
		m_end_ = true;
		return (true);
	}
	else
		return (m_set_.add((uint8_t)terminal));
}

bool	LookaheadSet::contains( t_id_symbol terminal )  const
{
	if (terminal == CFG_END)
		return (m_end_);
	else
		return (m_set_.contains((uint8_t)terminal));
}

bool	LookaheadSet::merge( const LookaheadSet& other )
{
	bool	changed = false;

	if (m_set_.merge(other.m_set_))
		changed = true;

	if (other.m_end_ && !m_end_)
	{
		m_end_ = true;
		changed = true;
	}

	return (changed);
}

bool	LookaheadSet::merge( const TerminalBitSet& other )
{
	bool	changed = false;

	if (m_set_.merge(other))
		changed = true;

	return (changed);
}

void	LookaheadSet::clear( void )
{
	m_set_.clear();
	m_end_ = false;
}

bool	LookaheadSet::empty( void ) const
{	return (!m_end_ && m_set_.empty());	}

/* OCF */

LookaheadSet::LookaheadSet( void )
:	m_set_(), m_end_(false)
{	}

LookaheadSet::~LookaheadSet( void )
{	}

LookaheadSet::LookaheadSet( const LookaheadSet& other )
{	*this = other;	}

LookaheadSet& LookaheadSet::operator= ( const LookaheadSet& other )
{
	if (this != &other)
	{
		m_set_ = other.m_set_;
		m_end_ = other.m_end_;
	}
	return (*this);
}

bool	LookaheadSet::operator== ( const LookaheadSet& other ) const
{	return (m_set_ == other.m_set_ && m_end_ == other.m_end_);	}