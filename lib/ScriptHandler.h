//
// Created by Lukas Rosenthaler on 29.06.21.
//

#ifndef CSERVER_SCRIPTHANDLER_H
#define CSERVER_SCRIPTHANDLER_H

#include "LuaServer.h"

namespace cserve {

    extern void ScriptHandler(cserve::Connection &conn, LuaServer &lua, void *user_data, void *hd);

}

#endif //CSERVER_SCRIPTHANDLER_H
