// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#include "SemanticRuntime.hpp"

SemanticRuntime::SemanticRuntime( const t_reduce_callback *callbacks, size_t callback_count )
:	m_callbacks_(callbacks),
	m_callback_count_(callback_count)
{
	if (!m_callbacks_ && m_callback_count_ != 0)
		throw std::logic_error("SemanticRuntime: null callback table");
}

SemanticRuntime::~SemanticRuntime(void)
{	}

t_semantic_result	SemanticRuntime::dispatch(uint32_t semantic_id, const t_reduce_event& event, void *user_context) const
{
	// already checker by Session::dispatch
	// if (semantic_id == PARSING_NO_SEMANTIC)
	// 	return (SEMANTIC_OK);

	t_reduce_callback callback = m_callbacks_[semantic_id];

	return (callback(event, user_context));
}

size_t	SemanticRuntime::count(void) const
{	return (m_callback_count_);	}