// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#include "TableView.hpp"

// metadata
const t_table_blob_hdr&	TableView::header(void) const
{	return (*m_hdr_);	}

size_t	TableView::blob_size(void) const
{	return ((size_t)m_hdr_->total_size);	}

size_t	TableView::state_count(void) const
{	return ((size_t)m_hdr_->state_count);	}

size_t	TableView::non_terminal_count(void) const
{	return ((size_t)m_hdr_->non_terminal_count);	}

size_t	TableView::rule_count(void) const
{	return ((size_t)m_hdr_->rule_count);	}

size_t	TableView::semantic_count(void) const
{	return ((size_t)m_hdr_->semantic_count);	}

size_t	TableView::action_count(void) const
{	return ((size_t)m_hdr_->action_count);	}

size_t	TableView::goto_count(void) const
{	return ((size_t)m_hdr_->goto_count);	}


// section pointers
const t_action_span	*TableView::action_spans_ptr(void) const
{	return ((const t_action_span*)(m_base_ + m_hdr_->action_spans_offset));	}

const t_action_entry	*TableView::action_entries_ptr(void) const
{	return ((const t_action_entry*)(m_base_ + m_hdr_->action_entries_offset));	}

const t_blob_span	*TableView::goto_spans_ptr(void) const
{	return ((const t_blob_span*)(m_base_ + m_hdr_->goto_spans_offset));	}

const t_goto_entry	*TableView::goto_entries_ptr(void) const
{	return ((const t_goto_entry*)(m_base_ + m_hdr_->goto_entries_offset));	}

const t_rule_entry	*TableView::rule_entries_ptr(void) const
{	return ((const t_rule_entry*)(m_base_ + m_hdr_->rule_entries_offset));	}


// sparse access
const t_action_span&	TableView::action_span(t_id_state state) const
{
	// validate_state(state);
	return (action_spans_ptr()[state]);
}

const t_action_entry&	TableView::action_entry(size_t index) const
{	return (action_entries_ptr()[index]);	}

const t_blob_span&	TableView::goto_span(t_id_state state) const
{
	// validate_state(state);
	return (goto_spans_ptr()[state]);
}

const t_goto_entry&	TableView::goto_entry(size_t index) const
{	return (goto_entries_ptr()[index]);	}
