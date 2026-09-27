// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#include "Lexer.hpp"

static const char*	token_type_to_string(e_token_type type)
{
	switch (type)
	{
		case T_RULENAME:		return ("RULENAME");
		case T_NUMBER:			return ("NUMBER");

		case T_EQUAL:			return ("EQUAL");
		case T_EQUAL_SLASH:		return ("EQUAL_SLASH");
		case T_SLASH:			return ("SLASH");
		case T_STAR:			return ("STAR");

		case T_LPAREN:			return ("LPAREN");
		case T_RPAREN:			return ("RPAREN");
		case T_LBRACKET:		return ("LBRACKET");
		case T_RBRACKET:		return ("RBRACKET");

		case T_CHAR_VAL:		return ("CHAR_VAL");
		case T_I_CHAR_VAL:		return ("I_CHAR_VAL");
		case T_S_CHAR_VAL:		return ("S_CHAR_VAL");

		case T_D_NUM_VAL:		return ("D_NUM_VAL");
		case T_X_NUM_VAL:		return ("X_NUM_VAL");
		case T_B_NUM_VAL:		return ("B_NUM_VAL");

		case T_WSP:				return ("WSP");
		case T_NL:				return ("NL");

		case T_PROSE_VAL:		return ("PROSE_VAL");

		case T_END:				return ("END");
	}

	return ("UNKNOWN");
}

void				print_tokens( const SourceFile& src, const t_token_buffer& buf )
{
	for (size_t i = 0; i < buf.size(); ++i)
	{
		const t_token& tok = buf[i];

		std::cout	<< std::setw(3) << i << " | " \
					<< std::setw(12) << token_type_to_string(tok.type) << " | " \
					<< tok.span.line << ":" << tok.span.column \
					<< " | off=" << tok.span.offset << " len=" << tok.span.len \
					<< " | [";

		if (tok.type == T_NL)
			std::cout << "NL";
		else if (tok.type != T_END)
			std::cout.write(src.data() + tok.span.offset, tok.span.len);

		std::cout << "]\n";
	}
}