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

#include <sys/stat.h>
#include <unistd.h>
#include <cstring>

#include <stdexcept>

#include "directory.h"
#include "errno_message.h"

static const std::string empty_path_;

directory::directory (const std::string& path) :
    _dirname (path)
{}


void
directory::create (const std::string& path)
{
    if (mkdir (path.c_str(), 0777) < 0)
    {
	throw std::runtime_error
	    (errno_message("Can not create " + path, errno));
    }
}

bool
directory::exists (const std::string& path)
{
    struct stat st;

    return (stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode));
}

bool
directory::remove (const std::string& path,
		   const bool with_files,
		   const bool recursive)
{
    if (!with_files && !recursive)
    {
	if (rmdir(path.c_str()) < 0)
	{
	    if (errno == ENOENT) return false;

	    throw std::runtime_error
		(errno_message("Can not remove " + path, errno));
	}
	return true;
    }

    DIR* dir = opendir (path.c_str());

    if (dir)
    {
	struct dirent* result;
	struct stat st;

	errno = 0;

	while ((result = readdir(dir)))
	{
	    if (strcmp (result->d_name, ".") == 0
		|| strcmp (result->d_name, "..") == 0)
	    {
		continue;
	    }

	    std::string fname (path + "/" + result->d_name);
	    if (lstat(fname.c_str(), &st) < 0)
	    {
		throw std::runtime_error
		    (errno_message("Can not stat " + fname, errno));
	    }
	    if (S_ISDIR(st.st_mode))
	    {
		if (recursive)
		{
		    remove (fname, true);
		    continue;
		}
		else throw std::runtime_error
			 ("Directory " + path + " contains subdirectories"
			  + " and cannot be removed");
	    }

	    if (unlink(fname.c_str()) < 0)
	    {
		throw std::runtime_error
		    (errno_message("Can not remove " + fname, errno));
	    }
	}
	if (errno != 0)
	{
	    closedir (dir);
	    throw std::runtime_error
		(errno_message("Can not read directory " + path, errno));
	}
	closedir (dir);
	if (rmdir(path.c_str()) < 0)
	{
	    throw std::runtime_error
		(errno_message("Can not remove " + path, errno));
	}
	return true;
    }
    return false;
}

directory::const_iterator::const_iterator (const std::string& path) :
    _path (path),
    _dir (opendir(path.c_str()))
{
    if (!_dir)
    {
	throw std::runtime_error
	    (errno_message("Can not open " + path, errno));
    }
    get_next_file();
}


directory::const_iterator::const_iterator() : _path(empty_path_), _dir(nullptr)
{}

directory::const_iterator::~const_iterator()
{
    if (_dir) closedir (_dir);
}

const char*
directory::const_iterator::operator*()
{
    if (_result) return _result->d_name;
    return "";
}

const directory::const_iterator&
directory::const_iterator::operator++()
{
    get_next_file();
    return *this;
}

bool
directory::const_iterator::operator!=(const const_iterator& other) const
{
    if (!_result) return (other._result);
    if (!other._result) return true;
    return _result != other._result;
}

bool
directory::const_iterator::get_next_file()
{
    do
    {
	_result = readdir(_dir);
    }
    while (_result && _result->d_name[0] == '.');
    return _result;

    //throw std::runtime_error (errno_message("Can not access " + _path, errno));
}
