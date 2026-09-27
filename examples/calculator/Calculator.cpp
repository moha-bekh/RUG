// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#include "Calculator.hpp"

static t_calc_value	calc_make_none(void)
{
	t_calc_value value;

	value.type = CALC_VALUE_NONE;
	value.number = 0.0;
	value.op = 0;
	return (value);
}

static t_calc_value	calc_make_number(double number)
{
	t_calc_value value;

	value.type = CALC_VALUE_NUMBER;
	value.number = number;
	value.op = 0;
	return (value);
}

static t_calc_value	calc_make_operator(char op)
{
	t_calc_value value;

	value.type = CALC_VALUE_OPERATOR;
	value.number = 0.0;
	value.op = op;
	return (value);
}

void	calc_init(t_calc_context& ctx, const char *input, size_t input_size,
		t_calc_value *stack, size_t stack_capacity)
{
	size_t i;

	ctx.input = input;
	ctx.input_size = input_size;
	ctx.stack = stack;
	ctx.stack_capacity = stack_capacity;
	ctx.result = 0.0;
	ctx.has_result = false;
	i = 0;
	while (i < stack_capacity)
	{
		ctx.stack[i] = calc_make_none();
		++i;
	}
}

const char	*calc_error_string(t_semantic_result error)
{
	if (error == CALC_OK)
		return ("ok");
	if (error == CALC_INTERNAL_ERROR)
		return ("internal calculator error");
	if (error == CALC_INVALID_NUMBER)
		return ("invalid number");
	if (error == CALC_NUMBER_OVERFLOW)
		return ("number overflow");
	if (error == CALC_DIVISION_BY_ZERO)
		return ("division by zero");
	if (error == CALC_MODULO_BY_ZERO)
		return ("modulo by zero");
	return ("unknown calculator error");
}

static bool	calc_slot_valid(const t_calc_context& ctx, size_t index)
{
	return (index < ctx.stack_capacity);
}

static t_calc_value	*calc_lhs(t_calc_context& ctx,
					const t_reduce_event& event)
{
	if (!calc_slot_valid(ctx, event.rhs_begin))
		return (NULL);
	return (&ctx.stack[event.rhs_begin]);
}

static const t_calc_value	*calc_rhs(const t_calc_context& ctx,
							const t_reduce_event& event, size_t index)
{
	size_t absolute;

	if (index >= event.rhs_count)
		return (NULL);
	absolute = event.rhs_begin + index;
	if (!calc_slot_valid(ctx, absolute))
		return (NULL);
	return (&ctx.stack[absolute]);
}

static bool	calc_number_value(const t_calc_value *value)
{
	return (value != NULL && value->type == CALC_VALUE_NUMBER);
}

static bool	calc_operator_value(const t_calc_value *value)
{
	return (value != NULL && value->type == CALC_VALUE_OPERATOR);
}

static bool	calc_bad_double(double value)
{
	if (value != value)
		return (true);
	if (value > DBL_MAX)
		return (true);
	if (value < -DBL_MAX)
		return (true);
	return (false);
}

static bool	calc_span_valid(const t_calc_context& ctx,
					const t_parse_span& span)
{
	if (span.begin > span.end)
		return (false);
	if (span.end > ctx.input_size)
		return (false);
	return (true);
}

static bool	calc_is_digit(char c)
{
	return (c >= '0' && c <= '9');
}

static t_semantic_result	calc_parse_number(const t_calc_context& ctx,
							const t_parse_span& span, double& output)
{
	size_t		i;
	long double	value;
	long double	factor;
	int			exponent_sign;
	unsigned long	exponent;
	bool		exponent_too_large;
	double		scale;

	if (!calc_span_valid(ctx, span))
		return (CALC_INVALID_NUMBER);
	if (span.begin == span.end)
		return (CALC_INVALID_NUMBER);

	i = span.begin;
	value = 0.0L;

	if (!calc_is_digit(ctx.input[i]))
		return (CALC_INVALID_NUMBER);

	while (i < span.end && calc_is_digit(ctx.input[i]))
	{
		int digit = ctx.input[i] - '0';

		if (value > (LDBL_MAX - digit) / 10.0L)
			return (CALC_NUMBER_OVERFLOW);
		value = value * 10.0L + digit;
		++i;
	}

	if (i < span.end && ctx.input[i] == '.')
	{
		++i;
		if (i >= span.end || !calc_is_digit(ctx.input[i]))
			return (CALC_INVALID_NUMBER);
		factor = 0.1L;
		while (i < span.end && calc_is_digit(ctx.input[i]))
		{
			value += (ctx.input[i] - '0') * factor;
			factor *= 0.1L;
			++i;
		}
	}

	if (i < span.end && (ctx.input[i] == 'e' || ctx.input[i] == 'E'))
	{
		++i;
		exponent_sign = 1;
		if (i < span.end && (ctx.input[i] == '+' || ctx.input[i] == '-'))
		{
			if (ctx.input[i] == '-')
				exponent_sign = -1;
			++i;
		}
		if (i >= span.end || !calc_is_digit(ctx.input[i]))
			return (CALC_INVALID_NUMBER);

		exponent = 0;
		exponent_too_large = false;
		while (i < span.end && calc_is_digit(ctx.input[i]))
		{
			if (exponent > 10000)
				exponent_too_large = true;
			else
				exponent = exponent * 10 + (ctx.input[i] - '0');
			++i;
		}

		if (value != 0.0L && exponent_too_large && exponent_sign > 0)
			return (CALC_NUMBER_OVERFLOW);
		if (exponent_too_large && exponent_sign < 0)
			value = 0.0L;
		else
		{
			scale = std::pow(10.0, static_cast<int>(exponent) * exponent_sign);
			value *= scale;
		}
	}

	if (i != span.end)
		return (CALC_INVALID_NUMBER);
	if (value > DBL_MAX)
		return (CALC_NUMBER_OVERFLOW);

	output = static_cast<double>(value);
	if (calc_bad_double(output))
		return (CALC_NUMBER_OVERFLOW);
	return (CALC_OK);
}

t_semantic_result calc_numeric(const t_reduce_event& event, void *user)
{
	t_calc_context *ctx;
	t_calc_value *lhs;
	double number;
	t_semantic_result result;

	ctx = static_cast<t_calc_context *>(user);
	if (ctx == NULL)
		return (CALC_INTERNAL_ERROR);
	lhs = calc_lhs(*ctx, event);
	if (lhs == NULL)
		return (CALC_INTERNAL_ERROR);
	result = calc_parse_number(*ctx, event.span, number);
	if (result != CALC_OK)
		return (result);
	*lhs = calc_make_number(number);
	return (CALC_OK);
}

t_semantic_result calc_number(const t_reduce_event& event, void *user)
{
	t_calc_context *ctx;
	t_calc_value *lhs;
	const t_calc_value *value;

	ctx = static_cast<t_calc_context *>(user);
	if (ctx == NULL || event.rhs_count != 2)
		return (CALC_INTERNAL_ERROR);
	lhs = calc_lhs(*ctx, event);
	value = calc_rhs(*ctx, event, 0);
	if (lhs == NULL || !calc_number_value(value))
		return (CALC_INTERNAL_ERROR);
	*lhs = *value;
	return (CALC_OK);
}

t_semantic_result calc_add_op(const t_reduce_event& event, void *user)
{
	t_calc_context *ctx;
	t_calc_value *lhs;
	char op;

	ctx = static_cast<t_calc_context *>(user);
	if (ctx == NULL || !calc_span_valid(*ctx, event.span) || event.span.begin == event.span.end)
		return (CALC_INTERNAL_ERROR);

	op = ctx->input[event.span.begin];
	if (op != '+' && op != '-')
		return (CALC_INTERNAL_ERROR);

	lhs = calc_lhs(*ctx, event);
	if (lhs == NULL)
		return (CALC_INTERNAL_ERROR);

	*lhs = calc_make_operator(op);
	return (CALC_OK);
}

t_semantic_result calc_mul_op(const t_reduce_event& event, void *user)
{
	t_calc_context *ctx;
	t_calc_value *lhs;
	char op;

	ctx = static_cast<t_calc_context *>(user);
	if (ctx == NULL || !calc_span_valid(*ctx, event.span) || event.span.begin == event.span.end)
		return (CALC_INTERNAL_ERROR);

	op = ctx->input[event.span.begin];
	if (op != '*' && op != '/' && op != '%')
		return (CALC_INTERNAL_ERROR);

	lhs = calc_lhs(*ctx, event);
	if (lhs == NULL)
		return (CALC_INTERNAL_ERROR);

	*lhs = calc_make_operator(op);
	return (CALC_OK);
}

t_semantic_result calc_expression(const t_reduce_event& event, void *user)
{
	t_calc_context *ctx;
	t_calc_value *lhs;
	const t_calc_value *left;
	const t_calc_value *op;
	const t_calc_value *right;
	double result;

	ctx = static_cast<t_calc_context *>(user);
	if (ctx == NULL)
		return (CALC_INTERNAL_ERROR);
	lhs = calc_lhs(*ctx, event);
	if (lhs == NULL)
		return (CALC_INTERNAL_ERROR);

	if (event.rhs_count == 1)
	{
		left = calc_rhs(*ctx, event, 0);
		if (!calc_number_value(left))
			return (CALC_INTERNAL_ERROR);
		*lhs = *left;
		return (CALC_OK);
	}

	if (event.rhs_count != 3)
		return (CALC_INTERNAL_ERROR);

	left = calc_rhs(*ctx, event, 0);
	op = calc_rhs(*ctx, event, 1);
	right = calc_rhs(*ctx, event, 2);

	if (!calc_number_value(left) || !calc_operator_value(op) || !calc_number_value(right))
		return (CALC_INTERNAL_ERROR);

	if (op->op == '+')
		result = left->number + right->number;
	else if (op->op == '-')
		result = left->number - right->number;
	else
		return (CALC_INTERNAL_ERROR);

	if (calc_bad_double(result))
		return (CALC_NUMBER_OVERFLOW);

	*lhs = calc_make_number(result);
	return (CALC_OK);
}
t_semantic_result calc_term(const t_reduce_event& event, void *user)
{
	t_calc_context *ctx;
	t_calc_value *lhs;
	const t_calc_value *left;
	const t_calc_value *op;
	const t_calc_value *right;
	double result;

	ctx = static_cast<t_calc_context *>(user);
	if (ctx == NULL)
		return (CALC_INTERNAL_ERROR);
	lhs = calc_lhs(*ctx, event);
	if (lhs == NULL)
		return (CALC_INTERNAL_ERROR);

	if (event.rhs_count == 1)
	{
		left = calc_rhs(*ctx, event, 0);
		if (!calc_number_value(left))
			return (CALC_INTERNAL_ERROR);
		*lhs = *left;
		return (CALC_OK);
	}

	if (event.rhs_count != 3)
		return (CALC_INTERNAL_ERROR);

	left = calc_rhs(*ctx, event, 0);
	op = calc_rhs(*ctx, event, 1);
	right = calc_rhs(*ctx, event, 2);

	if (!calc_number_value(left) || !calc_operator_value(op) || !calc_number_value(right))
		return (CALC_INTERNAL_ERROR);

	if (op->op == '*')
		result = left->number * right->number;
	else if (op->op == '/')
	{
		if (right->number == 0.0)
			return (CALC_DIVISION_BY_ZERO);
		result = left->number / right->number;
	}
	else if (op->op == '%')
	{
		if (right->number == 0.0)
			return (CALC_MODULO_BY_ZERO);
		result = std::fmod(left->number, right->number);
	}
	else
		return (CALC_INTERNAL_ERROR);

	if (calc_bad_double(result))
		return (CALC_NUMBER_OVERFLOW);

	*lhs = calc_make_number(result);
	return (CALC_OK);
}

t_semantic_result	calc_unary(const t_reduce_event& event, void *user)
{
	t_calc_context		*ctx;
	t_calc_value			*lhs;
	const t_calc_value	*value;
	char				op;
	double				result;

	ctx = static_cast<t_calc_context *>(user);
	if (ctx == NULL)
		return (CALC_INTERNAL_ERROR);
	lhs = calc_lhs(*ctx, event);
	if (lhs == NULL)
		return (CALC_INTERNAL_ERROR);

	if (event.rhs_count == 1)
	{
		value = calc_rhs(*ctx, event, 0);
		if (!calc_number_value(value))
			return (CALC_INTERNAL_ERROR);
		*lhs = *value;
		return (CALC_OK);
	}

	if (event.rhs_count != 3)
		return (CALC_INTERNAL_ERROR);
	if (!calc_span_valid(*ctx, event.span))
		return (CALC_INTERNAL_ERROR);
	if (event.span.begin == event.span.end)
		return (CALC_INTERNAL_ERROR);

	value = calc_rhs(*ctx, event, 2);
	if (!calc_number_value(value))
		return (CALC_INTERNAL_ERROR);

	op = ctx->input[event.span.begin];
	if (op == '+')
		result = value->number;
	else if (op == '-')
		result = -value->number;
	else
		return (CALC_INTERNAL_ERROR);

	if (calc_bad_double(result))
		return (CALC_NUMBER_OVERFLOW);

	*lhs = calc_make_number(result);
	return (CALC_OK);
}

t_semantic_result	calc_primary(const t_reduce_event& event, void *user)
{
	t_calc_context		*ctx;
	t_calc_value			*lhs;
	const t_calc_value	*value;

	ctx = static_cast<t_calc_context *>(user);
	if (ctx == NULL)
		return (CALC_INTERNAL_ERROR);
	lhs = calc_lhs(*ctx, event);
	if (lhs == NULL)
		return (CALC_INTERNAL_ERROR);

	if (event.rhs_count == 1)
		value = calc_rhs(*ctx, event, 0);
	else if (event.rhs_count == 5)
		value = calc_rhs(*ctx, event, 2);
	else
		return (CALC_INTERNAL_ERROR);

	if (!calc_number_value(value))
		return (CALC_INTERNAL_ERROR);

	*lhs = *value;
	return (CALC_OK);
}

t_semantic_result calc_start(const t_reduce_event& event, void *user)
{
	t_calc_context *ctx;
	t_calc_value *lhs;
	const t_calc_value *expression;

	ctx = static_cast<t_calc_context *>(user);
	if (ctx == NULL || event.rhs_count != 2)
		return (CALC_INTERNAL_ERROR);

	expression = calc_rhs(*ctx, event, 1);
	if (!calc_number_value(expression))
		return (CALC_INTERNAL_ERROR);

	lhs = calc_lhs(*ctx, event);
	if (lhs == NULL)
		return (CALC_INTERNAL_ERROR);

	*lhs = *expression;
	ctx->result = expression->number;
	ctx->has_result = true;
	return (CALC_OK);
}