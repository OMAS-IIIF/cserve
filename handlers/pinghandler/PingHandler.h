/*
 * Copyright © 2022 Lukas Rosenthaler
 * This file is part of OMAS/cserve
 * OMAS/cserve is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * OMAS/cserve is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
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

        void handler(Connection& conn, LuaServer& lua, const std::string &route) override;

        void set_config_variables(CserverConf &conf) override;

        void get_config_variables(const CserverConf &conf) override;
    };

} // cserve

#endif //CSERVER_PINGHANDLER_H
