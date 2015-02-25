/*
 * mapredo
 * Copyright (C) 2015 Kjell Irgens <hextremist@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 */

#include "consumer.h"
#include "mapreducer.h"

consumer::consumer (mapredo::base& mapreducer,
		    const std::string& tmpdir,
		    const bool is_subdir,
		    const uint16_t buckets,
		    const uint16_t worker_id,
		    const size_t bytes_buffer,
		    const bool reverse) :
    _mapreducer (mapreducer),
    _tmpdir (tmpdir),
    _is_subdir (is_subdir),
    _buckets (buckets),
    _worker_id (worker_id)
{
    for (size_t i = 0; i < buckets; i++)
    {
	_sorters.emplace_back (_tmpdir, i, worker_id, bytes_buffer,
			       mapreducer.type(), reverse);
    }
}

consumer::~consumer()
{
    join_thread();
}

void
consumer::start_thread (buffer_trader& trader)
{
    _thread = std::thread (&consumer::work, this, std::ref(trader));
}

void
consumer::join_thread()
{
    if (_thread.joinable()) _thread.join();
}

void
consumer::append_tmpfiles (const size_t index, std::list<std::string>& files)
{
    auto iter (_tmpfiles.find (index));

    if (iter != _tmpfiles.end()) files.splice (files.end(), iter->second);
}

void
consumer::work (buffer_trader& trader)
{
    try
    {
	auto* buffer = trader.consumer_get (_worker_id);

	if (!buffer) return;
	std::stringstream stream;

	size_t pos;

	do
	{
	    char* buf = buffer->get();
	    size_t start = buffer->start();
	    const size_t end = buffer->end();

	    while (start < end)
	    {
		for (pos = start; pos < end && buf[pos] != '\n'; pos++) ;

		if (pos == start || buf[pos-1] != '\r')
		{
		    buf[pos] = '\0';
		    _mapreducer.map (buf + start, pos - start, *this);
		}
		else
		{
		    buf[pos-1] = '\0';
		    _mapreducer.map (buf + start, pos - start - 1, *this);
		}
		start = pos + 1;
	    }
	}
	while ((buffer = trader.consumer_swap(buffer, _worker_id)));

	for (auto& sorter: _sorters)
	{
	    sorter.flush();
	    _tmpfiles[sorter.hash_index()] = sorter.grab_tmpfiles();
	}
	_sorters.clear();
    }
    catch (...)
    {
	_texception = std::current_exception();
	trader.consumer_fail (_worker_id);
    }
}

static unsigned int hash (const char* str, size_t siz)
{
    unsigned int result = 0x55555555;
    size_t i;

    for (i = 0; i < siz && str[i] != '\t'; i++)
    {
	result ^= str[i];
	result = ((result << 5) | (result >> 27));
    }
    return result;
}


void
consumer::collect (const char* inbuffer, const size_t insize)
{
    if (_buckets > 1)
    {
	_sorters[hash(inbuffer, insize) % _buckets].add (inbuffer, insize);
    }
    else _sorters[0].add (inbuffer, insize);
}