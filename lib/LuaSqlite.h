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
#ifndef CSERVE_LUASQLITE_H
#define CSERVE_LUASQLITE_H

#include <iostream>
#include <vector>

#include "LuaServer.h"

namespace cserve {
    extern void sqliteGlobals(lua_State *L, cserve::Connection &conn, void *user_data);
}

#endif //CSERVE_LUASQLITE_H
