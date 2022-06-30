//
// Created by Lukas Rosenthaler on 08.06.22.
//

#ifndef CSERVER_DEFAULTHANDLER_H
#define CSERVER_DEFAULTHANDLER_H

#include "RequestHandler.h"


namespace cserve {

    class DefaultHandler : public RequestHandler {
        const static std::string _name;
    public:
        DefaultHandler() : RequestHandler() {}

        const std::string& name() const override;

        void handler(Connection& conn, LuaServer &lua, const std::string &route) override;
    };

}

#endif //CSERVER_DEFAULTHANDLER_H
