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
#ifndef CSERVER_LIB_REQUESTHANDLERDATA_H_
#define CSERVER_LIB_REQUESTHANDLERDATA_H_

#include <string>

namespace cserve {
    class RequestHandlerData {
    public:
        inline RequestHandlerData() = default;

        inline virtual ~RequestHandlerData() = default;

    };

}

#endif //CSERVER_LIB_REQUESTHANDLERDATA_H_
