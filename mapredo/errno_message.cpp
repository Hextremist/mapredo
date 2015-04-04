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

#include "config.h"

#include <cstring>
#include <string>

#include "errno_message.h"

std::string errno_message (const std::string& message, const int error)
{
    char err[80];

#ifdef HAVE_GNU_STRERROR_R
    return (message + ": " + strerror_r(error, err, sizeof(err)));
#elif HAVE_STRERROR_R
    strerror_r (error, err, sizeof(err));
#elif HAVE_STRERROR_S
    strerror_s (err, sizeof(err), error);
#else
    strerror (error, err, sizeof(err));
#endif
    return (message + ": " + err);
}
