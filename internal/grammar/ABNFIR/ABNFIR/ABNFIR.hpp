// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <vector>
#include <cstddef>
#include <iostream>

#include "../../SourceFile/SourceFile.hpp"

typedef size_t	t_abnf_node_id;

static const t_abnf_node_id ABNF_INVALID_NODE = (size_t)-1;

enum e_abnf_node_type
{
	ABNF_REF,
	ABNF_CHAR_VAL,
	ABNF_NUM_VAL,
	ABNF_PROSE_VAL,
	ABNF_ALT,
	ABNF_CONCAT,
	ABNF_REPEAT,
	ABNF_OPTION
};

typedef struct	s_abnf_child
{
	size_t		begin;
	size_t		count;
}				t_abnf_child;

typedef struct		s_abnf_repeat
{
	t_abnf_node_id	child;
	size_t			min;
	size_t			max;
	bool			unbounded;
}					t_abnf_repeat;

typedef struct		s_abnf_unary
{
	t_abnf_node_id	child;
}					t_abnf_unary;

typedef struct		s_abnf_char_val
{
	t_source_span	span;
	bool			is_sensitive;
}					t_abnf_char_val;

enum e_abnf_num_kind
{
	ABNF_NUM_SINGLE,	// %d12
	ABNF_NUM_RANGE,		// %12-32
	ABNF_NUM_SEQUENCE	// %12.4.54
};

typedef struct		s_abnf_num_val
{
	e_abnf_num_kind	kind;
	int				base;
	size_t			value_begin;
	size_t			value_count;
}					t_abnf_num_val;

typedef struct			s_abnf_node
{
	e_abnf_node_type	type;
	t_source_span		span;

	union {
		t_abnf_child	child;
		t_abnf_repeat	repeat;
		t_abnf_unary	unary;
		t_abnf_char_val	char_val;
		t_abnf_num_val	num_val;
	};
}						t_abnf_node;

typedef struct		s_abnf_rule
{
	t_source_span	name;
	t_source_span	span;
	t_abnf_node_id	root_id;
	bool			incremental;
}					t_abnf_rule;

class ABNFIR
{
	private:
		// owned
		std::vector<t_abnf_node>	m_nodes_;
		std::vector<t_abnf_node_id>	m_children_ids_;
		std::vector<t_abnf_rule>	m_rules_;
		std::vector<size_t>			m_num_values_;

	public:
		// ocf
		ABNFIR( void );
		~ABNFIR( void );

		// printer debug
		void				print(const SourceFile& src) const;
		
		// leaf nodes : ABNF_REF, ABNF_PROSE_VAL
		t_abnf_node_id		make_leaf( const t_source_span& span, e_abnf_node_type type );

		// num / char values
		t_abnf_node_id		make_char_val( const t_source_span& span, const t_abnf_char_val& val );
		t_abnf_node_id		make_num_val( const t_source_span& span, const t_abnf_num_val& val, \
										  const std::vector<size_t>& values );

		// stucture nodes
		// alt -> concat -> repeat -> element -> alt (begin)
		t_abnf_node_id		make_repeat( const t_source_span& span, const t_abnf_repeat& rep );
		t_abnf_node_id		make_repeat( const t_source_span& span, \
										 t_abnf_node_id child_id, \
										 size_t min, size_t max, \
										 bool unbounded );
		t_abnf_node_id		make_option( const t_source_span& span, \
										 t_abnf_node_id child_id );
		t_abnf_node_id		make_concat( const t_source_span& span, \
										 const std::vector<t_abnf_node_id>& child_ids );
		t_abnf_node_id		make_alt( const t_source_span& span, \
									  const std::vector<t_abnf_node_id>& child_ids );

		// rules
		void				add_rule( const t_source_span& name, \
									  const t_source_span& span, \
									  t_abnf_node_id root_id, \
									  bool incremental );
		
		// getters
		size_t				num_value( const t_abnf_node& node, size_t index ) const;
		const t_abnf_node&	node( t_abnf_node_id id ) const;
		const t_abnf_rule&	rule( size_t id ) const;
		size_t				node_count( void ) const;
		size_t				rule_count( void ) const;
		t_abnf_node_id		child( const t_abnf_node& node, size_t index ) const;

	private:
		void				print_node( const SourceFile& src, t_abnf_node_id id, size_t depth ) const;
		void				print_indent( size_t depth ) const;
		const char*			node_type_to_str( e_abnf_node_type type ) const;
		t_abnf_node_id		make_children_node( e_abnf_node_type type, \
												const t_source_span& span, \
												const std::vector<t_abnf_node_id>& child_ids );
	private:
		/* forbideen OCF */
		ABNFIR( const ABNFIR& other );
		ABNFIR&	operator= ( const ABNFIR& other );
};
