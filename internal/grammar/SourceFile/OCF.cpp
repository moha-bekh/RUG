// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#include "SourceFile.hpp"

SourceFile::SourceFile( const char *filename )
{
	std::streampos	size;
	std::ifstream	ifs(filename, std::ios::in | std::ios::binary);

	if (!ifs)
		throw std::runtime_error("ifstream() can not open file");
	
	ifs.seekg(0, std::ios::end);
	size = ifs.tellg();
	if (size < 0)
		throw std::runtime_error("ifstream().tellg() return error -1");
	
	ifs.seekg(0, std::ios::beg);
	m_data_.resize((size_t)size);
	ifs.read(&m_data_[0], size);

	if (!(ifs.good() || ifs.eof()))
		throw std::runtime_error("ifstream().good() || ifstream().eof() failed");

	build_line_idx_vec();
}


SourceFile::~SourceFile( void )
{	}

void		SourceFile::build_line_idx_vec( void )
{
	m_v_ln_idx_.clear();

	m_v_ln_idx_.push_back(0);

	for (size_t i = 0; i < m_data_.size(); i++)
	{
		if (m_data_[i] == '\n')
			m_v_ln_idx_.push_back(i + 1);
		else if (m_data_[i] == '\r' && i + 1 < m_data_.size() && m_data_[i + 1] == '\n')
		{
			m_v_ln_idx_.push_back(i + 2);
			i++;
		}
	}
}
