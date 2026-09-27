// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../../inc/Rug.hpp"
#include "../../inc/RugSession.hpp"

#include <vector>

typedef struct	s_span_ticket
{
	size_t		offset;
	size_t		len;
}				t_span_ticket;

typedef struct		s_ticket
{
	t_span_ticket	name;
	t_span_ticket	surname;
	// t_span_ticket	birthdate;

	// t_span_ticket	from;
	// t_span_ticket	to;

	// t_span_ticket	flight_date;
	// t_span_ticket	flight_time;
	// t_span_ticket	flight_duration;
}					t_ticket;

typedef struct				s_db
{
	const char				*data;
	std::vector<t_ticket> 	v_db;
	size_t					id;
}							t_db;

void				init_span( t_span_ticket& span );
void				init_ticket( t_ticket& ticket );
t_ticket			make_ticket( void );
void				init_db( t_db& db, const char *addr );

t_semantic_result	on_ticket( const t_reduce_event& event, void *ctx );
t_semantic_result	on_name( const t_reduce_event& event, void *ctx );
t_semantic_result	on_surname( const t_reduce_event& event, void *ctx );

void				print_span( t_span_ticket& span, const char *addr );
void				print_ticket( t_ticket& ticket, const char *addr );
void				print_db( t_db& db );

// t_semantic_result	on_birthdate( const t_reduce_event& event, void *ctx );
// t_semantic_result	on_from( const t_reduce_event& event, void *ctx );
// t_semantic_result	on_to( const t_reduce_event& event, void *ctx );
// t_semantic_result	on_date( const t_reduce_event& event, void *ctx );
// t_semantic_result	on_time( const t_reduce_event& event, void *ctx );
// t_semantic_result	on_duration( const t_reduce_event& event, void *ctx );
