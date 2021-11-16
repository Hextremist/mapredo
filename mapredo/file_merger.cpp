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

#include <sstream>
#include <stdexcept>
#include <thread>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <cstdio>
#include <memory>
#include <cerrno>

#include "file_merger.h"
#include "valuelist.h"

file_merger::file_merger (mapredo::base& reducer,
			  std::list<std::string>&& tmpfiles,
			  const std::string& tmpdir,
			  const size_t index,
			  const size_t max_open_files) :
    _reducer (reducer),
    _max_open_files (max_open_files),
    _tmpfiles (tmpfiles)
{
    std::ostringstream filename;

    filename << tmpdir << "/merge_" << std::this_thread::get_id()
	     << ".w" << index << '.';
    _file_prefix = filename.str();

    if (max_open_files < 3)
    {
	throw std::runtime_error ("Can not operate on less than three files"
				  " per bucket");
    }

    if (settings::instance().compressed()) 
    {
        _compressor.reset (new compression());
    }
}

file_merger::file_merger (file_merger&& other) :
    _reducer (other._reducer),
    _max_open_files (other._max_open_files),
    _file_prefix (std::move(other._file_prefix)),
    _tmpfiles (std::move(other._tmpfiles))
{}

file_merger::~file_merger()
{}

void
file_merger::merge()
{
    while (!_tmpfiles.empty())
    {
	merge_max_files (TO_OUTPUT);
	if (_texception) return;
    }
}

std::string
file_merger::merge_to_file (prefered_output* alt_output)
{
    try
    {
	do
	{
	    merge_max_files (TO_SINGLE_FILE, alt_output);
	}
	while (_tmpfiles.size() > 1);

	return _tmpfiles.front();
    }
    catch (...)
    {
	_texception = std::current_exception();
	return ("");
    }
}

std::list<std::string>
file_merger::merge_to_files()
{
    try
    {
	while (_tmpfiles.size() > _num_merged_files
	       || _tmpfiles.size() > _max_open_files)
	{
	    if (_tmpfiles.size() == _num_merged_files)
	    {
		// We have to re-merge files because we still have too many
		_num_merged_files = 0;
	    }
	    merge_max_files (TO_MAX_FILES);
	}

	return _tmpfiles;
    }
    catch (...)
    {
	_texception = std::current_exception();
	return (std::list<std::string>());
    }
}

void
file_merger::merge_max_files (const file_merger::merge_mode mode,
			      prefered_output* alt_output)
{
    switch (_reducer.type())
    {
    case mapredo::base::keytype::STRING:
    {
	do_merge<char*> (mode, alt_output,
			 settings::instance().reverse_sort());
	break;
    }
    case mapredo::base::keytype::DOUBLE:
    {
	do_merge<double> (mode, alt_output,
			  settings::instance().reverse_sort());
	break;
    }
    case mapredo::base::keytype::INT64:
    {
	do_merge<int64_t> (mode, alt_output,
			   settings::instance().reverse_sort());
	break;
    }
    case mapredo::base::keytype::UNKNOWN:
    {
	throw std::runtime_error ("Program error, keytype not set"
				  " in mapredo::base");
    }
    }
}

void
file_merger::collect (const char* line, const size_t length)
{
    if (_buffer_pos + length >= _buffer_size) flush();
    memcpy (_buffer + _buffer_pos, line, length);
    _buffer_pos += length;
    _buffer[_buffer_pos++] = '\n';
}

char*
file_merger::reserve (const size_t bytes)
{
    _reserved_bytes = bytes;
    if (_buffer_pos + bytes >= _buffer_size) flush();
    return (_buffer + _buffer_pos);
}

void
file_merger::collect_reserved (const size_t length)
{
    if (_reserved_bytes == 0)
    {
	throw std::runtime_error
	    ("No memory reserved via reserve() in"
	     " file_merger::collect_reserved()");
    }

    if (length == 0) _buffer_pos += _reserved_bytes;
    else _buffer_pos += length;
    _buffer[_buffer_pos++] = '\n';

    _reserved_bytes = 0;
}

template <typename T> void
file_merger::do_merge (const merge_mode mode, prefered_output* alt_output,
		       const bool reverse)
{
    data_reader_queue<T> queue (reverse);
    size_t files;

    if (mode == TO_MAX_FILES)
    {
	files = std::min (_tmpfiles.size() - _num_merged_files,
			  _max_open_files);
    }
    else files = std::min (_tmpfiles.size(), _max_open_files);

    // Move max tmpfiles to queue of data readers
    for (size_t i = 0; i < files; ++i)
    {
	const std::string& filename = _tmpfiles.front();
	auto* proc = new tmpfile_reader<T>
	    (filename, 0x100000, !settings::instance().keep_tmpfiles());
	const T* key = proc->next_key();

	if (key) queue.push(proc);
        else if (!settings::instance().keep_tmpfiles())
	{
            remove (filename.c_str());  // Remove exhausted file
	}

	_tmpfiles.pop_front();
    }
    if (settings::instance().verbose())
    {
        std::ostringstream stream; // not to mix output with other threads
	stream << "Processing " << queue.size() << " tmpfiles, "
	       << _tmpfiles.size() << " left\n";
        std::cerr << stream.str();
    }

    if (queue.empty())
    {
	throw std::runtime_error ("Queue should not be empty here");
    }

    _num_merged_files++;

    if (_tmpfiles.empty() && mode == TO_OUTPUT) // last_merge, run reducer
    {
        if (settings::instance().verbose())
        {
          std::cerr << "Last merge, run reducer\n";
        }
	mapredo::valuelist<T> list (queue);

	while (!queue.empty())
	{
	    static_cast<mapredo::mapreducer<T>&>(_reducer).reduce
		(list.get_key(), list, *this);
	}
	flush();
    }
    else if (_reducer.reducer_can_combine()
	     || (_tmpfiles.empty() && mode == TO_SINGLE_FILE))
    {
	tmpfile_collector collector
	    (_file_prefix, _tmpfile_id,
             (_tmpfiles.empty() && mode == TO_SINGLE_FILE)
             ? alt_output
	     : nullptr);
	mapredo::valuelist<T> list (queue);

	while (!queue.empty())
        {
            static_cast<mapredo::mapreducer<T>&>(_reducer).reduce
                (list.get_key(), list, collector);
	}
        collector.flush();
	_tmpfiles.emplace_back (collector.filename()); // add merged file
    }
    else // no reduction
    {
	std::ofstream outfile;
	std::ostringstream filename;
	const bool compressed (settings::instance().compressed());

	filename << _file_prefix << _tmpfile_id++;
	if (compressed)
	{
	    filename << ".snappy";
	    _coutbuffer.reset (new char[0x15000]);
	    _buffer_pos = 0;
	}
	outfile.open (filename.str(), std::ofstream::binary);
	if (!outfile)
	{
	    throw std::invalid_argument
		(errno_message("Unable to open " + filename.str()
			       + " for writing", errno));
	}
	_tmpfiles.push_back (filename.str());

	const T* next_key;
	size_t length;
	key_holder<T> keyh;
	auto* proc = queue.top();
	T key (keyh.get_key(*proc));
	queue.pop();

	for(;;)
	{
	    while ((next_key = proc->next_key())
		   && (*proc == key || queue.empty()))
	    {
		auto line = proc->get_next_line (length);
		if (compressed)
		{
		    if (_buffer_pos + length > _buffer_size)
		    {
			_coutbufpos = 0x15000;
			_compressor->compress (_buffer,
					       _buffer_pos,
					       _coutbuffer.get(),
					       _coutbufpos);
			outfile.write (_coutbuffer.get(), _coutbufpos);
			_buffer_pos = 0;
		    }
		    memcpy (_buffer + _buffer_pos, line, length);
		    _buffer_pos += length;
		}
		else outfile.write (line, length);
	    }

	    if (!next_key) // file emptied
	    {
		delete proc;
		if (queue.empty()) break;
		proc = queue.top();
		key = keyh.get_key (*proc);
		queue.pop();
		continue;
	    }

	    auto* nproc = queue.top();

	    if (*nproc == key)
	    {
		queue.pop();
		queue.push (proc);
		proc = nproc;
	    }
	    else
	    {
		int cmp = nproc->compare (*next_key);
		if (cmp < 0)
		{
		    queue.pop();
		    queue.push (proc);
		    proc = nproc;
		}
		key = keyh.get_key (*proc);
	    }
	}

	if (compressed && _buffer_pos > 0)
	{
	    _coutbufpos = 0x15000;
	    _compressor->compress (_buffer,
				   _buffer_pos,
				   _coutbuffer.get(),
				   _coutbufpos);
	    outfile.write (_coutbuffer.get(), _coutbufpos);
	}

	outfile.close();
    }
}
