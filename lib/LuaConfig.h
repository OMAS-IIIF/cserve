//
// Created by Lukas Rosenthaler on 23.06.22.
//

#ifndef CSERVER_LUACONFIG_H
#define CSERVER_LUACONFIG_H

#include <iostream>
#include <vector>
#include <map>
#include <unordered_map>
#include <variant>
#include <stdexcept>
#include <memory>
#include <spdlog/common.h>

#include "Error.h"
#include "Connection.h"
#include "Global.h"
#include "lua.hpp"

std::string configString(const std::string& table, const std::string& variable, const std::string& defval);

[[maybe_unused]] bool configBoolean(const std::string& table, const std::string& variable, bool defval);

int configInteger(const std::string& table, const std::string& variable, int defval);

spdlog::level::level_enum configLoglevel(const std::string& table, const std::string& variable, spdlog::level::level_enum defval);

[[maybe_unused]] float configFloat(const std::string& table, const std::string& variable, float defval);

[[maybe_unused]] std::vector<std::string> configStringList(const std::string& table, const std::string& stringlist);

[[maybe_unused]] std::map<std::string,std::string> configStringTable(
        const std::string &table,
        const std::string &variable,
        const std::map<std::string,std::string> &defval);

std::vector<LuaRoute> configRoute(const std::string& routetable);


#endif //CSERVER_LUACONFIG_H
