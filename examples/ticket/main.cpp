// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#include "Ticket.hpp"

#define PROG_NAME "my ticket db"

const char *semantic_error_message( int code )
{
	(void)code;
	return ("semantic callback failed");
}

static int	print_runtime_error(const e_parse_result result, const RugSession& session, const char *input, size_t input_size)
{
	if (result == PARSE_ERROR)
	{
		RugDiag::syntax(session.syntax_error(), input, input_size, PROG_NAME)
				 .print(std::cerr, true);
		return (1);
	}

	if (result == PARSE_SEMANTIC_ERROR)
	{
		const t_semantic_error&	error = session.semantic_error();
		RugDiag::semantic(session.semantic_error(), semantic_error_message(error.code), 
						  input, input_size, PROG_NAME).print(std::cerr, true);
		return (1);
	}

	if (result == PARSE_STACK_OVERFLOW)
	{
		RugDiag::stack_overflow(session.input_offset(), session.stack_capacity(), 
								input, input_size, PROG_NAME).print(std::cerr, true);
		return (1);
	}

	return (0);
}

static bool	success( e_parse_result res )
{
	if (res == PARSE_ERROR || res == PARSE_SEMANTIC_ERROR || res == PARSE_STACK_OVERFLOW)
		return (false);
	
	return (true);
}

std::string extract_file_content(const char *filename )
{
	std::string		out;
	std::streampos	size;
	std::ifstream	ifs(filename, std::ios::in | std::ios::binary);

	if (!ifs)
		throw std::runtime_error("ifstream() can not open file");

	ifs.seekg(0, std::ios::end);
	size = ifs.tellg();
	if (size < 0)
		throw std::runtime_error("ifstream().tellg() return error -1");
	
	ifs.seekg(0, std::ios::beg);
	out.resize((size_t)size);
	ifs.read(&out[0], size);

	if (!(ifs.good() || ifs.eof()))
		throw std::runtime_error("ifstream().good() || ifstream().eof() failed");

	return (out);
}

int main( int ac, char **av )
{
	if (ac != 2)
	{ 
		std::cerr<< "usage: " << av[0] << " \"raw ticket\"" << std::endl;
		return (1);
	}

	try
	{
		// user context
		static const size_t	PARSE_DEPTH = 256;

		std::string			raw = extract_file_content(av[1]);
		const char			*input = &raw[0];
		const size_t		input_size = raw.size();

		t_db	db;
		init_db(db, input);

		// api
		Rug		rug("examples/ticket/ticket.abnf");

		rug.bind("ticket", on_ticket);
		rug.bind("name", on_name);
		rug.bind("surname", on_surname);

		rug.build();

		// session init
		RugSession	session(rug, &db, PARSE_DEPTH);

		// run parser
		e_parse_result	result = session.push((const unsigned char*)input, input_size);

		// error mgmt
		if (!success(result))
			return (print_runtime_error(result, session, input, input_size));

		// inject CFG_END
		result = session.finish();
		if (!success(result))
			return (print_runtime_error(result, session, input, input_size));

		if (result != PARSE_ACCEPTED)
		{
			RugDiag::failure("parser finished without accepting the input", PROG_NAME).print(std::cerr, true);
			return (1);
		}

		// user output
		print_db(db);

		return (0);
	}
	catch (const RugBuildError& e)
	{
		e.print(std::cerr, true);
		return (1);
	}
	catch (const std::exception& e)
	{
		std::cerr << "Rug: " << e.what() << std::endl;
		return (1);
	}
	
	return (0);
}