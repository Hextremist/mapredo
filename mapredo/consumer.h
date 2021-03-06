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

#ifndef _HEXTREME_MAPREDO_CONSUMER_H
#define _HEXTREME_MAPREDO_CONSUMER_H

#include <unordered_map>
#include <list>
#include <thread>
#include <vector>

#include "mcollector.h"

class plugin_loader;
class mapreducer;
class buffer_trader;
class sorter;
namespace mapredo
{
  class base;
}

/**
 * Class used to run map and sort
 */
class consumer : public mapredo::mcollector
{
public:
    /**
     * Create object and start thread waiting for 
     * @param mapreducer map-reducer plugin object.
     * @param tmpdir root of temporary directory.
     * @param is_subdir true if the directory is a specified subdirectory.
     * @param type type to use for sorting.
     * @param reverse if true, sort in descending order instead of ascending.
     */
    consumer (mapredo::base& mapred,
	      const std::string& tmpdir,
	      const bool is_subdir,
	      const uint16_t buckets,
	      const uint16_t worker_id, 
	      const size_t bytes_buffer,
	      const bool reverse);
    virtual ~consumer();

    /**
     * Process input data in a separate thread.
     * @param trader object to pull jobs from.
     */
    void start_thread (buffer_trader& trader);

    /**
     * Wait for input data processing thread to finish.
     */
    void join_thread();

    /** Append all temporary files of a given index to a list of files */
    void append_tmpfiles (const size_t index, std::list<std::string>& files);

    /** Used to collect data, called from the mapper */
    virtual void collect (const char* line, const size_t length) final;

    /** Reserve memory buffer to store the collected value to. */
    virtual char* reserve (const char* const key, const size_t bytes) final;

    /** Collect from the reserved memory buffer returned from reserve(). */
    virtual void collect_reserved (const size_t length = 0) final;

    /**
     * @returns a thread exception if it occured or nullptr otherwise.
     *          Do not call this until the thread has been joined.
     */
    std::exception_ptr& exception_ptr() {return _texception;}


    consumer(consumer&&) = delete;
    consumer& operator=(const consumer&) = delete;
    
private:
    void work (buffer_trader& trader);

    std::thread _thread;
    mapredo::base& _mapreducer;
    const std::string _tmpdir;
    const bool _is_subdir = false;
    const size_t _buckets;
    const size_t _worker_id;
    std::exception_ptr _texception = nullptr;

    std::vector<sorter> _sorters;
    std::unordered_map<int, std::list<std::string>> _tmpfiles;

    size_t _reserved_bucket;
    size_t _reserved_keylen;
    size_t _reserved_valuelen;
};

#endif
