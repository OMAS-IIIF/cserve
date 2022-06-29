//
// Created by Lukas Rosenthaler on 29.06.21.
//

#ifndef CSERVER_FILEHANDLER_H
#define CSERVER_FILEHANDLER_H


#include <utility>

#include "LuaServer.h"
#include "RequestHandlerData.h"
#include "RequestHandler.h"

namespace cserve {

    /*!
     * Implements the "standard" HTTP-server filehandler. It delivers all files as is, with the exception of
     * of files with the extension ".lua" and ".elua".
     * - ".lua" are files containing Lua code
     * - ".elua" are files containing embedded lua in HTML. The Lua code start with <lua> and ends with </lua>.
     */
     class FileHandler: public RequestHandler {
    private:
         const static std::string _name;
         std::string _route;
         std::string _docroot;
    public:
        [[maybe_unused]] FileHandler(std::string route, std::string docroot) : RequestHandler(), _route(std::move(route)), _docroot(std::move(docroot)) {}

         const std::string& name() const override;

        void handler(Connection& conn, LuaServer &lua, const std::string &route, void *user_data) override;

        [[maybe_unused]] std::string route() { return _route; }

        [[maybe_unused]] std::string docroot() { return _docroot; }
    };

}

#endif //CSERVER_FILEHANDLER_H
