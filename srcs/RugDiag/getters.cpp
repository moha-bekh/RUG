// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#include "../../inc/RugDiag.hpp"

bool				RugDiag::has_errors( void ) const
{	return (m_error_count_ != 0);	}

size_t				RugDiag::error_count( void ) const
{	return (m_error_count_);	}

size_t				RugDiag::warning_count( void ) const
{	return (m_warning_count_);	}

size_t				RugDiag::count( void ) const
{	return (mv_entries_.size());	}

const t_rug_diag&	RugDiag::entry( size_t index ) const
{	return (mv_entries_[index]);	}

