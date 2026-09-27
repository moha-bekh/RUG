// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstddef>
#include <cfloat>
#include <cmath>

#include "../../inc/Rug.hpp"
#include "../../inc/RugSession.hpp"

enum e_calc_value_type
{
	CALC_VALUE_NONE,
	CALC_VALUE_NUMBER,
	CALC_VALUE_OPERATOR
};

enum e_calc_error
{
	CALC_OK = 0,
	CALC_INTERNAL_ERROR = 1,
	CALC_INVALID_NUMBER = 2,
	CALC_NUMBER_OVERFLOW = 3,
	CALC_DIVISION_BY_ZERO = 4,
	CALC_MODULO_BY_ZERO = 5
};

typedef struct	s_calc_value
{
	e_calc_value_type	type;
	double				number;
	char				op;
}					t_calc_value;

typedef struct	s_calc_context
{
	const char			*input;
	size_t				input_size;

	t_calc_value		*stack;
	size_t				stack_capacity;

	double				result;
	bool				has_result;
}					t_calc_context;

void				calc_init(t_calc_context& ctx, const char *input,
						size_t input_size, t_calc_value *stack,
						size_t stack_capacity);

const char			*calc_error_string(t_semantic_result error);

t_semantic_result	calc_start(const t_reduce_event& event, void *user);
t_semantic_result	calc_expression(const t_reduce_event& event, void *user);
t_semantic_result	calc_add_op(const t_reduce_event& event, void *user);
t_semantic_result	calc_term(const t_reduce_event& event, void *user);
t_semantic_result	calc_mul_op(const t_reduce_event& event, void *user);
t_semantic_result	calc_unary(const t_reduce_event& event, void *user);
t_semantic_result	calc_primary(const t_reduce_event& event, void *user);
t_semantic_result 	calc_numeric(const t_reduce_event& event, void *user);
t_semantic_result	calc_number(const t_reduce_event& event, void *user);