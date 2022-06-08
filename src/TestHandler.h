//
// Created by Lukas Rosenthaler on 08.06.22.
//

#ifndef CSERVER_TESTHANDLER_H
#define CSERVER_TESTHANDLER_H


#include "RequestHandler.h"

class TestHandler: public cserve::RequestHandler {
public:
    TestHandler() : RequestHandler() {}

    void handler(cserve::Connection& conn, cserve::LuaServer &lua, void *user_data) override;

};


#endif //CSERVER_TESTHANDLER_H
