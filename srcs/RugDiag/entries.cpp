// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#include "../../inc/RugDiag.hpp"

void	RugDiag::add( e_rug_diag_level level, bool has_span, 
					  size_t offset, size_t lenght, const std::string& message )
{
	t_rug_diag	diag;

	diag.level = level;
	diag.message = message;
	diag.has_span = has_span;
	diag.offset = offset;
	diag.lenght = lenght;

	mv_entries_.push_back(diag);

	if (level == RUG_DIAG_ERROR)
		m_error_count_++;
	else if (level == RUG_DIAG_WARNING)
		m_warning_count_++;
}

void	RugDiag::error( size_t offset, size_t length, const std::string& message )
{	add(RUG_DIAG_ERROR, true, offset, length, message);	}

void	RugDiag::error( const std::string& message )
{	add(RUG_DIAG_ERROR, false, 0, 0, message);	}

void	RugDiag::warning( size_t offset, size_t length, const std::string& message )
{	add(RUG_DIAG_WARNING, true, offset, length, message);	}

void	RugDiag::warning( const std::string& message )
{	add(RUG_DIAG_WARNING, false, 0, 0, message);	}

void	RugDiag::note( size_t offset, size_t length, const std::string& message )
{	add(RUG_DIAG_NOTE, true, offset, length, message);	}

void	RugDiag::note( const std::string& message )
{	add(RUG_DIAG_NOTE, false, 0, 0, message);	}