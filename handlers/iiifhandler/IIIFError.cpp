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
#include "spdlog/fmt/bundled/format.h"

namespace cserve {

    IIIFError::IIIFError(const char *file, const int line, const char *msg, int errno_p)
        : Error(file, line, msg, errno_p) {
        cname = __func__;
    }

    IIIFError::IIIFError(const char *file, const int line, const std::string &msg, int errno_p)
        : Error(file, line, msg, errno_p) {
        cname = __func__;
    }

    std::ostream &operator<<(std::ostream &outStream, const IIIFError &rhs) {
        std::string errStr = rhs.to_string();
        outStream << errStr << std::endl; // TODO: remove the endl, the logging code should do it
        return outStream;
    }

} // cserve