
#include <unistd.h>
#include <cctype>
#include <iostream>
#include <future>
#include <stdexcept>

#include "engine.h"
#include "collector.h"
#include "directory.h"
#include "settings.h"

engine::engine (const std::string& tmpdir,
		const std::string& subdir,
		const int parallel,
		const int bytes_buffer,
		const int max_open_files)
    : _tmpdir (subdir.empty() ? tmpdir : (tmpdir + "/" + subdir)),
      _is_subdir (subdir.size()),
      _parallel (parallel),
      _bytes_buffer (bytes_buffer),
      _max_files (max_open_files)
{
    if (access(tmpdir.c_str(), R_OK|W_OK|X_OK) != 0)
    {
	throw std::runtime_error (tmpdir + " needs to be a writable directory");
    }
}

engine::~engine()
{}

void
engine::enable_sorters (const mapredo::base::keytype type, const bool reverse)
{
    if (_is_subdir)
    {
	if (!directory::exists(_tmpdir)) directory::create (_tmpdir);
    }    

    for (size_t i = 0; i < _parallel; i++)
    {
	_sorters.push_back (sorter(_tmpdir, i, _bytes_buffer/_parallel,
				   type, reverse));
    }
    _unprepared = false;
}

void
engine::flush (mapredo::base* mapreducer)
{
    for (auto& sorter: _sorters) sorter.flush();

    if (!mapreducer)
    {
	for (auto& sorter: _sorters)
	{
	    sorter.wait_flushed();
	    sorter.grab_tmpfiles();
	}
	return;
    }

    for (auto& sorter: _sorters)
    {
	sorter.wait_flushed();
	std::list<std::string> tmpfiles (sorter.grab_tmpfiles());

	if (tmpfiles.size() == 1)
	{
	    _files_final_merge.push_back (tmpfiles.front());
	}
	else if (tmpfiles.size())
	{
	    _mergers.push_back
		(file_merger(*mapreducer,
			     static_cast<std::list<std::string>&&>(tmpfiles),
			     _tmpdir, _unique_id++, 0x20000,
			     _max_files/_parallel));
	}
    }
    _sorters.clear();

    merge (*mapreducer);
}

void
engine::reduce (mapredo::base& mapreducer)
{
    if (!_is_subdir)
    {
	throw std::logic_error
	    ("Subdir need to be set for reduce only processing");
    }

    try
    {
	std::vector<std::list<std::string>> lists;
	directory dir (_tmpdir);

	lists.resize (_parallel);

	for (const auto& file: dir)
	{
	    const char *period = strchr (file, '.');

	    if (!period || !isdigit(period[1]))
	    {
		throw std::runtime_error (std::string("Invalid tmpfile name ")
					  + file);
	    }
	    lists[atoi(period+1)%_parallel].push_back (_tmpdir + "/" + file);
	}

	for (auto& tmpfiles: lists)
	{
	    if (tmpfiles.size() == 1)
	    {
		_files_final_merge.push_back (tmpfiles.front());
	    }
	    else if (tmpfiles.size())
	    {
		_mergers.push_back
		    (file_merger
		     (mapreducer,
		      static_cast<std::list<std::string>&&>(tmpfiles),
		      _tmpdir, _unique_id++, 0x20000, _max_files/_parallel));
	    }
	}

	merge (mapreducer);
    }
    catch (...)
    {
	if (!settings::instance().keep_tmpfiles())
	{
	    directory::remove(_tmpdir, true);
	}
	throw;
    }

    if (!settings::instance().keep_tmpfiles())
    {
	directory::remove(_tmpdir);
    }
}

void
engine::merge (mapredo::base& mapreducer)
{
    std::vector<std::future<std::string>> results;
    results.resize (_mergers.size());
    auto iter = _mergers.begin();
    auto riter = results.begin();

    if (_files_final_merge.empty())
    {
	if (_mergers.empty()) return;
	if (_mergers.size() == 1)
	{
	    iter->merge();
	    return;
	}
    }

    for (; iter != _mergers.end(); iter++, riter++)
    {
	auto& merger (*iter);

	*riter = std::async (std::launch::async,
			     &file_merger::merge_to_file,
			     &merger);
    }

    for (iter = _mergers.begin(), riter = results.begin();
	 iter != _mergers.end(); iter++, riter++)
    {
	if (iter->exception_ptr())
	{
	    std::rethrow_exception(iter->exception_ptr());
	}
	_files_final_merge.push_back (riter->get());
    }
    _mergers.clear();

    file_merger merger
	(mapreducer,
	 static_cast<std::list<std::string>&&>(_files_final_merge),
	 _tmpdir, _unique_id, 0x10000, _max_files);
    merger.merge();

    _files_final_merge.clear();
}

void
engine::collect (const char* inbuffer, const int insize)
{
    if (_unprepared)
    {
	throw std::runtime_error ("engine::enable_sorters() needs to be called"
				  " before any mapping is done");
    }

    if (_parallel > 1)
    {
	int i;
	int value = 0;
	
	// Very simple hash function here
	for (i = 0; i < insize && inbuffer[i] != '\t'; i++)
	{
	    value += i * inbuffer[i];
	}

	_sorters[value % _parallel].add (inbuffer, insize);
    }
    else _sorters[0].add (inbuffer, insize);
}
