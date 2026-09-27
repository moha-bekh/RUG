// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#include "../../inc/RugDiag.hpp"

void				RugDiag::build_line_index( void )
{
	mv_line_starts_.push_back(0);

	for (size_t i = 0; i < m_source_.size(); i++)
	{
		if (m_source_[i] == '\r')
		{
			if (i + 1 < m_source_.size() && m_source_[i + 1] == '\n')
				i++;
			mv_line_starts_.push_back(i + 1);
		}
		else if (m_source_[i] == '\n')
			mv_line_starts_.push_back(i + 1);
	}
}

void				RugDiag::print( std::ostream& out, bool color ) const
{
	for (size_t i = 0; i < mv_entries_.size(); i++)
	{
		if (i != 0 && mv_entries_[i].level != RUG_DIAG_NOTE)
			write_text(out, "\n");
		print_entry(out, mv_entries_[i], color);
	}
}

void				RugDiag::print_entry( std::ostream& out, const t_rug_diag& diag, bool color ) const
{
	std::vector<size_t>::const_iterator cit;
	std::string							name;
	size_t								ln_idx;
	size_t								ln_begin;
	size_t								ln_end;
	size_t								mark_begin;
	size_t								mark_end;
	size_t								col_begin;
	size_t								col_end;
	std::string							line_raw;
	std::string							line_number;
	std::string							gutter;
	size_t								first;
	size_t								shown;
	std::string							prefix;
	std::string							suffix;
	size_t								mark_width;

	write_colored(out, level_name(diag.level), level_color(diag.level), color);
	write_text(out, ": " + safe_text(diag.message) + "\n");

	name = m_name_.empty() ? "<input>" : safe_text(m_name_);

	if (!diag.has_span)
	{
		if (!m_name_.empty() && diag.level != RUG_DIAG_NOTE)
			write_text(out, " --> " + name + "\n");

		return ;
	}

	if (!mb_has_source_ || diag.offset > m_source_.size())
	{
		write_text(out, " --> " + name +  ": byte " + to_decimal(diag.offset) + "\n");
		if (mb_has_source_)
			write_text(out, "  = source unavailable at this offset\n");

		return ;
	}

	cit = std::upper_bound(mv_line_starts_.begin(), mv_line_starts_.end(), diag.offset);

	ln_idx = (size_t)(cit - mv_line_starts_.begin() - 1);
	ln_begin = mv_line_starts_[ln_idx];
	ln_end = m_source_.size();

	if (ln_idx + 1 < mv_line_starts_.size())
		ln_end = mv_line_starts_[ln_idx + 1];

	if (ln_end > ln_begin && m_source_[ln_end - 1] == '\n')
		ln_end--;
	if (ln_end > ln_begin && m_source_[ln_end - 1] == '\r')
		ln_end--;
	
	mark_begin = std::min(diag.offset, ln_end);
	mark_end = mark_begin + std::min(diag.lenght, ln_end - mark_begin);

	// will init col_begin and col_end
	line_raw = expand_line(m_source_, ln_begin, ln_end, mark_begin, mark_end, col_begin, col_end);
	line_number = to_decimal(ln_idx + 1);
	gutter = std::string(line_number.size(), ' ');

	write_text(out, " --> " + name + ":" + line_number + ":" + to_decimal(diag.offset - ln_begin + 1) + "\n");
	write_text(out, gutter + " |\n");

	first = 0;

	if (line_raw.size() > DIAG_WINDOW_WIDTH && col_begin > DIAG_WINDOW_WIDTH - 25)
		first = col_begin - 25;

	if (line_raw.size() > DIAG_WINDOW_WIDTH && first > line_raw.size() - DIAG_WINDOW_WIDTH)
		first = line_raw.size() - DIAG_WINDOW_WIDTH;

	shown = std::min((size_t)DIAG_WINDOW_WIDTH, line_raw.size() - first);

	if (first == 0)
		prefix = "";
	else
		prefix = "...";
	
	if (shown < line_raw.size() - first)
		suffix = "...";
	else
		suffix = "";

	write_text(out, line_number + " | " + prefix + line_raw.substr(first, shown) + suffix + "\n");

	mark_width = std::min(col_end -  col_begin, first + shown - col_begin);
	if (mark_width == 0)
		mark_width = 1;

	write_text(out, gutter + " | " + std::string(prefix.size() + col_begin - first, ' '));
	write_colored(out, "^" + std::string(mark_width - 1, '~'), level_color(diag.level), color);
	write_text(out, "\n");

	if (diag.lenght > ln_end - mark_end - mark_begin)
		write_text(out, gutter + " = selection extends past this line\n");
}
