// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#include "Ticket.hpp"

void				init_span( t_span_ticket& span )
{
	span.offset = 0;
	span.len = 0;
}

void				init_ticket( t_ticket& ticket )
{
	init_span(ticket.name);
	init_span(ticket.surname);
}

t_ticket			make_ticket( void )
{
	t_ticket	t;

	init_span(t.name);
	init_span(t.surname);

	return (t);
}

void				init_db( t_db& db, const char *addr )
{
	db.data = addr;
	db.v_db.clear();
	db.v_db.push_back(make_ticket());
	db.id = 0;
}

t_semantic_result	on_name( const t_reduce_event& event, void *ctx )
{
	t_db *db = (t_db*)ctx;
	t_ticket& current = db->v_db[db->id];

	current.name.offset = event.span.begin;
	current.name.len = event.span.end - event.span.begin;

	return (SEMANTIC_OK);
}

t_semantic_result	on_surname( const t_reduce_event& event, void *ctx )
{
	t_db *db = (t_db*)ctx;
	t_ticket& current = db->v_db[db->id];

	current.surname.offset = event.span.begin;
	current.surname.len = event.span.end - event.span.begin;

	return (SEMANTIC_OK);
}

t_semantic_result	on_ticket( const t_reduce_event& event, void *ctx )
{
	(void)event;

	t_db *db = (t_db*)ctx;

	db->v_db.push_back(make_ticket());
	db->id++;

	return (SEMANTIC_OK);
}

void				print_span( t_span_ticket& span, const char *addr )
{
	std::cout.write(&addr[span.offset], span.len);
}

void				print_ticket( t_ticket& ticket, const char *addr )
{
	std::cout << "name: ";
	print_span(ticket.name, addr);
	std::cout << std::endl;

	std::cout << "surname: ";
	print_span(ticket.surname, addr);
	std::cout << std::endl;
}

void				print_db( t_db& db )
{
	for (size_t i = 0; i < db.v_db.size() - 1; i++)
	{
		std::cout << "id: " << i + 1; 
		std::cout << std::endl;
		print_ticket(db.v_db[i], db.data);
		std::cout << std::endl;
	}
}