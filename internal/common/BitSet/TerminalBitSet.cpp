// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#include "BitSet.hpp"

bool	TerminalBitSet::add( uint8_t terminal )
{
	const size_t	word_id = terminal / 64;
	const uint64_t	mask = ((uint64_t)1 << terminal % 64);

	// if terminal already in word we return (false) / indicator not changed
	if (m_256_bits_[word_id] & mask)
		return (false);

	m_256_bits_[word_id] |= mask;

	return (true);
}

bool	TerminalBitSet::contains( uint8_t terminal ) const
{	return (m_256_bits_[terminal / 64] & ((uint64_t)1 << (terminal % 64)));	}

/*
	will return true if one terminal of *this is changed
*/
bool	TerminalBitSet::merge( const TerminalBitSet& other )
{
	uint64_t	old;
	bool		changed = false;

	for (size_t i = 0; i < 4; i++)
	{
		old = m_256_bits_[i];

		m_256_bits_[i] |= other.m_256_bits_[i];

		if (old != m_256_bits_[i])
			changed = true;
	}

	return (changed);
}

void	TerminalBitSet::clear( void )
{
	m_256_bits_[0] = 0;
	m_256_bits_[1] = 0;
	m_256_bits_[2] = 0;
	m_256_bits_[3] = 0;
}

bool	TerminalBitSet::empty( void ) const
{
	return (m_256_bits_[0] == 0 && \
			m_256_bits_[1] == 0 && \
			m_256_bits_[2] == 0 && \
			m_256_bits_[3] == 0);
}

bool	TerminalBitSet::operator== ( const TerminalBitSet& other ) const
{
	return (m_256_bits_[0] == other.m_256_bits_[0] &&
			m_256_bits_[1] == other.m_256_bits_[1] &&
			m_256_bits_[2] == other.m_256_bits_[2] &&
			m_256_bits_[3] == other.m_256_bits_[3]);
}

/* OCF */
TerminalBitSet::TerminalBitSet( void )
{	clear();	}

TerminalBitSet::~TerminalBitSet( void )
{	}

TerminalBitSet::TerminalBitSet( const TerminalBitSet& other )
{	*this = other;	}

TerminalBitSet& TerminalBitSet::operator= ( const TerminalBitSet& other )
{
	if (this != &other)
	{
		m_256_bits_[0] = other.m_256_bits_[0];
		m_256_bits_[1] = other.m_256_bits_[1];
		m_256_bits_[2] = other.m_256_bits_[2];
		m_256_bits_[3] = other.m_256_bits_[3];
	}
	return (*this);
}
