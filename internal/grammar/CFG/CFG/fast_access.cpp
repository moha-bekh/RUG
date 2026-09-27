// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#include "CFG.hpp"

size_t		CFG::rule_count_for_same_lhs( t_id_symbol lhs ) const
{
	size_t			id_non_terminal= lhs - CFG_FIRST_NON_TERMINAL;
	const t_slice&	slice = mv_same_lhs_slices_[id_non_terminal];

	return (slice.count);
}

t_id_rule	CFG::rule_id_for_lhs_at( t_id_symbol lhs, size_t index ) const
{
	size_t			id_non_terminal= lhs - CFG_FIRST_NON_TERMINAL;
	const t_slice&	slice = mv_same_lhs_slices_[id_non_terminal];

	return (mv_sorted_rule_ids_[slice.begin + index]);
}
