// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#include "../../inc/Rug.hpp"

void		Rug::bind( const std::string& rule_name, t_reduce_callback function )
{
	if (mb_built_)
		throw std::logic_error("Rug::bind: program already built");
	if (!function)
		throw std::logic_error("Rug::bind: null function");
	if (!m_build_data_)
		throw std::logic_error("Rug::bind: missing build data");

	/*
		if required_size() has already been called, it invalidate the preparation
		the next call to required_size() will compile again
	*/
	if (m_build_data_->mb_prepared_)
	{
		// invalidate previous artifact
		// grammar and pending bindings are preserved
		// and set the BuildData to NOT PREPARED
		std::vector<uint8_t>().swap(m_build_data_->mv_parsing_table_);
		std::vector<t_reduce_callback>().swap(m_build_data_->mv_callback_table_);

		m_build_data_->m_callbacks_offset_ = 0;
		m_build_data_->m_required_size_ = 0;
		m_build_data_->mb_prepared_ = false;
	}

	// add the new binding to m_build_data_
	t_pending_binding	binding;

	binding.name = rule_name;
	binding.callback = function;

	m_build_data_->mv_bindings_.push_back(binding);
}

void	Rug::prepare( void )
{
	if (mb_built_)
		return ;
	if (!m_build_data_)
		throw std::logic_error("Rug::prepare: missing build state");
	if (m_build_data_->mb_prepared_)
		return ;

	// abnf parsing
	SourceFile			src(m_build_data_->m_abnf_file_.c_str());
	DiagnosticEngine 	diag(src, m_build_data_->m_abnf_file_);
	// DiagnosticEngine	diag(src);
	t_token_buffer		tokens;
	Lexer				lexer(src, tokens, diag);

	lexer.tokenize();
	check_diagnostics(diag, "Rug: Lexer failed");

	ABNFIR				abnf_ir;
	ABNFParser			abnf_parser(src, tokens, diag, abnf_ir);

	if (!abnf_parser.parse())
	{
		diag.throw_if_errors("Rug: ABNF parser failed");
		throw std::logic_error("Rug: ABNF parser failed without diagnostics");
	}

	ABNFValidator		abnf_validator(src, abnf_ir, diag);

	if (!abnf_validator.validate())
	{
		diag.throw_if_errors("Rug: ABNF validation failed");
		throw std::logic_error("Rug: ABNF validation failed without diagnostics");
	}

	// CFG
	std::string			start_rule = get_start_rule(src, abnf_ir);
	CFG					cfg;
	CFGBuilder			cfg_builder(src, abnf_ir, diag, cfg, start_rule, false);

	cfg_builder.build();
	check_diagnostics(diag, "Rug: CFG build failed");

	// LR1
	LRGraph				lr_graph;
	LRBuilder			lr_builder(cfg, lr_graph);

	lr_builder.build();

	// LALR
	LRGraph				lalr_graph;
	LALRBuilder			lalr_builder(lr_graph, lalr_graph);

	lalr_builder.build();

	// Table IR
	TableIR				table_ir(lalr_graph.state_count(), cfg.non_terminal_count());
	TableIRBuilder		table_ir_builder(cfg, lalr_graph, table_ir);

	table_ir_builder.build();

	// Semantic 
	t_reduce_callback	null_callback = NULL;
	t_reduce_callback	*callbacks_addr = NULL;

	m_build_data_->mv_callback_table_.assign(m_build_data_->mv_bindings_.size(), null_callback);
	
	if (!m_build_data_->mv_callback_table_.empty())
		callbacks_addr = &m_build_data_->mv_callback_table_[0];
	
	SemanticBindings	semantic_bindings(cfg, callbacks_addr, m_build_data_->mv_callback_table_.size());

	for (size_t i = 0; i < m_build_data_->mv_bindings_.size(); i++)
	{
		semantic_bindings.bind(
			m_build_data_->mv_bindings_[i].name,
			m_build_data_->mv_bindings_[i].callback);
	}

	// Packed Table
	TableBuilder		table_builder(cfg, table_ir, semantic_bindings);
	size_t				parsing_table_size = table_builder.required_size();

	m_build_data_->mv_parsing_table_.resize(parsing_table_size);

	if (parsing_table_size == 0)
		throw std::logic_error("Rug: empty parser blob");

	table_builder.build_into(&m_build_data_->mv_parsing_table_[0], parsing_table_size);
	// validate the final blob once during init
	TableView			validation_view(&m_build_data_->mv_parsing_table_[0], parsing_table_size);
	(void)validation_view;

	// Runtime image build
	size_t				callback_alignement = sizeof(void*);
	
	m_build_data_->m_callbacks_offset_ = align_up(parsing_table_size, callback_alignement);

	if (m_build_data_->mv_callback_table_.size() > ((size_t)-1) / sizeof(t_reduce_callback))
		throw std::overflow_error("Rug: callback table size overflow");
	
	size_t				callback_bytes = m_build_data_->mv_callback_table_.size() * sizeof(t_reduce_callback);

	if (m_build_data_->m_callbacks_offset_ > (size_t)-1 - callback_bytes)
		throw std::overflow_error("Rug: runtime image size overflow");

	m_build_data_->m_required_size_ = m_build_data_->m_callbacks_offset_ + callback_bytes;

	m_build_data_->mb_prepared_ = true;
}

size_t	Rug::required_alloc_size( void )
{
	if (mb_built_)
		return (m_image_size_);

	prepare();

	return (m_build_data_->m_required_size_);
}

void	Rug::build( void )
{
	if (mb_built_)
		throw std::logic_error("Rug::build already built");
	
	prepare();

	size_t	size = m_build_data_->m_required_size_;

	void	*workspace = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (workspace == MAP_FAILED)
		throw std::runtime_error("Rug::build: mmap failed");

	try
	{
		finalize_into(workspace, size, true);
	}
	catch ( ... )
	{
		munmap(workspace, size);
		throw ;
	}
}

void	Rug::build_into( void *addr, size_t capacity )
{
	if (mb_built_)
		throw std::logic_error("Rug::build_into already built");
	
	prepare();
	finalize_into(addr, capacity, false);
}

void	Rug::finalize_into( void *addr, size_t size, bool owns_workspace )
{
	if (!addr)
		throw std::logic_error("Rug: null runtime workspace");
	if (!m_build_data_ || !m_build_data_->mb_prepared_)
		throw std::logic_error("Rug: program was not prepared");
	if (size < m_build_data_->m_required_size_)
		throw std::runtime_error("Rug: runtime workspace too small");
	if (((uintptr_t)addr % sizeof(void*)) != 0)
		throw std::runtime_error("Rug: runtime workspace is not aligned");

	// byte level ptr conversion (+ 1) == (+ 8 bits)
	uint8_t *base = (uint8_t*)addr;

	// copy from temporary allocated m_build_data_
	// copy parsing table
	std::memcpy(base, &m_build_data_->mv_parsing_table_[0], m_build_data_->mv_parsing_table_.size());
	// alignement padding8 bits
	if (m_build_data_->m_callbacks_offset_ > m_build_data_->mv_parsing_table_.size())
	{
		std::memset(base + m_build_data_->mv_parsing_table_.size(), 0,
					m_build_data_->m_callbacks_offset_ - m_build_data_->mv_parsing_table_.size());
	}
	// copy callback table
	if (!m_build_data_->mv_callback_table_.empty())
	{
		std::memcpy(base + m_build_data_->m_callbacks_offset_,
					&m_build_data_->mv_callback_table_[0],
					m_build_data_->mv_callback_table_.size() * sizeof(t_reduce_callback));
	}

	m_image_ = addr;
	m_image_size_ = m_build_data_->m_required_size_;

	m_parsing_table_size_ = m_build_data_->mv_parsing_table_.size();
	m_callbacks_offset_ = m_build_data_	->m_callbacks_offset_;
	m_callback_count_ = m_build_data_->mv_callback_table_.size();

	mb_owns_memory_ = owns_workspace;

	// construct runtime viewa
	try
	{
		m_table_ = new (m_table_storage_.data) TableView(m_image_, m_parsing_table_size_);
		
		const t_reduce_callback	*callbacks = NULL;
		if (m_callback_count_ != 0)
			callbacks = (const t_reduce_callback*)(base + m_callbacks_offset_);
		
		m_semantics_ = new (m_semantic_storage_.data) SemanticRuntime(callbacks, m_callback_count_);
	}
	catch ( ... )
	{
		destroy_runtime_views();

		m_image_ = NULL;
		m_image_size_ = 0;
		m_parsing_table_size_ = 0;
		m_callbacks_offset_ = 0;
		m_callback_count_ = 0;
		mb_owns_memory_ = false;

		throw ;
	}

	mb_built_ = true;

	// destruction / deletion of RugBuildData
	// all stl allocated data is destroyed here
	delete m_build_data_;
	m_build_data_ = NULL;
}

void	Rug::destroy_runtime_views( void )
{
	if (m_semantics_)
	{
		m_semantics_->~SemanticRuntime();
		m_semantics_ = NULL;
	}

	if (m_table_)
	{
		m_table_->~TableView();
		m_table_ = NULL;
	}
}

/* GETTERS */

const TableView&		Rug::table( void ) const
{	return (*m_table_);	}

const SemanticRuntime&	Rug::semantics(void) const
{	return (*m_semantics_);	}

bool	Rug::owns_memory(void) const
{	return (mb_owns_memory_);	}

bool	Rug::is_built( void ) const
{	return (mb_built_);	}

size_t	Rug::image_size(void) const
{	return (m_image_size_);	}

size_t	Rug::parsing_table_size(void) const
{	return (m_parsing_table_size_);	}

size_t	Rug::semantic_count(void) const
{	return (m_callback_count_);	}