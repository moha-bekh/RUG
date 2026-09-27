// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstddef>

// #include "../BitSet/BitSet.hpp"

typedef size_t t_id_symbol;
typedef size_t t_id_rule;

static const t_id_symbol	CFG_INVALID_SYMBOL			= (size_t)-1;
static const t_id_rule		CFG_INVALID_RULE			= (size_t)-1;
static const t_id_symbol	CFG_END						= 256; // logical end / this is not real socket/file EOF 
static const t_id_symbol	CFG_FIRST_NON_TERMINAL		= 257;
