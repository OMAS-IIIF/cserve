//
// Created by Lukas Rosenthaler on 15.06.22.
//

#ifndef CSERVER_PINGHANDLER_H
#define CSERVER_PINGHANDLER_H

#include "PingHandler.h"
#include "../../lib/RequestHandler.h"

namespace cserve {

    class PingHandler : public RequestHandler {
    private:
        const static std::string _name;
        std::string _echo{};
    public:
        PingHandler() : RequestHandler() {}

        const std::string& name() const override;

        void handler(Connection& conn, LuaServer& lua, const std::string &route, void* user_data) override;

        void set_config_variables(CserverConf &conf) override;

        void get_config_variables(const CserverConf &conf) override;
    };

} // cserve

#endif //CSERVER_PINGHANDLER_H
