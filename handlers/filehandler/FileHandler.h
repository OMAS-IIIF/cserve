//
// Created by Lukas Rosenthaler on 29.06.21.
//

#ifndef CSERVER_FILEHANDLER_H
#define CSERVER_FILEHANDLER_H


#include <utility>

#include "../../lib/LuaServer.h"
#include "../../lib/RequestHandlerData.h"
#include "../../lib/RequestHandler.h"

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
         std::string _docroot;
    public:
        [[maybe_unused]] FileHandler() : RequestHandler() {}

         const std::string& name() const override;

        void handler(Connection& conn, LuaServer &lua, const std::string &route) override;

        //[[maybe_unused]] std::string route() { return _route; }

        //[[maybe_unused]] std::string docroot() { return _docroot; }

         void set_config_variables(CserverConf &conf) override;

         void get_config_variables(const CserverConf &conf) override;

     };

}

#endif //CSERVER_FILEHANDLER_H
