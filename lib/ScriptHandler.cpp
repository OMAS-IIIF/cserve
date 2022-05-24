//
// Created by Lukas Rosenthaler on 29.06.21.
//

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "ScriptHandler.h"
#include "LuaServer.h"
#include "Cserve.h"

namespace cserve {

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
    void ScriptHandler(cserve::Connection &conn, LuaServer &lua, void *user_data, std::shared_ptr<RequestHandlerData> request_data) {
        std::vector<std::string> headers = conn.header();
        std::string uri = conn.uri();
        std::shared_ptr<ScriptHandlerData> data = std::dynamic_pointer_cast<ScriptHandlerData>(request_data);

        if (data == nullptr) {
            conn.setBuffer();
            conn.status(Connection::INTERNAL_SERVER_ERROR);
            conn.header("Content-Type", "text/text; charset=utf-8");
            conn << "Error in ScriptHandler: No script path defined.\r\n";
            conn.flush();
            Server::logger()->error("Error in ScriptHandler: No script path defined.");
            return;
        }
        std::string script = data->scriptpath();

        if (access(script.c_str(), R_OK) != 0) { // test, if file exists
            conn.status(Connection::NOT_FOUND);
            conn.header("Content-Type", "text/text; charset=utf-8");
            conn << "File not found\n";
            conn.flush();
            Server::logger()->error("ScriptHandler '{}' not readable", script);
            return;
        }

        size_t extpos = script.find_last_of('.');
        std::string extension;

        if (extpos != std::string::npos) {
            extension = script.substr(extpos + 1);
        }

        try {
            if (extension == "lua") { // pure lua
                std::ifstream inf;
                inf.open(script); //open the input file
                std::stringstream sstr;
                sstr << inf.rdbuf(); //read the file
                std::string luacode = sstr.str();//str holds the content of the file

                try {
                    if (lua.executeChunk(luacode, script) < 0) {
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
            } else if (extension == "elua") { // embedded lua <lua> .... </lua>
                conn.setBuffer();
                std::ifstream inf;
                inf.open(script);//open the input file

                std::stringstream sstr;
                sstr << inf.rdbuf();//read the file
                std::string eluacode = sstr.str(); // eluacode holds the content of the file

                size_t pos = 0;
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
                        if (lua.executeChunk(luastr, script) < 0) {
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


}