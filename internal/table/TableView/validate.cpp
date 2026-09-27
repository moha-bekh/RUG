// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#include "TableView.hpp"

static size_t	section_end(size_t total_size, uint32_t offset, size_t count, size_t element_size, const char *what)
{
	size_t begin = (size_t)offset;

	if (begin > total_size)
		throw std::runtime_error(what);

	size_t available = total_size - begin;

	if (element_size != 0 && count > available / element_size)
		throw std::runtime_error(what);

	return (begin + count * element_size);
}

static void	validate_alignment(uint32_t offset, const char *what)
{
	if ((offset & 3u) != 0)
		throw std::runtime_error(what);
}

static void	validate_packed_action(t_packed_action packed,
								size_t state_count, size_t rule_count)
{
	e_action_type type = unpack_action_type(packed);
	uint32_t value = unpack_action_value(packed);

	if (type == ACTION_SHIFT)
	{
		if ((size_t)value >= state_count)
			throw std::runtime_error("TableView: invalid shift target");
		return ;
	}
	if (type == ACTION_REDUCE)
	{
		if ((size_t)value >= rule_count)
			throw std::runtime_error("TableView: invalid reduce rule");
		return ;
	}
	if (type == ACTION_ACCEPT)
	{
		if (value != 0)
			throw std::runtime_error("TableView: invalid accept value");
		return ;
	}
	if (type == ACTION_ERROR)
	{
		if (value != 0)
			throw std::runtime_error("TableView: invalid error value");
		return ;
	}
	throw std::runtime_error("TableView: invalid action type");
}


void	TableView::validate_state(t_id_state state) const
{
	if ((size_t)state >= state_count())
		throw std::out_of_range("TableView: invalid state");
}

void	TableView::validate(void) const
{
	size_t total_size = (size_t)m_hdr_->total_size;

	if (total_size < sizeof(t_table_blob_hdr))
		throw std::runtime_error("TableView: invalid total size");
	if (total_size > m_capacity_)
		throw std::runtime_error("TableView: declared blob size exceeds mapped capacity");

	// section allignement
	validate_alignment(m_hdr_->action_spans_offset, "TableView: unaligned action spans");
	validate_alignment(m_hdr_->action_entries_offset, "TableView: unaligned action entries");
	validate_alignment(m_hdr_->goto_spans_offset, "TableView: unaligned goto spans");
	validate_alignment(m_hdr_->goto_entries_offset, "TableView: unaligned goto entries");
	validate_alignment(m_hdr_->rule_entries_offset, "TableView: unaligned rule entries");

	// validate section bounds
	size_t action_spans_end = section_end(total_size, m_hdr_->action_spans_offset, state_count(), sizeof(t_action_span), "TableView: action spans outside blob");
	size_t action_entries_end = section_end(total_size, m_hdr_->action_entries_offset, action_count(), sizeof(t_action_entry), "TableView: action entries outside blob");
	size_t goto_spans_end = section_end(total_size, m_hdr_->goto_spans_offset, state_count(), sizeof(t_blob_span), "TableView: goto spans outside blob");
	size_t goto_entries_end = section_end(total_size, m_hdr_->goto_entries_offset, goto_count(), sizeof(t_goto_entry), "TableView: goto entries outside blob");
	size_t rule_entries_end = section_end(total_size, m_hdr_->rule_entries_offset, rule_count(), sizeof(t_rule_entry), "TableView: rule entries outside blob");

	// overlap
	if ((size_t)m_hdr_->action_spans_offset < sizeof(t_table_blob_hdr))
		throw std::runtime_error("TableView: action spans overlap header");
	if (action_spans_end > (size_t)m_hdr_->action_entries_offset)
		throw std::runtime_error("TableView: action sections overlap");
	if (action_entries_end > (size_t)m_hdr_->goto_spans_offset)
		throw std::runtime_error("TableView: action entries overlap goto spans");
	if (goto_spans_end > (size_t)m_hdr_->goto_entries_offset)
		throw std::runtime_error("TableView: goto sections overlap");
	if (goto_entries_end > (size_t)m_hdr_->rule_entries_offset)
		throw std::runtime_error("TableView: goto entries overlap rule entries");
	if (rule_entries_end > total_size)
		throw std::runtime_error("TableView: rule entries exceed blob");

	// ACTION
	for (size_t state = 0; state < state_count(); state++)
	{
		const t_action_span& span = action_spans_ptr()[state];

		if ((size_t)span.offset > action_count())
			throw std::runtime_error("TableView: invalid action span offset");
		if ((size_t)span.count > action_count() - (size_t)span.offset)
			throw std::runtime_error("TableView: invalid action span count");

		validate_packed_action(span.default_action, state_count(), rule_count());

		for (size_t i = 0; i < (size_t)span.count; i++)
		{
			size_t index = (size_t)span.offset + i;
			const t_action_entry& entry = action_entries_ptr()[index];

			if ((t_id_symbol)entry.terminal > CFG_END)
				throw std::runtime_error("TableView: invalid action terminal");

			validate_packed_action(entry.action, state_count(), rule_count());

			if (i == 0)
				continue ;

			const t_action_entry& previous = action_entries_ptr()[index - 1];
			if (previous.terminal >= entry.terminal)
				throw std::runtime_error("TableView: unsorted action entries");
		}
	}


	// GOTO
	for (size_t state = 0; state < state_count(); state++)
	{
		const t_blob_span& span = goto_spans_ptr()[state];

		if ((size_t)span.offset > goto_count())
			throw std::runtime_error("TableView: invalid goto span offset");

		if ((size_t)span.count > goto_count() - (size_t)span.offset)
			throw std::runtime_error("TableView: invalid goto span count");

		for (size_t i = 0; i < (size_t)span.count; i++)
		{
			size_t index = (size_t)span.offset + i;
			const t_goto_entry& entry = goto_entries_ptr()[index];

			if ((size_t)entry.non_terminal >= non_terminal_count())
				throw std::runtime_error("TableView: invalid goto non-terminal");

			if ((size_t)entry.target >= state_count())
				throw std::runtime_error("TableView: invalid goto target");

			if (i == 0)
				continue ;

			const t_goto_entry& previous = goto_entries_ptr()[index - 1];

			if (previous.non_terminal >= entry.non_terminal)
				throw std::runtime_error("TableView: unsorted goto entries");
		}
	}

	// Rules
	for (size_t rule_id = 0; rule_id < rule_count(); rule_id++)
	{
		const t_rule_entry& rule = rule_entries_ptr()[rule_id];

		if ((t_id_symbol)rule.lhs < CFG_FIRST_NON_TERMINAL)
			throw std::runtime_error("TableView: invalid rule lhs");

		size_t nt_index =
			(size_t)((t_id_symbol)rule.lhs - CFG_FIRST_NON_TERMINAL);

		if (nt_index >= non_terminal_count())
			throw std::runtime_error("TableView: rule lhs outside non-terminal range");

		if (rule.semantic_id != PARSING_NO_SEMANTIC
			&& (size_t)rule.semantic_id >= semantic_count())
		{
			throw std::runtime_error("TableView: invalid semantic id");
		}
	}
}