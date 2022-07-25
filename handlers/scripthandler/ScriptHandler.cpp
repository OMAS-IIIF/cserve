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
#include <unistd.h>

#include "ScriptHandler.h"
#include "../../lib/LuaServer.h"
#include "../../lib/Cserve.h"

namespace cserve {

    const std::string ScriptHandler::_name = "scripthandler";

    const std::string& ScriptHandler::name() const {
        return _name;
    }

    /**
     * This is the handler that is used to execute pure lua scripts (e.g. implementing
     * RESTful services based on Lua. The file must have the extention ".lua" or
     * ".elua" (for embeded lua in HTML, tags <lua>...</lua> in order that this handler
     * is being called.
     *
     * @param conn Connection instance
     * @param lua Lua interpreter instance
     * @param user_data
     * @param hd Pointer to string object containing the lua script file name
     */
    void ScriptHandler::handler(cserve::Connection &conn, LuaServer &lua, const std::string &route) {
        std::vector<std::string> headers = conn.header();
        std::string scriptname{};
        try {
            scriptname = routedata.at(route);
        }
        catch(std::out_of_range &err) {
            try {
                conn.setBuffer();
                conn.status(Connection::INTERNAL_SERVER_ERROR);
                conn.header("Content-Type", "text/text; charset=utf-8");
                conn << "Error in ScriptHandler: No script route defined.\r\n";
                conn.flush();
            }
            catch (InputFailure &err) {}
            Server::logger()->error("Error in ScriptHandler: No script route defined.");
            return;
        }

        if (scriptname.empty()) {
            try {
                conn.setBuffer();
                conn.status(Connection::INTERNAL_SERVER_ERROR);
                conn.header("Content-Type", "text/text; charset=utf-8");
                conn << "Error in ScriptHandler: No script path defined.\r\n";
                conn.flush();
            }
            catch (InputFailure &err) {}
            Server::logger()->error("Error in ScriptHandler: No script path defined.");
            return;
        }

        std::filesystem::path scriptpath(_scriptdir);
        scriptpath /= scriptname;

        std::error_code ec; // For noexcept overload usage.
        if (!std::filesystem::exists(scriptpath, ec) && !ec) {
            try {
                conn.status(Connection::NOT_FOUND);
                conn.header("Content-Type", "text/text; charset=utf-8");
                conn << "File not found\n";
                conn.flush();
            }
            catch (InputFailure &err) {}
            Server::logger()->error("Error in ScriptHandler: Script '{}' not existing", scriptpath.string());
            return;
        }
        if (access(scriptpath.c_str(), R_OK) != 0) { // test, if file exists
            try {
                conn.status(Connection::NOT_FOUND);
                conn.header("Content-Type", "text/text; charset=utf-8");
                conn << "File not found\n";
                conn.flush();
            }
            catch (InputFailure &err) {}
            Server::logger()->error("Error in ScriptHandler: Script '{}' not readable", scriptpath.string());
            return;
        }

        std::string extension = scriptpath.extension();

        try {
            if (extension == ".lua") { // pure lua
                std::ifstream inf;
                inf.open(scriptpath); //open the input file
                std::stringstream sstr;
                sstr << inf.rdbuf(); //read the file
                std::string luacode = sstr.str();//str holds the content of the file
                inf.close();
                try {
                    if (lua.executeChunk(luacode, scriptpath.c_str()) < 0) {
                        conn.flush();
                        return;
                    }
                } catch (Error &err) {
                    try {
                        conn.setBuffer();
                        conn.status(Connection::INTERNAL_SERVER_ERROR);
                        conn.header("Content-Type", "text/text; charset=utf-8");
                        conn << "Lua Error:\r\n==========\r\n" << err << "\r\n";
                        conn.flush();
                    } catch (int i) {
                        return;
                    }

                    Server::logger()->error("ScriptHandler: error executing lua script: '{}'", err.to_string());
                    return;
                }
                conn.flush();
            } else if (extension == ".elua") { // embedded lua <lua> .... </lua>
                conn.setBuffer();
                std::ifstream inf;
                inf.open(scriptpath);//open the input file

                std::stringstream sstr;
                sstr << inf.rdbuf(); //read the file
                std::string eluacode = sstr.str(); // eluacode holds the content of the file
                inf.close();

                size_t pos;
                size_t end = 0; // end of last lua code (including </lua>)

                while ((pos = eluacode.find("<lua>", end)) != std::string::npos) {
                    std::string htmlcode = eluacode.substr(end, pos - end);
                    pos += 5;

                    if (!htmlcode.empty()) conn << htmlcode; // send html...

                    std::string luastr;

                    if ((end = eluacode.find("</lua>", pos)) != std::string::npos) { // we found end;
                        luastr = eluacode.substr(pos, end - pos);
                        end += 6;
                    } else {
                        luastr = eluacode.substr(pos);
                    }

                    try {
                        if (lua.executeChunk(luastr, scriptpath) < 0) {
                            conn.flush();
                            return;
                        }
                    } catch (Error &err) {
                        try {
                            conn.status(Connection::INTERNAL_SERVER_ERROR);
                            conn.header("Content-Type", "text/text; charset=utf-8");
                            conn << "Lua Error:\r\n==========\r\n" << err << "\r\n";
                            conn.flush();
                        } catch (InputFailure &iofail) {
                            return;
                        }

                        Server::logger()->error("ScriptHandler: error executing lua chunk: '{}'", err.to_string());
                        return;
                    }
                }

                std::string htmlcode = eluacode.substr(end);
                conn << htmlcode;
                conn.flush();
            } else {
                conn.status(Connection::INTERNAL_SERVER_ERROR);
                conn.header("Content-Type", "text/text; charset=utf-8");
                conn << "Script has no valid extension: '" << extension << "' !";
                conn.flush();
                Server::logger()->error("ScriptHandler: error executing script, unknown extension: '{}'", extension);
            }
        } catch (InputFailure &iofail) {
            return; // we have an io error => just return, the thread will exit
        } catch (Error &err) {
            try {
                conn.status(Connection::INTERNAL_SERVER_ERROR);
                conn.header("Content-Type", "text/text; charset=utf-8");
                conn << err;
                conn.flush();
            } catch (InputFailure &iofail) {
                return;
            }

            Server::logger()->error("ScriptHandler: internal error: '{}'", err.to_string());
            return;
        }
    }

    void ScriptHandler::set_config_variables(CserverConf &conf) {
        std::vector<RouteInfo> routes = {};
        conf.add_config(_name, "routes",routes, "Route(s) for scripthandler");
        conf.add_config(_name, "scriptdir", "./scripts", "Path to directory containing Lua scripts to implement routes.");
    }

    void ScriptHandler::get_config_variables(const CserverConf &conf) {
        _scriptdir = conf.get_string("scriptdir").value_or("-- no scriptdir --");
    }

    void ScriptHandler::set_lua_globals(lua_State *L, cserve::Connection &conn) {
        lua_createtable(L, 0, 1);
        lua_pushstring(L, "scriptdir");
        lua_pushstring(L, _scriptdir.c_str());
        lua_rawset(L, -3); // table1
        lua_setglobal(L, _name.c_str());
    }

}

extern "C" cserve::ScriptHandler * create_scripthandler() {
    return new cserve::ScriptHandler();
};

extern "C" void destroy_scripthandler(cserve::ScriptHandler *handler) {
    delete handler;
}
