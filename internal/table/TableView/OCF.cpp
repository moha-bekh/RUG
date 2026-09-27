// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#include "TableView.hpp"

TableView::TableView(const void *blob, size_t capacity)
:	m_base_((const uint8_t*)blob),
	m_hdr_((const t_table_blob_hdr*)blob),
	m_capacity_(capacity)
{
	if (!blob)
		throw std::logic_error("TableView: null blob");

	if (capacity < sizeof(t_table_blob_hdr))
		throw std::runtime_error("TableView: blob smaller than header");

	validate();
}

TableView::~TableView(void)
{	}