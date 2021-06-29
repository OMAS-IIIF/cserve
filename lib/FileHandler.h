//
// Created by Lukas Rosenthaler on 29.06.21.
//

#ifndef CSERVER_FILEHANDLER_H
#define CSERVER_FILEHANDLER_H


#include "LuaServer.h"

namespace cserve {

    extern void FileHandler(cserve::Connection &conn, LuaServer &lua, void *user_data, void *hd);
}

#endif //CSERVER_FILEHANDLER_H
