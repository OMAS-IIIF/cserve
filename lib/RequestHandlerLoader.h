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
#ifndef CSERVER_REQUESTHANDLERLOADER_H
#define CSERVER_REQUESTHANDLERLOADER_H

#include <string>
#include "RequestHandler.h"

namespace cserve {

    class RequestHandlerLoader {
    private:
        void *handle;
        std::string path;
        std::string allocatorSymbol;
        std::string deleterSymbol;

    public:
        RequestHandlerLoader(std::string path, std::string allocatorSymbol, std::string deleterSymbol)
        : handle(nullptr), path(std::move(path)), allocatorSymbol(std::move(allocatorSymbol)), deleterSymbol(std::move(deleterSymbol)) {}

        ~RequestHandlerLoader() = default;

        void load();

        void close();

        std::shared_ptr<RequestHandler> get_instance();

    };

}


#endif //CSERVER_REQUESTHANDLERLOADER_H
