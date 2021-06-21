
#include <algorithm>
#include <functional>
#include <cctype>
#include <locale>
#include <new>
#include <regex>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <string>
#include <cstring>      // Needed for memset
#include <netinet/in.h>
#include <unistd.h>    //write
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>

#include "Error.h"
#include "Connection.h"
#include "ChunkReader.h"

static const char __file__[] = __FILE__;

using namespace std;

namespace cserve {

    ChunkReader::ChunkReader(std::istream *ins_p, size_t post_maxsize_p) : ins(ins_p), post_maxsize(post_maxsize_p) {
        chunk_size = 0;
        chunk_pos = 0;
    };
    //=========================================================================

    size_t ChunkReader::read_chunk(istream &ins, char **buf, size_t offs) {
        string line;
        (void) safeGetline(ins, line, max_headerline_len); // read chunk size

        if (ins.fail() || ins.eof()) {
            throw INPUT_READ_FAIL;
        }

        size_t n;

        try {
            n = stoul(line, 0, 16);
        } catch (const std::invalid_argument &ia) {
            throw Error(__file__, __LINE__, ia.what());
        }

        //
        // Check for post_maxsize
        //
        if ((post_maxsize > 0) && (n > post_maxsize)) {
            stringstream ss;
            ss << "Chunksize (" << n << ") to big (maxsize=" << post_maxsize << ")";
            throw Error(__file__, __LINE__, ss.str());
        }

        if (n == 0) return 0;

        if (*buf == nullptr) {
            if ((*buf = (char *) malloc((n + 1) * sizeof(char))) == nullptr) {
                throw Error(__file__, __LINE__, "malloc failed", errno);
            }
        } else {
            if ((*buf = (char *) realloc(*buf, (offs + n + 1) * sizeof(char))) == nullptr) {
                throw Error(__file__, __LINE__, "realloc failed", errno);
            }
        }

        ins.read(*buf + offs, n);
        if (ins.fail() || ins.eof()) {
            throw -1;
        }

        (*buf)[offs + n] = '\0';
        (void) safeGetline(ins, line, max_headerline_len); // read "\r\n" at end of chunk...

        if (ins.fail() || ins.eof()) {
            throw INPUT_READ_FAIL;
        }

        return n;
    }
    //=========================================================================


    size_t ChunkReader::readAll(char **buf) {
        size_t n, nbytes = 0;
        *buf = nullptr;

        while ((n = read_chunk(*ins, buf, nbytes)) > 0) {
            nbytes += n;
            //
            // check for post_maxsize
            //
            if ((post_maxsize > 0) && (nbytes > post_maxsize)) {
                stringstream ss;
                ss << "Chunksize (" << nbytes << ") to big (maxsize=" << post_maxsize << ")";
                throw Error(__file__, __LINE__, ss.str());
            }
        }

        return nbytes;
    }
    //=========================================================================

    size_t ChunkReader::getline(std::string &t) {
        t.clear();

        size_t n = 0;
        while (true) {
            if (chunk_pos >= chunk_size) {
                string line;

                (void) safeGetline(*ins, line, max_headerline_len); // read chunk size
                if (ins->fail() || ins->eof()) {
                    throw INPUT_READ_FAIL;
                }

                try {
                    chunk_size = stoul(line, 0, 16);
                } catch (const std::invalid_argument &ia) {
                    throw Error(__file__, __LINE__, ia.what());
                }

                //
                // check for post_maxsize
                //
                if ((post_maxsize > 0) && (chunk_size > post_maxsize)) {
                    stringstream ss;
                    ss << "Chunksize (" << chunk_size << ") to big (maxsize=" << post_maxsize << ")";
                    throw Error(__file__, __LINE__, ss.str());
                }

                if (chunk_size == 0) {
                    (void) safeGetline(*ins, line, max_headerline_len); // get last "\r\n"....
                    if (ins->fail() || ins->eof()) {
                        throw INPUT_READ_FAIL;
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
                    throw INPUT_READ_FAIL;
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
                                throw INPUT_READ_FAIL;
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

    int ChunkReader::getc(void) {
        if (chunk_pos >= chunk_size) {
            string line;
            (void) safeGetline(*ins, line, max_headerline_len); // read the size of the new chunk

            if (ins->fail() || ins->eof()) {
                throw INPUT_READ_FAIL;
            }

            try {
                chunk_size = stoul(line, 0, 16);
            } catch (const std::invalid_argument &ia) {
                throw Error(__file__, __LINE__, ia.what() + line);
            }

            //
            // check for post_maxsize
            //
            if ((post_maxsize > 0) && (chunk_size > post_maxsize)) {
                stringstream ss;
                ss << "Chunksize (" << chunk_size << ") to big (maxsize=" << post_maxsize << ")";
                throw Error(__file__, __LINE__, ss.str());
            }

            if (chunk_size == 0) {
                (void) safeGetline(*ins, line, max_headerline_len); // get last "\r\n"....
                if (ins->fail() || ins->eof()) {
                    throw INPUT_READ_FAIL;
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
                throw INPUT_READ_FAIL;
            }
        }
        return c;
    }
    //=========================================================================


}
