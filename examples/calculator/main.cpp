// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#include <iostream>
#include <cstring>
#include <stdint.h>
#include <cstddef>
#include <new>

#include "Calculator.hpp"

const char *semantic_error_message(int code)
{
	if (code == CALC_DIVISION_BY_ZERO)
		return ("division by zero");
	if (code == CALC_MODULO_BY_ZERO)
		return ("division by zero");
	if (code == CALC_NUMBER_OVERFLOW)
		return ("number overflow");
	return ("calculator semantic callback failed");
}

static int	print_runtime_error(const e_parse_result result, const RugSession& session, const char *input, size_t input_size)
{
	if (result == PARSE_ERROR)
	{
		RugDiag::syntax(session.syntax_error(), input, input_size, "my calculator")
				 .print(std::cerr, true);
		return (1);
	}

	if (result == PARSE_SEMANTIC_ERROR)
	{
		const t_semantic_error&	error = session.semantic_error();
		RugDiag::semantic(session.semantic_error(), semantic_error_message(error.code), 
						  input, input_size, "my calculator").print(std::cerr, true);
		return (1);
	}

	if (result == PARSE_STACK_OVERFLOW)
	{
		RugDiag::stack_overflow(session.input_offset(), session.stack_capacity(), 
								input, input_size, "my calculator").print(std::cerr, true);
		return (1);
	}

	return (0);
}

int	main(int ac, char **av)
{
	if (ac != 2)
	{ 
		std::cerr<< "usage: " << av[0] << " \"expression\"" << std::endl;
		return (1);
	}

	try
	{
		// user context
		static const size_t	PARSE_DEPTH = 256;

		const char			*input = av[1];
		const size_t		input_size = std::strlen(input);

		t_calc_value	calc_stack[PARSE_DEPTH];
		t_calc_context	calc;

		calc_init(calc, input, input_size, calc_stack, PARSE_DEPTH);

		// rug init
		Rug	rug("examples/calculator/calculator.abnf");

		rug.bind("start", calc_start);
		rug.bind("expression", calc_expression);
		rug.bind("add-op", calc_add_op);
		rug.bind("term", calc_term);
		rug.bind("mul-op", calc_mul_op);
		rug.bind("unary", calc_unary);
		rug.bind("primary", calc_primary);
		rug.bind("number", calc_number);
		rug.bind("numeric", calc_numeric);

		rug.build();

		// session init
		RugSession	session(rug, &calc, PARSE_DEPTH);

		// run parser
		e_parse_result	result;

		result = session.push((const unsigned char*)input, input_size);

		// error mgmt
		if (result == PARSE_ERROR || result == PARSE_SEMANTIC_ERROR || result == PARSE_STACK_OVERFLOW)
			return (print_runtime_error(result, session, input, input_size));

		// inject CFG_END
		result = session.finish();
		if (result == PARSE_ERROR || result == PARSE_SEMANTIC_ERROR || result == PARSE_STACK_OVERFLOW)
			return (print_runtime_error(result, session, input, input_size));

		if (result != PARSE_ACCEPTED)
		{
			RugDiag::failure("parser finished without accepting the input", "calculator").print(std::cerr, true);
			return (1);
		}

		// user output
		std::cout << calc.result << std::endl;

		return (0);
	}
	catch (const RugBuildError& error)
	{
		error.print(std::cerr, true);
		return (1);
	}
	catch (const std::bad_alloc&)
	{
		std::cerr << "RUG: memory allocation failed" << std::endl;
		return (1);
	}
	catch (const std::exception& error)
	{
		RugDiag::failure(error.what(),"RUG").print(std::cerr, true);
		return (1);
	}
}