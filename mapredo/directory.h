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

#ifndef _HEXTREME_MAPREDO_DIRECTORY_H
#define _HEXTREME_MAPREDO_DIRECTORY_H

#include "config.h"

#include <sys/types.h>
#include <dirent.h>

/**
 * Directory handling code, wraps plain C
 */
class SO_EXPORT directory
{
public:
    class const_iterator
    {
    public:
        /** @param path the path to the examined directory */
	const_iterator (const std::string& path);
	const_iterator();
	~const_iterator();
        /** Move iterator to next file */
	const const_iterator& operator++();
        /** Compare two iterators */
	bool operator!=(const const_iterator& other) const;
        /** @returns relative name of sub directory */
	const char* operator*();
    private:
	bool get_next_file();
	const std::string& _path;
	DIR* _dir;
	struct dirent _entry;
	struct dirent* _result = nullptr;
    };

    /**
     * Open a directory for scanning
     * @param path the path to the directory examined
     */
    directory (const std::string& path);
    virtual ~directory() {}

    /** Iterator to first file in directory */
    const_iterator begin() const {return const_iterator (_dirname);}
    /** Iterator to past end of last file in directory */
    const const_iterator& end() const {return _end;}

    /**
     * Create a directory
     */
    static void create (const std::string& path);
    /**
     * Check whether a path exists and is a directory
     */
    static bool exists (const std::string& path);
    /**
     * Delete a directory.  If the directory exists, but can not be deleted,
     * an exception is thrown.
     * @param path directory to delete.
     * @param with_files delete files contained in the directory.
     * @param recursive delete files and directories recursively.
     * @returns true if the directory was deleted, false if it did not exist.
     */
    static bool remove (const std::string& path,
			const bool with_files = false,
			const bool recursive = false);

 private:
    std::string _dirname;
    const_iterator _end;
};

#endif
