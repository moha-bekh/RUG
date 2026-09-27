// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#include "CFGBuilder.hpp"

/*
	2*5 A	-> exact(2) + bounded(3)
	3* A	-> exact(3) + unbounded
	*5 A	-> bounded(5)
	5 A		-> exact(5)
*/
void CFGBuilder::lower_repeat( const t_abnf_node& node, std::vector<t_id_symbol>& v_rhs )
{
	t_id_symbol						unit;
	std::map<size_t, t_id_symbol> m_exact_cache;
	std::map<size_t, t_id_symbol> m_bounded_cache;

	// considering: 2*5"a"
	// unit == id("a")
	unit = lower_to_symbol(node.repeat.child);

	if (node.repeat.min > 0)
		v_rhs.push_back(make_exact_repeat(unit, node.repeat.min, m_exact_cache));

	if (!node.repeat.unbounded && node.repeat.min == node.repeat.max)
		return ;

	if (node.repeat.unbounded)
	{
		v_rhs.push_back(make_unbounded_repeat(unit));
		return ;
	}

	if (node.repeat.max > node.repeat.min)
	{
		v_rhs.push_back(make_bounded_repeat(unit, node.repeat.max - node.repeat.min, m_exact_cache, m_bounded_cache));
		return ;
	}
}

t_id_symbol CFGBuilder::make_unbounded_repeat( t_id_symbol unit )
{
	t_id_symbol					id;
	std::vector<t_id_symbol>	v_rhs;

	id = new_synthetic_symbol("star");

	// produce EPSILON
	// $star -> ε
	m_cfg_.add_empty_rule(id);

	// produde main branch
	// $star -> $star unit
	v_rhs.push_back(id);
	v_rhs.push_back(unit);
	m_cfg_.add_rule(id, v_rhs);

	return (id);
}

/*
	2"a"
	considering a count of two
	we create a node conataining two terminal 
	left count = 1;
	right count = 1;
	$exact(2) -> "a" "a"

	but then if we want to make a 4"a"
	left count = 2;
	right count = 2;

	we will recurse into make_exact_repeat()
	and we will have a cahce hit on the exact_cache
	$exact(4) -> $exact(2) $exact(2)
*/
t_id_symbol CFGBuilder::make_exact_repeat( t_id_symbol unit, size_t count, std::map<size_t, t_id_symbol>& m_cache )
{
	std::map<size_t, t_id_symbol>::iterator	it;
	size_t									left_count;
	size_t									right_count;
	t_id_symbol								left;
	t_id_symbol								right;
	t_id_symbol								id;
	std::vector<t_id_symbol>				v_rhs;

	// end recursion
	if (count == 1)
		return (unit);

	it = m_cache.find(count);

	// if cache hit ret symbol id
	if (it != m_cache.end())
		return (it->second);

	// recursively build symbols E4 = E2 E2
	left_count = count / 2;
	right_count = count - left_count;
	left = make_exact_repeat(unit, left_count, m_cache);
	right = make_exact_repeat(unit, right_count, m_cache);

	// make synthetique rule E
	id = new_synthetic_symbol("exact");
	
	m_cache[count] = id; // make cache entry

	// insert synthtique rule
	v_rhs.push_back(left);
	v_rhs.push_back(right);
	m_cfg_.add_rule(id, v_rhs);

	return (id);
}


/*
	if yopu wonder why a map is used
	and why we create synthetic nodes named "$upto" 
	think about the following case: 1*1000000 A
*/
t_id_symbol CFGBuilder::make_bounded_repeat( t_id_symbol unit, size_t maximum, std::map<size_t, t_id_symbol>& m_exact_cache, std::map<size_t, t_id_symbol>& m_bounded_cache )
{
	std::map<size_t, t_id_symbol>::iterator	it;
	t_id_symbol								id;
	t_id_symbol 							lower_part;
	t_id_symbol 							exact_part;
	t_id_symbol 							remaining_part;
	size_t 									power;
	size_t 									remaining;
	std::vector<t_id_symbol> 				v_rhs;


	it = m_bounded_cache.find(maximum);

	// if cache hit ret symbol id
	if (it != m_bounded_cache.end())
		return (it->second);

	// make synthetique rule E
	id = new_synthetic_symbol("upto");
	m_bounded_cache[maximum] = id; // make cache entry 

	if (maximum == 1)
	{
		// act as an option []

		// make epsilon
		m_cfg_.add_empty_rule(id);
		// make value
		m_cfg_.add_rule(id, unit);

		return (id);
	}

	power = highest_power_of_two(maximum);

	lower_part = make_bounded_repeat(unit, power - 1, m_exact_cache, m_bounded_cache);
	exact_part = make_exact_repeat(unit, power, m_exact_cache);
	
	remaining = maximum - power;

	// avoid vec use / use overload with symbol_id
	v_rhs.push_back(lower_part);
	m_cfg_.add_rule(id, v_rhs);

	v_rhs.clear();

	v_rhs.push_back(exact_part);

	if (remaining > 0)
	{
		remaining_part = make_bounded_repeat(unit, remaining, m_exact_cache, m_bounded_cache);
		v_rhs.push_back(remaining_part);
	}

	m_cfg_.add_rule(id, v_rhs);

	return (id);
}

size_t CFGBuilder::highest_power_of_two( size_t value ) const
{
	size_t power;

	if (value == 0)
		return (0);

	power = 1;

	while (power <= value / 2)
		power *= 2;

	return (power);
}