//
// Created by Lukas Rosenthaler on 15.06.22.
//

#ifndef CSERVER_PINGHANDLER_H
#define CSERVER_PINGHANDLER_H

#include "../../lib/RequestHandler.h"

namespace cserve {

    class PingHandler : public RequestHandler {
    public:
        PingHandler() : RequestHandler() {}

        void handler(Connection& conn, LuaServer& lua, void* user_data) override;

    };

} // cserve

#endif //CSERVER_PINGHANDLER_H
