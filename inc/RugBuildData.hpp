// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstddef>
#include <string>
#include <vector>
#include <stdint.h>

#include "../internal/includes/runtime.hpp"

typedef struct			s_pending_binding
{
	std::string			name;
	t_reduce_callback	callback;
}						t_pending_binding;

class RugBuildData
{
	public:
		std::string						m_abnf_file_;
		std::vector<t_pending_binding>	mv_bindings_;
		std::vector<uint8_t>			mv_parsing_table_;
		std::vector<t_reduce_callback>	mv_callback_table_;

		size_t							m_callbacks_offset_;
		size_t							m_required_size_;
		
		bool							mb_prepared_;

		RugBuildData( const std::string& abnf_file )
		:	m_abnf_file_(abnf_file),
			mv_bindings_(),
			mv_parsing_table_(),
			mv_callback_table_(),
			m_callbacks_offset_(0),
			m_required_size_(0),
			mb_prepared_(false)
		{	}

		~RugBuildData( void )
		{	}
};