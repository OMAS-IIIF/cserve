//
// Created by Lukas Rosenthaler on 15.06.22.
//

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
