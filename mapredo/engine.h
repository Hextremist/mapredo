/*** -*- C++ -*- *********************************************************/

#ifndef _HEXTREME_MAPREDO_ENGINE_H
#define _HEXTREME_MAPREDO_ENGINE_H

#include <memory>
#include <deque>
#include <list>

#include "collector.h"
#include "sorter.h"
#include "file_merger.h"
#include "base.h"
#include "plugin_loader.h"
#include "buffer_trader.h"
#include "consumer.h"

class buffer_trader;

/**
 * Runs overall map-reduce algorithm
 */
class engine
{
public:
    /**
     * @param loader plugin loader factory for creation of extra mapreducers
     * @param tmpdir temporary directory
     * @param subdirectory under temporary directory, may be empty
     * @param parallel the number of worker threads
     * @param bytes_buffer number of bytes in each sort buffer, must be at
     *        least as high as parallel.
     * @param max_open_files the maximum number of files open while merging
     */
    engine (const std::string& plugin,
	    const std::string& tmpdir,
	    const std::string& subdir,
	    const size_t parallel,
	    const size_t bytes_buffer,
	    const int max_open_files);
    virtual ~engine();

    /**
     * Set up consumer objects for mapping and sorting of input data
     * @returns empty buffer which can be filled with data before calling
     *          provide_input_data() or complete_input().
     */
    input_buffer* prepare_input();

    /**
     * Provide data to sorters.
     * @param data input data to be provided.
     * @returns buffer with room for more data.
     */
    input_buffer* provide_input_data (input_buffer* data);

    /**
     * Wait until input phase is completed.
     * @param data the last input_buffer returned by
     *             provide_input_data(), or prepare_input() if
     *             provide_input_data() was never called.
     */
    void complete_input (input_buffer* data);

    /**
     * Go through all sorted temporary files and generate a reduced file.
     */
    void reduce();

    /**
     * Reduce existing files only.  Reduction is normally done inside
     * reduce(), but this can be used to reduce existing files in
     * a sub-directory.
     */
    void reduce_existing_files();

private:
    void merge_grouped (mapredo::base& mapreducer);
    void merge_sorted (mapredo::base& mapreducer);
    void output_final_files();

    plugin_loader _plugin_loader;
    input_buffer* _buffers[2];
    const std::string _tmpdir;
    bool _is_subdir = false;
    size_t _parallel;
    size_t _bytes_buffer;
    int _max_files;
    size_t _unique_id = 0;
    enum stage {
	UNPREPARED,
	PREPARED,
	ACTIVE
    };
    stage _sorting_stage = UNPREPARED;

    std::list<consumer> _consumers;
    buffer_trader _buffer_trader;

    std::deque<file_merger> _mergers;
    std::list<std::string> _files_final_merge;
};

#endif
