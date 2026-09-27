// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#include "TableBuilder.hpp"

size_t	TableBuilder::align4( size_t value ) const
{
	if (value > SIZE_MAX - 3)
		throw std::overflow_error("TbleBuilder::align4 overflow");
	return ((value + 3) & ~(size_t)3);
}

size_t	TableBuilder::required_size( void ) const
{	return (m_required_size_);	}

void	TableBuilder::build_into( void *memory, size_t capacity ) const
{
	uint8_t				*base;
	t_table_blob_hdr	*hdr;

	base = (uint8_t*)memory;

	if (!base)
		throw std::logic_error("TableBuilder::build_into: null addr");
	if (capacity < m_required_size_)
		throw std::logic_error("TableBuilder::build_into: insufficient mem size");
	
	std::memset(base, 0, m_required_size_);

	hdr = (t_table_blob_hdr*)base;

	hdr->total_size = checked_u32(m_required_size_, "TableBuilder: blob size > uint32_t");
	hdr->state_count = checked_u32(m_ir_.state_count(), "TableBuilder: too many states");
	hdr->non_terminal_count = checked_u32(m_ir_.non_terminal_count(), "TableBuilder: too many non terminals");
	hdr->rule_count = checked_u32(m_cfg_.rule_count(), "TableBuilder: too many rules");
	hdr->semantic_count = checked_u32(m_semantics_ ? m_semantics_->semantic_count() : 0, "TableBuilder: too many semantic names");
	hdr->action_count = checked_u32(m_action_count_, "TableBuilder: too many actions");
	hdr->goto_count = checked_u32(m_goto_count_, "TableBuilder: too many gotos");
	hdr->action_spans_offset = checked_u32(m_action_spans_offset_, "TableBuilder: action spans offset overflow");
	hdr->action_entries_offset = checked_u32(m_action_entries_offset_, "TableBuilder: action entries offset overflow");
	hdr->goto_spans_offset = checked_u32(m_goto_spans_offset_, "TableBuilder: goto spans offset overflow");
	hdr->goto_entries_offset = checked_u32(m_goto_entries_offset_, "TableBuilder: goto entries offset overflow");
	hdr->rule_entries_offset = checked_u32(m_rule_entries_offset_,"TableBuilder: rule entries offset overflow");

	t_action_span	*action_spans = (t_action_span*)(base + m_action_spans_offset_);
	t_action_entry	*action_entries = (t_action_entry*)(base + m_action_entries_offset_);
	t_blob_span		*goto_spans = (t_blob_span*)(base + m_goto_spans_offset_);
	t_goto_entry	*goto_entries = (t_goto_entry*)(base + m_goto_entries_offset_);
	t_rule_entry	*rule_entries = (t_rule_entry*)(base + m_rule_entries_offset_);
	
	/*
		ACTION Table
		each store : default_action + exeptions
	*/
	uint32_t		action_index = 0;

	for (size_t state = 0; state < m_ir_.state_count(); state++)
	{
		t_action default_action = find_default_action(m_ir_, state);

		action_spans[state].offset = checked_u32(action_index, "TableBuilder: action index overflow");
		action_spans[state].count = 0;
		action_spans[state].default_action = pack_action(default_action.type, default_action.value);

		size_t exception_count = 0;

		for (t_id_symbol term = 0; term <= CFG_END; term++)
		{
			const t_action& action = m_ir_.action(state, term);

			if (same_action(action, default_action))
				continue ;

			action_entries[action_index].terminal = (uint32_t)term;
			action_entries[action_index].action = pack_action(action.type, action.value);
			action_index++;
			exception_count++;
		}
		action_spans[state].count = checked_u32(exception_count, "TableBuilder: action span count overflow");
	}

	if (action_index != m_action_count_)
		throw std::logic_error("TableBuilder: action count mismatch");


	/* GOTO TABEL */
	uint32_t	goto_index = 0;

	for (size_t state = 0; state < m_ir_.state_count(); state++)
	{
		goto_spans[state].offset = checked_u32(goto_index, "TableBuilder: goto index overflow");
		goto_spans[state].count = 0;

		size_t goto_state_count = 0;

		for (size_t nt = 0; nt < m_ir_.non_terminal_count(); nt++)
		{
			t_id_symbol sym = CFG_FIRST_NON_TERMINAL + nt;
			t_id_state	tgt = m_ir_.goto_state(state, sym);

			if (tgt == PARSING_INVALID_GOTO)
				continue ;

			goto_entries[goto_index].non_terminal = checked_u32(nt, "TableBuilder: non terminal index overflow");
			goto_entries[goto_index].target = checked_u32(tgt, "TableBuilder: non terminal index overflow");

			goto_index++;
			goto_state_count++;
		}
		goto_spans[state].count = checked_u32(goto_state_count, "TableBuilder: goto span count overflow");
	}

	if (goto_index != m_goto_count_)
		throw std::logic_error("TableBuilder: goto count mismatch");

	/*
		RULE TABLE
		rule_entries[rule_id] is exactly corresponding to CFG rule_id
	*/

	for (size_t rule_id = 0; rule_id < m_cfg_.rule_count(); rule_id++)
	{
		const t_rule& rule = m_cfg_.rule(rule_id);

		rule_entries[rule_id].lhs = checked_u32(rule.lhs, "TableBuilder: rule lhs overflow");
		rule_entries[rule_id].rhs_count = checked_u32(rule.rhs_slice.count, "TableBuilder: rule rhs count overflow");

		if (m_semantics_)
			rule_entries[rule_id].semantic_id = m_semantics_->semantic_id(rule.lhs);
		else
			rule_entries[rule_id].semantic_id = PARSING_NO_SEMANTIC;
	}
}

// private

void	TableBuilder::compute_layout( void )
{
	m_action_count_ = 0;
	m_goto_count_ = 0;

	// count ACTION EXCEPTIONS
	for (size_t state = 0; state < m_ir_.state_count(); state++)
	{
		t_action	default_action = find_default_action(m_ir_, state);

		for (t_id_symbol t = 0; t <= CFG_END; t++)
		{
			const t_action& action = m_ir_.action(state, t);

			if (!same_action(action, default_action))
				m_action_count_++;
		}
	}

	// count valid gotos
	for (size_t state = 0; state < m_ir_.state_count(); state++)
	{
		for (t_id_symbol nt = 0; nt < m_ir_.non_terminal_count(); nt++)
		{
			t_id_symbol symbol = CFG_FIRST_NON_TERMINAL + nt;

			if (m_ir_.goto_state(state, symbol) != PARSING_INVALID_GOTO)
				m_goto_count_++;
		}
	}

	// compute final
	size_t	offset = 0;

	offset += sizeof(t_table_blob_hdr);
	offset = align4(offset); 

	m_action_spans_offset_ = offset;
	offset = checked_u64_add(offset, checked_u64_mul(m_ir_.state_count(), sizeof(t_action_span), "TableBuilder: 1 : u64 mul overflow"), "TableBuilder: action spans layout overflow");
	offset = align4(offset);

	m_action_entries_offset_ = offset;
	offset = checked_u64_add(offset, checked_u64_mul(m_action_count_, sizeof(t_action_entry), "TableBuilder: 2 : u64 mul overflow"), "TableBuilder: action entries layout overflow");
	offset = align4(offset);

	m_goto_spans_offset_ = offset;
	offset = checked_u64_add(offset, checked_u64_mul(m_ir_.state_count(), sizeof(t_blob_span), "TableBuilder: 3 : u64 mul overflow"), "TableBuilder: goto spans layout overflow");
	offset = align4(offset);

	m_goto_entries_offset_ = offset;
	offset = checked_u64_add(offset, checked_u64_mul(m_goto_count_, sizeof(t_goto_entry), "TableBuilder: 4 : u64 mul overflow"), "TableBuilder: goto entires layout overflow");
	offset = align4(offset);

	m_rule_entries_offset_ = offset;
	offset = checked_u64_add(offset, checked_u64_mul(m_cfg_.rule_count(), sizeof(t_rule_entry), "TableBuilder: rule entries size overflow"), "TableBuilder: rule entries layout overflow");

	m_required_size_ = align4(offset);
}

void	TableBuilder::validate_layout( void ) const
{
	checked_u32(m_required_size_, "TableBuilder: blob exceeds uint32 format");
	checked_u32(m_ir_.state_count(), "TableBuilder: state count exceeds uint32");
	checked_u32(m_ir_.non_terminal_count(), "TableBuilder: non terminal count exceeds uint32");
	checked_u32(m_action_count_, "TableBuilder: action count exceeds uint32");
	checked_u32(m_goto_count_, "TableBuilder: goto count exceeds uint32");
	checked_u32(m_action_spans_offset_, "TableBuilder: action spans offset exceeds uint32");
	checked_u32(m_action_entries_offset_, "TableBuilder: action entries offset exceeds uint32");
	checked_u32(m_goto_spans_offset_, "TableBuilder: goto spans offset exceeds uint32");
	checked_u32(m_goto_entries_offset_, "TableBuilder: goto entries offset exceeds uint32");
	checked_u32(m_cfg_.rule_count(), "TableBuilder: rule count exceeds uint32");
	checked_u32(m_rule_entries_offset_, "TableBuilder: rule entries offset exceeds uint32");
	
	/* check packed 30-bit action representation. */

	for ( size_t state = 0; state < m_ir_.state_count(); state++)
	{
		for (t_id_symbol terminal = 0; terminal <= CFG_END; terminal++)
		{
			const t_action& action = m_ir_.action(state, terminal);
			pack_action(action.type, action.value);
		}
	}

	/* check useless but at least we are sur its determinist */
	// mieux vaut prevenir que guerir

	for (size_t rule_id = 0; rule_id < m_cfg_.rule_count(); rule_id++)
	{
		const t_rule& rule = m_cfg_.rule(rule_id);

		if (!m_cfg_.is_non_terminal(rule.lhs))
			throw std::logic_error("TableBuilder: rule lhs is not a non terminal");

		checked_u32(rule.lhs, "TableBuilder: rule lhs overflow");
		checked_u32(rule.rhs_slice.count, "TableBuilder: rule rhs count overflow");
	}
}

