// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#pragma once

// #include "../CFG/CFG.hpp"

#include "../CFGTypes/CFGTypes.hpp"

#include <stdint.h>

class TerminalBitSet
{
	private:
		uint64_t	m_256_bits_[4];
	
	public:
		TerminalBitSet( void );
		~TerminalBitSet( void );

		TerminalBitSet ( const TerminalBitSet& other );
		TerminalBitSet& operator= ( const TerminalBitSet& other );

		bool	add( uint8_t terminal );
		bool	contains( uint8_t terminal ) const;
		bool	merge( const TerminalBitSet& other );
		void	clear( void );
		bool	empty( void ) const;

		bool	operator== ( const TerminalBitSet& other ) const;
};

class LookaheadSet
{
	private:
		TerminalBitSet	m_set_;
		bool			m_end_;

	public:
		LookaheadSet( void );
		~LookaheadSet( void );

		LookaheadSet ( const LookaheadSet& other );
		LookaheadSet& operator= ( const LookaheadSet& other );

		bool	add( t_id_symbol terminal );
		bool	contains( t_id_symbol terminal ) const;
		bool	merge( const LookaheadSet& other );
		bool	merge( const TerminalBitSet& other );
		void	clear( void );
		bool	empty( void ) const;

		bool	operator== ( const LookaheadSet& other ) const;
};
