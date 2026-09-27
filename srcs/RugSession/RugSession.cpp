// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#include "../../inc/RugSession.hpp"

/* OCF */

RugSession::RugSession( const Rug& rug, void *user_ctx, size_t stack_capacity )
:	m_rug_(rug),
	m_workspace_(NULL),
	m_workspace_size_(0),
	m_stack_capacity_(0),
	mb_owns_workspace_(false),
	m_input_offset_(0),
	m_session_storage_(),
	m_session_(NULL)
{
	if (!m_rug_.is_built())
		throw std::logic_error("RugSession: Rug is not built");
	
	size_t	size = workspace_size(stack_capacity);

	void	*workspace = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (workspace == MAP_FAILED)
				throw std::runtime_error("RugSession: mmap failed");

	mb_owns_workspace_ = true;

	try
	{
		init(user_ctx, workspace, size);
	}
	catch ( ... )
	{
		munmap(workspace, size);
		mb_owns_workspace_ = false;

		throw ;
	}
}

RugSession::RugSession( const Rug& rug, void *user_ctx, void *workspace, size_t workspace_size )
:	m_rug_(rug),
	m_workspace_(NULL),
	m_workspace_size_(0),
	m_stack_capacity_(0),
	mb_owns_workspace_(false),
	m_input_offset_(0),
	m_session_storage_(),
	m_session_(NULL)
{
	if (!m_rug_.is_built())
		throw std::logic_error("RugSession: Rug is not built");

	init(user_ctx, workspace, workspace_size);
}

RugSession::~RugSession( void )
{
	if (m_session_)
	{
		m_session_->~Session();
		m_session_ = NULL;
	}

	if (mb_owns_workspace_ && m_workspace_)
		munmap(m_workspace_, m_workspace_size_);
	
	m_workspace_ = NULL;
}

/* WORKSPACE LAYOUT */

size_t			RugSession::workspace_size( size_t stack_capacity )
{
	if (stack_capacity < 2)
		throw std::logic_error("RugSession: stack capacity must be at least 2");
	
	/*
		Layout:

		state_stack[stack_capacity]
		padding
		span_stack[stack_capacity - 1]
	*/

	if (stack_capacity > (size_t)-1 / sizeof(t_id_state))
		throw std::overflow_error("RugSession: state stack size overflow");

	size_t	state_bytes = stack_capacity * sizeof(t_id_state);
	size_t	span_offset = align_up(state_bytes, sizeof(size_t));
	size_t	span_count = stack_capacity - 1;

	if (span_count > (size_t)-1 / sizeof(t_parse_span))
		throw std::overflow_error("RugSession: span stack size overflow");

	size_t	span_bytes = span_count * sizeof(t_parse_span);

	if (span_offset > (size_t)-1 - span_bytes)
		throw std::overflow_error("RugSession: workspace size overflow");

	return (span_offset + span_bytes);
}

size_t		RugSession::capacity_from_workspace( size_t available )
{
	if (available < workspace_size(2))
		return (0);
	
	size_t	element_cost = sizeof(t_id_state) + sizeof(t_parse_span);
	size_t	capacity = available / element_cost + 1;

	if (capacity < 2)
		capacity = 2;

	while (capacity > 2 && workspace_size(capacity) > available)
		capacity--;
	
	while (capacity < (size_t)-1)
	{
		size_t next = capacity + 1;
		if (workspace_size(next) > available)
			break ;
		capacity = next;
	}

	return (capacity);
}

/* INIT */

void		RugSession::init( void *user_ctx, void *workspace, size_t workspace_size )
{
	if (!workspace)
		throw std::logic_error("RugSession: null workspace");
	if (((uintptr_t)workspace % sizeof(size_t)) != 0)
		throw std::runtime_error("RugSession: workspace is not aligned");

	size_t			capacity = capacity_from_workspace(workspace_size);

	if (capacity < 2)
		throw std::runtime_error("RugSession: workspace too small");

	uint8_t			*base = (uint8_t*)workspace;

	size_t			state_bytes = capacity * sizeof(t_id_state);
	size_t			span_offset = align_up(state_bytes, sizeof(size_t));

	t_id_state		*state_stack = (t_id_state*)base;
	t_parse_span	*span_stack = (t_parse_span*)(base + span_offset);

	m_workspace_ = workspace;
	m_workspace_size_ = workspace_size;
	m_stack_capacity_ = capacity;

	m_session_ = new (m_session_storage_.data) 
				Session(m_rug_.table(), state_stack, 
				capacity, span_stack, capacity - 1, 
				m_rug_.semantics(), user_ctx);
}

/* FRONTEND */

/*
	THE USER MUST NEVER PASS:

	a NULL pointer
	a total size (streaming included),
	exceeding (size_t)-1 (this will overflow m_input_offset)
*/
e_parse_result			RugSession::push( const unsigned char *data, size_t size)
{
	for (size_t i = 0; i < size; i++)
	{
		t_parse_span	span;

		span.begin = m_input_offset_;
		span.end = m_input_offset_ + 1;

		e_parse_result res = m_session_->push_terminal((t_id_symbol)data[i], span);
		if (res != PARSE_CONSUMED)
			return (res);

		m_input_offset_++;
	}

	return (PARSE_CONSUMED);
}

/* tools and getters */

e_parse_result	RugSession::finish(void)
{	return (m_session_->finish(m_input_offset_));	}

void	RugSession::reset(void)
{
	m_session_->reset_session();
	m_input_offset_ = 0;
}

size_t	RugSession::input_offset(void) const
{	return (m_input_offset_);	}

size_t	RugSession::stack_capacity(void) const
{	return (m_stack_capacity_);	}


const t_parse_error&	RugSession::syntax_error(void) const
{	return (m_session_->error());	}


const t_semantic_error&	RugSession::semantic_error(void) const
{	return (m_session_->semantic_error());	}