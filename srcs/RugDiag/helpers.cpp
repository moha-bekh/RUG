// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#include "../../inc/RugDiag.hpp"

std::string	to_decimal( size_t value )
{
	std::ostringstream	out;

	out << value;
	return (out.str());
}

std::string	to_hex_byte( unsigned char value )
{
	const char	*digits;
	std::string res;

	digits = "0123456789ABCDEF";
	res = "\\x00";
	res[2] = digits[value >> 4];
	res[3] = digits[value & 15];

	return (res);
}

std::string	safe_text( const std::string& text )
{
	std::string 	res;
	unsigned char	c;

	for (size_t i = 0; i < text.size(); i++)
	{
		c = (unsigned char)text[i];

		if (c < 32 || c == 127)
			res += to_hex_byte(c);
		else
			res += (char)c;
	}

	return (res);
}

size_t		display_width( unsigned char c, size_t column )
{
	if (c == '\t')
		return (DIAG_TAB_WIDTH - column % DIAG_TAB_WIDTH);
	
	if ( c < 32 || c > 126)
		return (DIAG_TAB_WIDTH);
	
	return (1);
}

std::string	expand_line( const std::string& src, size_t begin, size_t end, 
						 size_t mark_begin, size_t mark_end, 
						 size_t& col_begin, size_t& col_end )
{
	std::string 	res;
	unsigned char	c;

	col_begin = 0;
	col_end = 0;

	for (size_t i = begin; i < end; i++)
	{
		if (i == mark_begin)
			col_begin = res.size();
		if (i == mark_end)
			col_end = res.size();

		c = (unsigned char)src[i];

		if (c == '\t')
			res.append(display_width(c, res.size()), ' ');
		else if (c < 32 || c > 126)
			res += to_hex_byte(c);
		else
			res += (char)c;
	}

	if (mark_begin == end)
		col_begin = res.size();
	if (mark_end == end)
		col_end = res.size();

	return (res);
}

const char	*level_name( e_rug_diag_level level )
{
	if (level == RUG_DIAG_ERROR)
		return ("error");

	if (level == RUG_DIAG_WARNING)
		return ("warning");

	return ("note");
}

const char	*level_color( e_rug_diag_level level )
{
	if (level == RUG_DIAG_ERROR)
		return ("\033[1;31m");

	if (level == RUG_DIAG_WARNING)
		return ("\033[1;33m");

	return ("\033[1;36m");
}

void		write_text( std::ostream& out, const std::string& text )
{	out.write(text.data(), text.size());	}

void		write_colored( std::ostream& out, const std::string& text, const char *color, bool enabled )
{
	if (enabled)
		write_text(out, color);

	write_text(out, text);

	if (enabled)
		write_text(out, "\033[0m");
}

RugDiag		report_with_source( const std::string& name, const char *data, size_t size )
{
	if (!data)
	{
		if (size != 0)
		{
			throw std::invalid_argument("RugDiag: null source with non-zero size");
			return (RugDiag(name));
		}
	}

	return (RugDiag(name, data, size));
}

std::string	terminal_name( t_id_symbol terminal )
{
	const char	*digits;
	std::string	hex;
	std::string	literal;

	if (terminal == CFG_END)
		return ("end of input");

	if (terminal > 255)
		throw std::invalid_argument("RugDiag::syntax: invalid terminal");

	digits = "0123456789ABCDEF";

	hex = "0x00";
	hex[2] = digits[terminal >> 4];
	hex[3] = digits[terminal & 15];

	if (terminal == '\n')
		return ("byte '\\n' (" + hex + ")");

	if (terminal == '\r')
		return ("byte '\\r' (" + hex + ")");

	if (terminal == '\t')
		return ("byte '\\t' (" + hex + ")");

	if (terminal >= 32 && terminal <= 126)
	{
		if (terminal == '\'' || terminal == '\\')
			literal += '\\';

		literal += (char)terminal;

		return ("byte '" + literal + "' (" + hex + ")");
	}

	return ("byte " + hex);
}