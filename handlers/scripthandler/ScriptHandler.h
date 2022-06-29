//
// Created by Lukas Rosenthaler on 29.06.21.
//

#ifndef CSERVER_SCRIPTHANDLER_H
#define CSERVER_SCRIPTHANDLER_H

#include <utility>
#include <unordered_map>

#include "../../lib/LuaServer.h"
#include "../../lib/RequestHandler.h"

namespace cserve {

    class ScriptHandler: public RequestHandler {
    private:
        const static std::string _name;
        std::string _scriptdir{};
        std::string _scriptname{};
    public:
        explicit ScriptHandler() : RequestHandler() {}

        const std::string& name() const override;

        void handler(Connection& conn, LuaServer &lua, const std::string &route, void *user_data) override;

        std::string scriptpath() { return _scriptname; }

        void set_config_variables(CserverConf &conf) override;

        void get_config_variables(const CserverConf &conf) override;
    };

}

#endif //CSERVER_SCRIPTHANDLER_H
