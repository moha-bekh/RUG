// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#include "../../inc/RugDiag.hpp"

RugDiag		RugDiag::failure( const std::string& message, const std::string& name )
{
	RugDiag	report(name);

	report.error(message);

	return (report);
}

RugDiag		RugDiag::syntax( const t_parse_error& error, const char *data, size_t size,
							  const std::string& name )
{
	bool				at_end;
	std::ostringstream	context;

	if (!error.has_error)
		throw std::invalid_argument("RugDiag::syntax no syntax error");
	
	RugDiag				report = report_with_source(name, data, size);

	at_end = (error.terminal == CFG_END);

	report.error(error.offset, at_end ? 0 : 1, "unexpected " + terminal_name(error.terminal));
	context << "parser state: " << error.state;
	report.note(context.str());

	return (report);
}

RugDiag		RugDiag::semantic( const t_semantic_error& error, const std::string& message,
					  const char *data, size_t size, const std::string& name )
{
	std::ostringstream	context;

	if (!error.has_error)
		throw std::invalid_argument("RugDiag::semantic no semantic error");
	if (error.event.span.end < error.event.span.begin)
		throw std::invalid_argument("RugDiag::semantic: invalid span");
	
	RugDiag				report = report_with_source(name, data, size);

	report.error(error.event.span.begin, error.event.span.end - error.event.span.begin, message);
	context << "user code: " << error.code;
	context << "; rule: " << error.event.rule_id;

	return (report);
}

RugDiag		RugDiag::stack_overflow( size_t offset, size_t capacity, const char *data, size_t size,
									  const std::string& name )
{
	std::ostringstream	context;

	RugDiag				report = report_with_source(name, data, size);

	report.error(offset, 0, "parser stack capacity exceeded");

	context << "configured capacity: ";
	context << capacity;
	context << " states";

	report.note(context.str());

	return (report);
}


