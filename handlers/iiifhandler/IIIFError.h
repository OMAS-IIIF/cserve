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
#ifndef CSERVER_IIIFERROR_H
#define CSERVER_IIIFERROR_H

#include "../../lib/Error.h"

namespace cserve {

    class IIIFError : public cserve::Error {
    private:
    public:

        IIIFError(const char *file, int line, const char *msg, int errno_p = 0);

        IIIFError(const char *file, int line, const std::string &msg, int errno_p = 0);

        friend std::ostream &operator<<(std::ostream &lhs, const IIIFError &rhs);
    };



} // cserve

#endif //CSERVER_IIIFERROR_H
