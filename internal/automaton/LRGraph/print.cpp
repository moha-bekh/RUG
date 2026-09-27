// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#include "LRGraph.hpp"

void					LRGraph::print_meta_data( void )
{
	std::cout << "==========================" << std::endl;
	std::cout << "states     = " << mv_states_.size() << std::endl;
	std::cout << "transition = " << mv_transitions_.size() << std::endl;
	std::cout << "==========================" << std::endl;
}