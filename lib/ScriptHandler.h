//
// Created by Lukas Rosenthaler on 29.06.21.
//

#ifndef CSERVER_SCRIPTHANDLER_H
#define CSERVER_SCRIPTHANDLER_H

#include "LuaServer.h"
#include "RequestHandlerData.h"

namespace cserve {

    class ScriptHandlerData: public RequestHandlerData {
    private:
        std::string _scriptpath;
    public:
        explicit ScriptHandlerData(const std::string scriptpath) : RequestHandlerData(), _scriptpath(scriptpath) {}

        std::string scriptpath() { return _scriptpath; }
    };

    extern void ScriptHandler(cserve::Connection &conn, LuaServer &lua, void *user_data, std::shared_ptr<RequestHandlerData> request_data);

}

#endif //CSERVER_SCRIPTHANDLER_H
