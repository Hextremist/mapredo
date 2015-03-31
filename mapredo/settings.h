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

#ifndef _HEXTREME_MAPREDO_SETTINGS_H
#define _HEXTREME_MAPREDO_SETTINGS_H

#include "config.h"

#include <cstdint>
#include <string>

/** Global settings for the engine */
class SO_EXPORT settings
{
public:
    /** @returns a singleton settings object */
    static settings& instance();

    /** Parse a size string which may be postfixed with k/M/G/T */
    int64_t parse_size (const std::string& size) const;
    /** @returns true if verbose flag is set */
    bool verbose() const {return _verbose;}
    /** Set verbose flag */
    void set_verbose() {_verbose = true;}
    /** @return true if compress flag is set */
    bool compressed() const {return _compressed;}
    /** Set or unset compress flag */
    void set_compressed (const bool on = true) {_compressed = on;}
    /** @return true if temporary files should be kept */
    bool keep_tmpfiles() const {return _keep_tmpfiles;}
    /** Indicate whether temporary files should be kept or not */
    void set_keep_tmpfiles (const bool on = true) {_keep_tmpfiles = on;}
    /** @return true if output should be fully sorted */
    bool sort_output() const {return _sort_output;}
    /** Set or unset fully sorted output */
    void set_sort_output (const bool on = true) {_sort_output = on;}
    /** @return true if output should be reverse sorted */
    bool reverse_sort() const {return _reverse_sort;}
    /** Set or unset reverse sorted output */
    void set_reverse_sort (const bool on = true) {
	if (on) _sort_output = true;
	_reverse_sort = on;
    }

private:
    settings() = default;

    bool _verbose = false;
    bool _compressed = false;
    bool _keep_tmpfiles = false;
    bool _sort_output = false;
    bool _reverse_sort = false;
};

#endif

