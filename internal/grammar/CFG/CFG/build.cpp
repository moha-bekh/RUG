// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#include "CFG.hpp"

/*
	AKA count sort
*/
void		CFG::group_productions( void )
{
	std::vector<size_t>	v_counts;
	std::vector<size_t>	v_cursors;
	size_t				id_non_terminal;
	size_t				offset = 0;

	v_counts.resize(m_non_terminal_count_, 0);

	for (size_t i = 0; i < mv_rules_.size(); i++)
	{
		id_non_terminal = mv_rules_[i].lhs - CFG_FIRST_NON_TERMINAL;
		v_counts[id_non_terminal]++;
	}

	mv_same_lhs_slices_.resize(m_non_terminal_count_);

	for (size_t i = 0; i < m_non_terminal_count_; i++)
	{
		mv_same_lhs_slices_[i].begin = offset;
		mv_same_lhs_slices_[i].count = v_counts[i];
		offset += v_counts[i];
	}

	v_cursors.resize(m_non_terminal_count_);

	for (size_t i = 0; i < m_non_terminal_count_; i++)
		v_cursors[i] = mv_same_lhs_slices_[i].begin;

	mv_sorted_rule_ids_.resize(mv_rules_.size());

	for (size_t i = 0; i < mv_rules_.size(); i++)
	{
		id_non_terminal = mv_rules_[i].lhs - CFG_FIRST_NON_TERMINAL;
		mv_sorted_rule_ids_[v_cursors[id_non_terminal]] = i;
		v_cursors[id_non_terminal]++;
	}
}

void		CFG::set_start_symbol( t_id_symbol id )
{	m_start_symbol_ = id;	}

t_id_symbol CFG::add_non_terminal( void )
{
	t_id_symbol		id;
	t_source_span	empty_span = {0, 0, 0, 0};

	id = CFG_FIRST_NON_TERMINAL + m_non_terminal_count_;
	m_non_terminal_count_++;

	mv_source_spans_.push_back(empty_span);
	mv_has_source_span_.push_back(false);

	return (id);
}

t_id_rule 	CFG::add_rule( t_id_symbol lhs, const std::vector<t_id_symbol>& v_rhs )
{
	t_rule	rule;

	rule.lhs = lhs;
	rule.rhs_slice.begin = mv_rhs_.size();
	rule.rhs_slice.count = v_rhs.size();

	for (size_t i = 0; i < v_rhs.size(); i++)
		mv_rhs_.push_back(v_rhs[i]);

	mv_rules_.push_back(rule);

	return (mv_rules_.size() - 1);
}

t_id_rule 	CFG::add_rule( t_id_symbol lhs, t_id_symbol unary_rhs_id )
{
	t_rule	rule;

	rule.lhs = lhs;
	rule.rhs_slice.begin = mv_rhs_.size();
	rule.rhs_slice.count = 1;

	mv_rhs_.push_back(unary_rhs_id);

	mv_rules_.push_back(rule);

	return (mv_rules_.size() - 1);
}

t_id_rule	CFG::add_empty_rule( t_id_symbol lhs )
{
	t_rule rule;

	rule.lhs = lhs;
	rule.rhs_slice.begin = mv_rhs_.size();
	rule.rhs_slice.count = 0;

	mv_rules_.push_back(rule);

	return (mv_rules_.size() - 1);
}