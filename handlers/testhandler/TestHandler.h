//
// Created by Lukas Rosenthaler on 15.06.22.
//

#ifndef CSERVER_TESTHANDLER_H
#define CSERVER_TESTHANDLER_H

#include "../../lib/RequestHandler.h"

namespace cserve {

    class TestHandler : public RequestHandler {
    public:
        TestHandler() : RequestHandler() {}

        void handler(Connection& conn, LuaServer& lua, void* user_data) override;

    };

} // cserve

#endif //CSERVER_TESTHANDLER_H
