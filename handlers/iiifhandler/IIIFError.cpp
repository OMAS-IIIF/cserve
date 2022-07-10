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
#include <sstream>

#include "IIIFError.h"

namespace cserve {

    IIIFError::IIIFError(const char *file_p, const int line_p, const char *msg, int errno_p) : Error(file_p, line_p,
                                                                                                     msg, errno_p) {}
    //============================================================================


    IIIFError::IIIFError(const char *file_p, const int line_p, const std::string &msg, int errno_p) : Error(file_p,
                                                                                                            line_p, msg,
                                                                                                            errno_p) {}
    //============================================================================

    std::string IIIFError::to_string(void) const {
        std::ostringstream errStream;
        errStream << "Sipi Error at [" << file << ": " << line << "]";
        if (sysErrno != 0) errStream << " (system error: " << std::strerror(sysErrno) << ")";
        errStream << ": " << message;
        return errStream.str();
    }
    //============================================================================

    std::ostream &operator<<(std::ostream &outStream, const IIIFError &rhs) {
        std::string errStr = rhs.to_string();
        outStream << errStr << std::endl; // TODO: remove the endl, the logging code should do it
        return outStream;
    }
    //============================================================================

} // cserve