//
// Created by Lukas Rosenthaler on 15.06.22.
//

#ifndef CSERVER_TESTHANDLER_H
#define CSERVER_TESTHANDLER_H

#include "../../lib/RequestHandler.h"

namespace cserve {

    class TestHandler : public RequestHandler {
    private:
        const static std::string _name;
        std::string _message{};
    public:
        TestHandler() : RequestHandler() {}

        const std::string& name() const override;

        void handler(Connection& conn, LuaServer& lua, const std::string &route) override;

        void set_config_variables(CserverConf &conf) override;

        void get_config_variables(const CserverConf &conf) override;

    };

} // cserve

#endif //CSERVER_TESTHANDLER_H
