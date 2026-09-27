// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#include "SemanticBindings.hpp"


SemanticBindings::SemanticBindings( const CFG& cfg, t_reduce_callback *callbacks, size_t callback_capacity )
:	m_cfg_(cfg),
	m_callbacks_(callbacks),
	m_callback_capacity_(callback_capacity),
	m_callback_count_(0),
	mm_semantic_ids_()
{
	if (!m_callbacks_ && m_callback_capacity_ != 0)
		throw std::logic_error("SemanticBindings: null callback table");
}


SemanticBindings::~SemanticBindings( void )
{	}


uint32_t	SemanticBindings::bind( const std::string& non_terminal, t_reduce_callback callback )
{
	if (!callback)
		throw std::logic_error("SemanticBindings::bind: null callback");


	if (m_callback_count_ >= m_callback_capacity_)
		throw std::logic_error("SemanticBindings::bind: callback table full");

	t_id_symbol symbol;

	if (!m_cfg_.find_non_terminal(non_terminal, symbol))
		throw std::logic_error("SemanticBindings::bind: unknown non-terminal");

	std::map<t_id_symbol, uint32_t>::const_iterator existing = mm_semantic_ids_.find(symbol);

	if (existing != mm_semantic_ids_.end())
		throw std::logic_error("SemanticBindings::bind: non-terminal already bound");
	if (m_callback_count_ >= (size_t)PARSING_NO_SEMANTIC)
		throw std::logic_error("SemanticBindings::bind: semantic id overflow");

	uint32_t semantic_id = (uint32_t)m_callback_count_;

	// push to callback array
	m_callbacks_[semantic_id] = callback;
	m_callback_count_++;

	// build time association
	mm_semantic_ids_.insert(std::make_pair(symbol, semantic_id));

	return (semantic_id);
}


uint32_t	SemanticBindings::semantic_id( t_id_symbol lhs ) const
{
	std::map<t_id_symbol, uint32_t >::const_iterator it = mm_semantic_ids_.find(lhs);

	if (it == mm_semantic_ids_.end())
		return (PARSING_NO_SEMANTIC);

	return (it->second);
}


size_t	SemanticBindings::semantic_count( void ) const
{	return (m_callback_count_);	}