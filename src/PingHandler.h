//
// Created by Lukas Rosenthaler on 08.06.22.
//

#ifndef CSERVER_PINGHANDLER_H
#define CSERVER_PINGHANDLER_H

#include "RequestHandler.h"


class PingHandler: public cserve::RequestHandler {
public:
    PingHandler() : RequestHandler() {}

    void handler(cserve::Connection& conn, cserve::LuaServer &lua, void *user_data) override;
};


#endif //CSERVER_PINGHANDLER_H
