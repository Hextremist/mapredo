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

#ifndef _MAPREDO_ERRNO_EXCEPTION_H
#define _MAPREDO_ERRNO_EXCEPTION_H

#include "config.h"

#include <stdexcept>

class SO_EXPORT errno_exception : public std::runtime_error
{
    /**
     * @param message text explaining what failed
     * @param error errno code which will converted to text and appended
     *        to message
     */
    errno_exception (const std::string& message, const int error) {
#ifdef HAVE_GNU_STRERROR_R
#elif HAVE_STRERROR_R
#elif HAVE_STRERROR_S
	
#else
	
#endif
    }
};

#endif
