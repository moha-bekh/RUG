// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../../grammar/CFG/CFG/CFG.hpp"
#include "../../common/BitSet/BitSet.hpp"

typedef size_t		t_id_state;

static const t_id_state	LR_INVALID_STATE = (size_t)-1;
static const t_id_rule	LR_AUGMENTED_RULE = (size_t)-2;

typedef struct		s_lr_item_core
{
	t_id_rule		rule;
	size_t			dot;
}					t_lr_item_core;

typedef struct		s_lr_item
{
	t_lr_item_core	core;
	LookaheadSet	lookaheads;
}					t_lr_item;

typedef struct		s_lr_transition
{
	t_id_symbol		symbol;
	t_id_state		from;
	t_id_state		to;
}					t_lr_transition;