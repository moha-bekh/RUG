// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstddef>
#include <stdint.h>
#include <stdexcept>

#include "../../common/ParserTypes/ParserTypes.hpp"
#include "../../table/Table/TableTypes.hpp"

class SemanticRuntime
{
	private:
		const t_reduce_callback	*m_callbacks_;
		size_t					m_callback_count_;

	public:
		SemanticRuntime(const t_reduce_callback *callbacks, size_t callback_count);
		~SemanticRuntime(void);

		t_semantic_result	dispatch(uint32_t semantic_id, const t_reduce_event& event, void *user_context) const;

		size_t	count(void) const;

	private:
		SemanticRuntime(void);
		SemanticRuntime(const SemanticRuntime& other);
		SemanticRuntime& operator=(const SemanticRuntime& other);
};