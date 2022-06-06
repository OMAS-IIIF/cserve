//
// Created by Lukas Rosenthaler on 29.06.21.
//

#ifndef CSERVER_FILEHANDLER_H
#define CSERVER_FILEHANDLER_H


#include <utility>

#include "LuaServer.h"
#include "RequestHandlerData.h"

namespace cserve {

    class FileHandlerData: public RequestHandlerData {
    private:
        std::string _route;
        std::string _docroot;
    public:
        FileHandlerData(std::string route, std::string docroot) : RequestHandlerData(), _route(std::move(route)), _docroot(std::move(docroot)) {}

        std::string route() { return _route; }

        std::string docroot() { return _docroot; }
    };

    extern void FileHandler(cserve::Connection &conn, LuaServer &lua, void *user_data, std::shared_ptr<RequestHandlerData> request_data);
}

#endif //CSERVER_FILEHANDLER_H
