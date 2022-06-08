//
// Created by Lukas Rosenthaler on 08.06.22.
//

#ifndef CSERVER_DEFAULTHANDLER_H
#define CSERVER_DEFAULTHANDLER_H

#include "RequestHandler.h"


namespace cserve {

    class DefaultHandler : public RequestHandler {
    public:
        DefaultHandler() : RequestHandler() {}

        void handler(Connection& conn, LuaServer &lua, void *user_data) override;
    };

}

#endif //CSERVER_DEFAULTHANDLER_H
