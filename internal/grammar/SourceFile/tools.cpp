// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#include "SourceFile.hpp"

const char*	SourceFile::data( void ) const
{	return (&m_data_[0]);	}

size_t		SourceFile::size( void ) const
{	return (m_data_.size());	}

size_t		SourceFile::line_start( size_t line ) const
{
	if (line == 0 || line > m_v_ln_idx_.size())
		throw std::runtime_error("SourceFile::line_start() : out of bounds");
	return (m_v_ln_idx_[line - 1]);
}

size_t		SourceFile::line_end( size_t line ) const
{
	size_t	start;
	size_t	end;

	if (line == 0 || line > m_v_ln_idx_.size())
		throw std::runtime_error("SourceFile::line_end() : out of bounds");
	
	start = m_v_ln_idx_[line - 1];

	if (line < m_v_ln_idx_.size())
		end = m_v_ln_idx_[line];
	else
		end = m_data_.size();
	
	if (end > start && m_data_[end - 1] == '\n')
		end--;
	if (end > start && m_data_[end - 1] == '\r')
		end--;
	
	return (end);
}
