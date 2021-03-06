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

#include <functional>
#include <locale>
#include <new>
#include <regex>
#include <iomanip>
#include <sstream>
#include <string>
#include <cstdio>
#include <cstdlib>


#include "Error.h"
#include "Connection.h"
#include "ChunkReader.h"

static const char file_[] = __FILE__;

using namespace std;

namespace cserve {

    ChunkReader::ChunkReader(std::istream *ins_p, size_t post_maxsize_p) : ins(ins_p), post_maxsize(post_maxsize_p) {
        chunk_size = 0;
        chunk_pos = 0;
    }
    //=========================================================================

    std::streamsize ChunkReader::read_chunk(istream &is, char **buf, size_t offs) const {
        string line;
        (void) safeGetline(is, line, max_headerline_len); // read chunk size

        if (is.fail() || is.eof()) {
            throw InputFailure(INPUT_READ_FAIL);
        }

        std::streamsize n;

        try {
            n = std::stol(line, nullptr, 16);
        } catch (const std::invalid_argument &ia) {
            throw Error(file_, __LINE__, ia.what());
        }

        //
        // Check for post_maxsize
        //
        if ((post_maxsize > 0) && (n > post_maxsize)) {
            stringstream ss;
            ss << "Chunksize (" << n << ") to big (maxsize=" << post_maxsize << ")";
            throw Error(file_, __LINE__, ss.str());
        }

        if (n == 0) return 0;

        if (*buf == nullptr) {
            if ((*buf = (char *) malloc((n + 1) * sizeof(char))) == nullptr) {
                throw Error(file_, __LINE__, "malloc failed", errno);
            }
        } else {
            if ((*buf = (char *) realloc(*buf, (offs + n + 1) * sizeof(char))) == nullptr) {
                throw Error(file_, __LINE__, "realloc failed", errno);
            }
        }

        is.read(*buf + offs, n);
        if (is.fail() || is.eof()) {
            throw InputFailure(INPUT_READ_FAIL);
        }

        (*buf)[offs + n] = '\0';
        (void) safeGetline(is, line, max_headerline_len); // read "\r\n" at end of chunk...

        if (is.fail() || is.eof()) {
            throw InputFailure(INPUT_READ_FAIL);
        }

        return n;
    }
    //=========================================================================


std::streamsize ChunkReader::readAll(char **buf) {
    std::streamsize n, nbytes = 0;
        *buf = nullptr;

        while ((n = read_chunk(*ins, buf, nbytes)) > 0) {
            nbytes += n;
            //
            // check for post_maxsize
            //
            if ((post_maxsize > 0) && (nbytes > post_maxsize)) {
                stringstream ss;
                ss << "Chunksize (" << nbytes << ") to big (maxsize=" << post_maxsize << ")";
                throw Error(file_, __LINE__, ss.str());
            }
        }

        return nbytes;
    }
    //=========================================================================

std::streamsize ChunkReader::getline(std::string &t) {
    t.clear();

    streamsize n = 0;
    while (true) {
        if (chunk_pos >= chunk_size) {
            string line;

            (void) safeGetline(*ins, line, max_headerline_len); // read chunk size
            if (ins->fail() || ins->eof()) {
                throw InputFailure(INPUT_READ_FAIL);
            }

            try {
                chunk_size = stoul(line, nullptr, 16);
            } catch (const std::invalid_argument &ia) {
                throw Error(file_, __LINE__, ia.what());
            }

            //
            // check for post_maxsize
            //
            if ((post_maxsize > 0) && (chunk_size > post_maxsize)) {
                stringstream ss;
                ss << "Chunksize (" << chunk_size << ") to big (maxsize=" << post_maxsize << ")";
                throw Error(file_, __LINE__, ss.str());
            }

            if (chunk_size == 0) {
                (void) safeGetline(*ins, line, max_headerline_len); // get last "\r\n"....
                if (ins->fail() || ins->eof()) {
                    throw InputFailure(INPUT_READ_FAIL);
                }
                return n;
            }
            chunk_pos = 0;
        }

        std::istream::sentry se(*ins, true);
        std::streambuf *sb = ins->rdbuf();
        int c = sb->sbumpc();
        chunk_pos++;

        if (chunk_pos >= chunk_size) {
            string line;
            (void) safeGetline(*ins, line, max_headerline_len); // read "\r\n" at end of  chunk...
            if (ins->fail() || ins->eof()) {
                throw InputFailure(INPUT_READ_FAIL);
            }
        }

        n++;

        switch (c) {
            case '\n':
                return n;
            case '\r':
                if (sb->sgetc() == '\n') {
                    sb->sbumpc();
                    chunk_pos++;
                    if (chunk_pos >= chunk_size) {
                        string line;
                        (void) safeGetline(*ins, line, max_headerline_len); // read "\r\n" at end of  chunk...
                        if (ins->fail() || ins->eof()) {
                            throw InputFailure(INPUT_READ_FAIL);
                        }
                    }
                    n++;
                }
                return n;
            case EOF:
                // Also handle the case when the last line has no line ending
                if (t.empty())
                    ins->setstate(std::ios::eofbit);
                return n;
            default:
                t += (char) c;
        }
    }
}
//=========================================================================

    int ChunkReader::getc() {
        if (chunk_pos >= chunk_size) {
            string line;
            (void) safeGetline(*ins, line, max_headerline_len); // read the size of the new chunk

            if (ins->fail() || ins->eof()) {
                throw InputFailure(INPUT_READ_FAIL);
            }

            try {
                chunk_size = stoul(line, nullptr, 16);
            } catch (const std::invalid_argument &ia) {
                throw Error(file_, __LINE__, ia.what() + line);
            }

            //
            // check for post_maxsize
            //
            if ((post_maxsize > 0) && (chunk_size > post_maxsize)) {
                stringstream ss;
                ss << "Chunksize (" << chunk_size << ") to big (maxsize=" << post_maxsize << ")";
                throw Error(file_, __LINE__, ss.str());
            }

            if (chunk_size == 0) {
                (void) safeGetline(*ins, line, max_headerline_len); // get last "\r\n"....
                if (ins->fail() || ins->eof()) {
                    throw InputFailure(INPUT_READ_FAIL);
                }
                return EOF;
            }
            chunk_pos = 0;
        }

        std::istream::sentry se(*ins, true);
        std::streambuf *sb = ins->rdbuf();
        int c = sb->sbumpc();
        chunk_pos++;

        if (chunk_pos >= chunk_size) {
            string line;
            (void) safeGetline(*ins, line, max_headerline_len); // read "\r\n" at end of chunk...
            if (ins->fail() || ins->eof()) {
                throw InputFailure(INPUT_READ_FAIL);
            }
        }
        return c;
    }
    //=========================================================================


}
