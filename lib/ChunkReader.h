/*
 * Copyright © 2022 Lukas Rosenthaler
 * This file is part of OMAS/cserve
 * OMAS/cserve is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * OMAS/cserve is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
/*!
 * \brief Implements reading of chunks
 *
 */
#ifndef __cserve_chunk_reader_h
#define __cserve_chunk_reader_h

#include <iostream>
#include <vector>

#include "Error.h"

namespace cserve {

    /*!
     * The class ChunkReader handles reading from HTTP-connections that used the
     * chunked transfer method. This method is used whenever the size of the data
     * to be transferred is known when sending the data starts. A chunk consists of
     * a header line containing the number of bytes in the chunk as hexadezimal value
     * followed by the data and an empty line. The end of the transfer is indicated
     * by an empty chunk.
     */
    class ChunkReader {
    private:
        std::istream *ins;
        size_t chunk_size;
        size_t chunk_pos;
        size_t post_maxsize;

        std::streamsize read_chunk(std::istream &is, char **buf, size_t offs = 0) const;

    public:
        /*!
         * Constructor for class used for reading chunks from a HTTP connection that is chunked
         *
         * \param[in] ins_p Input stream (e.g. socket stream of HTTP connection)
         * \param[in] maxisze_p Maximal total size the chunk reader is allowed to read.
         *            If the chunk or total data is bigger, an cserve::Error is thrown!
         *            IF post_maxsize_p is 0 (default), there is no limit.
         */
        explicit ChunkReader(std::istream *ins_p, size_t post_maxsize_p = 0);

        /*!
         * Read all chunks and return the data in the buffer
         *
         * This method reads all data from all chunks and returns it in the
         * pointer given. The data is allocated using malloc and realloc.
         * Important: The caller
         * is responsible for freeing the memory using free()!
         *
         * \param buf Address of a pointer to char. The method allocates the memory
         * using malloc and realloc and returns the pointer to the data. The caller is
         * responsible of freeing the memry using free()!
         *
         * \returns The number of bytes that have been read (length of buf)
         */
        std::streamsize readAll(char **buf);

        /*!
         * Get the next (text-)line from the chunk stream, even if the line spans the chunk
         * boundary of two successive chunks
         *
         * \param[out] String with the textline
         *
         * \return Number of bytes read (that is the length of the line...)
         */
        std::streamsize getline(std::string &t);

        /*!
         * Get the next byte in a chunked stream
         *
         * \returns The next byte or EOF, if the end of the HTTP data is reached
         */
        int getc();
    };

}

#endif
