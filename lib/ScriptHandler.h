//
// Created by Lukas Rosenthaler on 29.06.21.
//

#ifndef CSERVER_SCRIPTHANDLER_H
#define CSERVER_SCRIPTHANDLER_H

#include <utility>

#include "LuaServer.h"
#include "RequestHandlerData.h"
#include "RequestHandler.h"

namespace cserve {

    class ScriptHandler: public RequestHandler {
    private:
        std::string _scriptpath;
    public:
        explicit ScriptHandler(std::string  scriptpath) : RequestHandler(), _scriptpath(std::move(scriptpath)) {}

        void handler(Connection& conn, LuaServer &lua, void *user_data) override;

        std::string scriptpath() { return _scriptpath; }
    };

}

#endif //CSERVER_SCRIPTHANDLER_H
