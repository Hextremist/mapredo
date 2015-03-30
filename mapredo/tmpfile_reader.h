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

#ifndef _HEXTREME_MAPREDO_TMPFILE_READER_H
#define _HEXTREME_MAPREDO_TMPFILE_READER_H

#include <cstring>
#include <sstream>
#include <memory>
#include <algorithm>

#include "data_reader.h"
#include "compression.h"

/**
 * Used to read a temporary file while merge sorting
 */
template <class T>
class tmpfile_reader : public data_reader<T>
{
public:
    /**
     * @param filename the name of the temporary file
     * @param buffer_size size of the buffer to read data into
     * @param delete_file_after set true if the file can be deleted
     */
    tmpfile_reader (const std::string& filename,
		    const int buffer_size,
		    const bool delete_file_after);
    //tmpfile_reader (tmpfile_reader&& other)
    //: _file(std::move(other._file)), _pos(other._pos);
    ~tmpfile_reader() {
	if (_cbuffer) delete[] _cbuffer;
	if (_fp) fclose(_fp);
	if (_delete_file_after) remove (_filename.c_str());
    }

    /** @returns the name of the temporary file */
    const std::string& filename() const {return _filename;}

    tmpfile_reader (const tmpfile_reader&) = delete;
    tmpfile_reader& operator=(const tmpfile_reader&) = delete;

private:
    bool read_more();

    FILE* _fp = 0;
    std::string _filename;
    size_t _buffer_size;
    size_t _cstart_pos = 0;
    size_t _cend_pos = 0;
    size_t _cbuffer_size;
    char* _cbuffer = nullptr;
    size_t _bytes_left_file;
    bool _delete_file_after;
    std::unique_ptr<compression> _compressor;
};

template <class T>
tmpfile_reader<T>::tmpfile_reader (const std::string& filename,
				   const int buffer_size,
				   const bool delete_file_after) :
    _filename (filename),
    _buffer_size (buffer_size - 1),
    _cbuffer_size (buffer_size),
    _delete_file_after (delete_file_after)
{
    if (filename.size() > 7
	&& filename.substr(filename.size() - 7) == ".snappy")
    {
	_compressor.reset (new compression());
    }

    if (_compressor && buffer_size < 0x20000)
    {
	throw std::runtime_error ("Temporary file reader needs at least 128k"
				  " buffer for compressed input");
    }

    _fp = fopen (filename.c_str(), "rb");
    if (!_fp)
    {
	char err[80];

	strerror_r (errno, err, sizeof(err));
	throw std::invalid_argument ("Unable to open \"" + filename
				     + "\" for reading: " + err);
    }
    
    fseek (_fp, 0, SEEK_END);
    _bytes_left_file = ftell(_fp);
    fseek (_fp, 0, SEEK_SET);

    if (_compressor)
    {
	_buffer_size = 0x20000;
	this->_buffer = new char[0x20001];
	this->_buffer[0x20000] = '\0';
	_cbuffer = new char[buffer_size];
    }
    else
    {
	this->_buffer = new char[buffer_size];
	this->_buffer[buffer_size - 1] = '\0';
    }

    this->fill_next_line();
}

template <class T> bool
tmpfile_reader<T>::read_more()
{
    if (_bytes_left_file == 0 && _cstart_pos == _cend_pos) return false;

#if 0
    std::cerr << "Reading more: " << _bytes_left_file << " bytes left"
	      << " start " << this->_start_pos << " end " << this->_end_pos
	      << " cstart "<< _cstart_pos << " cend " << _cend_pos
	      << "\n";
#endif

    if (this->_start_pos != this->_end_pos)
    {
	if (this->_start_pos != 0)
	{
	    this->_end_pos -= this->_start_pos;
	    memmove (this->_buffer,
		     this->_buffer + this->_start_pos,
		     this->_end_pos);
	    this->_start_pos = 0;
	}
    }
    else this->_start_pos = this->_end_pos = 0;

    if (_compressor)
    {
	// Look for data in cbuffer first
	size_t insize = _cend_pos - _cstart_pos;
	size_t outsize = _buffer_size - this->_end_pos;

	if (insize > 0 &&_compressor->inflate (_cbuffer + _cstart_pos, insize,
					       this->_buffer + this->_end_pos,
					       outsize))
	{
	    _cstart_pos += insize;
	    this->_end_pos += outsize;
	}
	else
	{
	    if (_cstart_pos != _cend_pos)
	    {
		if (_cstart_pos != 0)
		{
		    _cend_pos -= _cstart_pos;
		    memmove (_cbuffer, _cbuffer + _cstart_pos, _cend_pos);
		    _cstart_pos = 0;
		}
	    }
	    else _cstart_pos = _cend_pos = 0;

	    size_t bytes_to_read = std::min<size_t> (_bytes_left_file,
						     _cbuffer_size - _cend_pos);
	    if (!fread(_cbuffer + _cend_pos, bytes_to_read, 1, _fp))
	    {
		throw std::runtime_error
		    ("Can not read compressed data from temporary file");
	    }
	    _bytes_left_file -= bytes_to_read;
	    _cend_pos += bytes_to_read;
	    insize = _cend_pos - _cstart_pos;
	    if (_compressor->inflate (_cbuffer, insize,
				      this->_buffer + this->_end_pos, outsize))
	    {
		_cstart_pos += insize;
		this->_end_pos += outsize;
	    }
	    else
	    {
		throw std::runtime_error
		    ("Can not uncompress data from temporary file");
	    }
	}
	//std::cerr << '|' << std::string(this->_buffer, this->_end_pos) << '|'
	//	  << "\n";
	return true;
    }

    size_t bytes_to_read = std::min<size_t> (_bytes_left_file,
					     _buffer_size - this->_end_pos);

    //std::cerr << "Have " << _end_pos << " bytes, reading " << bytes_to_read
    //<< " more\n";
    if (!fread (this->_buffer + this->_end_pos, bytes_to_read, 1, _fp))
    {
	throw std::runtime_error ("Can not read from temporary file");
    }
    this->_end_pos += bytes_to_read;
    _bytes_left_file -= bytes_to_read;

    return true;
}

#endif
