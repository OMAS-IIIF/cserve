//
// Created by Lukas Rosenthaler on 29.06.21.
//

#ifndef CSERVER_SCRIPTHANDLER_H
#define CSERVER_SCRIPTHANDLER_H

#include <utility>

#include "LuaServer.h"
#include "RequestHandler.h"

namespace cserve {

    class ScriptHandler: public RequestHandler {
    private:
        const static std::string _name;
        std::string _scriptpath;
    public:
        explicit ScriptHandler(std::string  scriptpath) : RequestHandler(), _scriptpath(std::move(scriptpath)) {}

        const std::string& name() const override;

        void handler(Connection& conn, LuaServer &lua, void *user_data) override;

        std::string scriptpath() { return _scriptpath; }
    };

}

#endif //CSERVER_SCRIPTHANDLER_H
