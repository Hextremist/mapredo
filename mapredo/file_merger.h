/* -*- C++ -*-
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

#ifndef _HEXTREME_MAPREDO_FILE_MERGER_H
#define _HEXTREME_MAPREDO_FILE_MERGER_H

#include "config.h"

#include <string>
#include <list>

#include "rcollector.h"
#include "tmpfile_reader.h"
#include "settings.h"
#include "valuelist.h"
#include "mapreducer.h"
#include "tmpfile_collector.h"
#include "data_reader_queue.h"

namespace mapredo
{
    class base;
}

/**
 * Does merging of temporary data from different files in the sorting phase
 */
class file_merger : public mapredo::rcollector
{
public:
    file_merger (mapredo::base& reducer,
		 std::list<std::string>&& tmpfiles,
		 const std::string& tmpdir,
		 const size_t index,
		 const size_t max_open_files);
    virtual ~file_merger();

    /**
     * Go through all sorted temporary files and generate a single sorted
     * stream.
     */
    void merge();

    /**
     * Merge to a single file and return the file name.  This may also
     * output final data to an alternate sink.
     * @param alt_output if not nullptr, attempt to write to this first
     */
    std::string merge_to_file (prefered_output* alt_output);

    /**
     * Merge to at most max_open_files file and return the file names.
     */
    std::list<std::string> merge_to_files();

    /** Function called by reducer to report output. */
    void collect (const char* line, const size_t length);

    /** Reserve memory buffer for reducer */
    virtual char* reserve (const size_t bytes) final;

    /** Collect data from reserved memory */
    virtual void collect_reserved (const size_t length = 0) final;

    /**
     * @returns a thread exception if it occured or nullptr otherwise
     */
    std::exception_ptr& exception_ptr() {return _texception;}

    file_merger (file_merger&& other);
    file_merger (const file_merger&) = delete;

private:
    enum merge_mode
    {
	TO_MAX_FILES,
	TO_SINGLE_FILE,
	TO_OUTPUT
    };

    template <class T> class key_holder {
    public:
	template<class U = T,
		 typename std::enable_if<std::is_fundamental<U>::value>
                 ::type* = nullptr>
	U get_key (data_reader<T>& reader) {
	    auto key = reader.next_key();
	    if (key) return *key;
	    throw std::runtime_error
		("Attempted to key_handler::get_key() on an empty file");
	}

	template<class U = T,
                 typename std::enable_if<std::is_same<U,char*>::value,
                                         bool>::type* = nullptr>
	char* get_key (data_reader<T>& reader) {
	    auto key = reader.next_key();
	    if (key)
	    {
		_key_copy = *key;
		return const_cast<char*>(_key_copy.c_str());
	    }
	    throw std::runtime_error
		("Attempted to key_handler::get_key() on an empty file");
	}
    private:
	std::string _key_copy;
    };
    
    void merge_max_files (const merge_mode mode,
			  prefered_output* alt_output = nullptr);
    void compressed_sort();
    void regular_sort();
    void flush() {
	if (_buffer_pos > 0)
	{
	    fwrite (_buffer, _buffer_pos, 1, stdout);
	    _buffer_pos = 0;
	}
    }
    template<typename T> void do_merge (const merge_mode mode,
					prefered_output* alt_output,
					const bool reverse);

    mapredo::base& _reducer;
    static const size_t _buffer_size = 0x10000;
    size_t _max_open_files;
    size_t _num_merged_files = 0;
    std::string _file_prefix;
    int _tmpfile_id = 0;
    std::list<std::string> _tmpfiles;
    std::unique_ptr<compression> _compressor;
    char _buffer[_buffer_size];
    std::unique_ptr<char[]> _coutbuffer;
    size_t _buffer_pos = 0;
    size_t _coutbufpos;
    size_t _reserved_bytes = 0;
    std::exception_ptr _texception = nullptr;
};

#endif
