// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#include "../../inc/RugDiag.hpp"

RugDiag::RugDiag( const std::string& name )
:	m_name_(name),
	m_source_(),
	mb_has_source_(false),
	mv_line_starts_(),
	mv_entries_(),
	m_error_count_(0),
	m_warning_count_(0)
{	}

RugDiag::RugDiag( const std::string& name, const char *data, size_t size)
:	m_name_(name),
	m_source_(),
	mb_has_source_(true),
	mv_line_starts_(),
	mv_entries_(),
	m_error_count_(0),
	m_warning_count_(0)
{
	if (!data && size != 0)
		throw std::invalid_argument("RugDiag: null source with non-zero size");

	if (size != 0)
		m_source_.assign(data, size);

	build_line_index();
}