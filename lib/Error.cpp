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
#include <cstring>      // std::strerror
#include <sstream>      // std::ostringstream

#include "Error.h"

namespace cserve {

    Error::Error(const char *file_p, const int line_p, const char *msg, int errno_p) : runtime_error(
            std::string(msg)
            + "\nFile: "
            + std::string(file_p)
            + std::string(" Line: ")
            + std::to_string(line_p)), line(line_p), file(file_p), message(msg), sysErrno(errno_p) {

    }
    //============================================================================


    Error::Error(const char *file_p, const int line_p, const std::string &msg, int errno_p) : runtime_error(
            std::string(msg)
            + "\nFile: "
            + std::string(file_p)
            + std::string(" Line: ")
            + std::to_string(line_p)), line(line_p), file(file_p), message(msg), sysErrno(errno_p) {

    }
    //============================================================================

    std::string Error::to_string() const {
        std::ostringstream err_stream;
        err_stream << "Error at [" << file << ": " << line << "]";
        if (sysErrno != 0) err_stream << " (system error: " << std::strerror(sysErrno) << ")";
        err_stream << ": " << message;
        return err_stream.str();
    }
    //============================================================================

    std::ostream &operator<<(std::ostream &out_stream, const Error &rhs) {
        std::string errStr = rhs.to_string();
        out_stream << errStr;
        return out_stream;
    }
    //============================================================================

}
