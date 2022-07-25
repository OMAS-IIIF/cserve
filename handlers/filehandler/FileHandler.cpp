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
#include <sys/stat.h>
#include <unistd.h>
#include <filesystem>

#include <vector>
#include "Cserve.h"
#include "FileHandler.h"
#include "Parsing.h"

static const char file_[] = __FILE__;

namespace cserve {

    const std::string FileHandler::_name = "filehandler";

    const std::string& FileHandler::name() const {
        return _name;
    }

    /*!
     * This is the normal file handler that just sends the contents of the file.
     * It is being activated in the main program (e.g. in shttps.cpp or sipi.cpp)
     * using "server.addRoute()". For binary objects (images, video etc.) this handler
     * supports the HTTP range header.
     *
     * @param conn Connection instance
     * @param lua Lua interpreter instance
     * @param user_data Hook to user data
     */
    void FileHandler::handler(cserve::Connection &conn, LuaServer &lua, const std::string &route) {
        std::vector<std::string> headers = conn.header();
        std::string uri = conn.uri();

        if (_docroot.empty()) {
            try {
                conn.setBuffer();
                conn.status(Connection::INTERNAL_SERVER_ERROR);
                conn.header("Content-Type", "text/text; charset=utf-8");
                conn << "Error in FileHandler: FileHandler request data missing.\r\n";
                conn.flush();
            }
            catch (InputFailure &err) {}
            Server::logger()->error("Error in FileHandler: FileHandler request data missing.");
            return;
        }

        lua.add_servertableentry("docroot", _docroot);
        if (uri.find(route) == 0) {
            uri = uri.substr(route.length());
            if (uri[0] != '/') uri = "/" + uri;
        }

        std::filesystem::path path(_docroot + uri);

        //
        // if there is no filename given, check if there is something like "index.XXX"
        //
        if (path.filename().empty()) {
            std::filesystem::path newpath(path);
            newpath /= "index.html";
            if (access(newpath.c_str(), R_OK) == 0) {
                path /= "index.html";
            }

            newpath = path;
            newpath /= "index.htm";
            if (access(newpath.c_str(), R_OK) == 0) {
                path /= "index.htm";
            }

            newpath = path;
            newpath /= "index.elua";
            if (access(newpath.c_str(), R_OK) == 0) {
                path /= "index.elua";
            }

            newpath = path;
            newpath /= "index.lua";
            if (access(newpath.c_str(), R_OK) == 0) {
                path /= "index.lua";
            }
        }

        if (access(path.c_str(), R_OK) != 0) { // test, if file exists
            try {
                conn.status(Connection::NOT_FOUND);
                conn.header("Content-Type", "text/text; charset=utf-8");
                conn << "File not found.\r\n";
                conn.flush();
            }
            catch (InputFailure &err) {}
            Server::logger()->error("FileHandler: '{}' not readable.", path.string());
            return;
        }

        struct stat s{};

        if (stat(path.c_str(), &s) == 0) {
            if (!(s.st_mode & S_IFREG)) { // we have not a regular file, do nothing!
                try {
                    conn.setBuffer();
                    conn.status(Connection::NOT_FOUND);
                    conn.header("Content-Type", "text/text; charset=utf-8");
                    conn << path << " not a regular file\r\n";
                    conn.flush();
                }
                catch (InputFailure &err) {}
                Server::logger()->error("FileHandler: '{}' is not regular file.", path.string());
                return;
            }
        } else {
            try {
                conn.setBuffer();
                conn.status(Connection::NOT_FOUND);
                conn.header("Content-Type", "text/text; charset=utf-8");
                conn << "Could not stat file" << path << "\r\n";
                conn.flush();
            }
            catch (InputFailure &err) {}
            Server::logger()->error("FileHandler: Could not stat '{}'", path.string());
            return;
        }
        std::pair<std::string, std::string> mime = Parsing::getFileMimetype(path.string());

        size_t extpos = uri.find_last_of('.');
        std::string extension;

        if (extpos != std::string::npos) {
            extension = uri.substr(extpos + 1);
        }

        try {
            if ((extension == "html") && (mime.first == "text/html")) {
                conn.header("Content-Type", "text/html; charset=utf-8");
                conn.sendFile(path.string());
            } else if (extension == "js") {
                conn.header("Content-Type", "application/javascript; charset=utf-8");
                conn.sendFile(path.string());
            } else if (extension == "css") {
                conn.header("Content-Type", "text/css; charset=utf-8");
                conn.sendFile(path.string());
            } else if (extension == "lua") { // pure lua
                conn.setBuffer();
                std::ifstream inf;
                inf.open(path.string());//open the input file

                std::stringstream sstr;
                sstr << inf.rdbuf();//read the file
                std::string luacode = sstr.str();//str holds the content of the file

                try {
                    if (lua.executeChunk(luacode, path.string()) < 0) {
                        conn.flush();
                        return;
                    }
                } catch (Error &err) {
                    try {
                        conn.status(Connection::INTERNAL_SERVER_ERROR);
                        conn.header("Content-Type", "text/text; charset=utf-8");
                        conn << "Lua Error:\r\n==========\r\n" << err << "\r\n";
                        conn.flush();
                    } catch (int i) {
                        Server::logger()->error("FileHandler: error executing lua chunk: {}", err.to_string());
                        return;
                    }
                    Server::logger()->error("FileHandler: error executing lua chunk: {}", err.to_string());
                    return;
                }

                conn.flush();
            } else if (extension == "elua") { // embedded lua <lua> .... </lua>
                conn.setBuffer();
                std::ifstream inf;
                inf.open(path.string());//open the input file

                std::stringstream sstr;
                sstr << inf.rdbuf();//read the file
                std::string eluacode = sstr.str(); // eluacode holds the content of the file

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
                        if (lua.executeChunk(luastr, path.string()) < 0) {
                            conn.flush();
                            return;
                        }
                    } catch (Error &err) {
                        try {
                            conn.status(Connection::INTERNAL_SERVER_ERROR);
                            conn.header("Content-Type", "text/text; charset=utf-8");
                            conn << "Lua Error:\r\n==========\r\n" << err << "\r\n";
                            conn.flush();
                        } catch (InputFailure &iofail) {}

                        Server::logger()->error("FileHandler: error executing lua chunk: {}", err.to_string());
                        return;
                    }
                }

                conn.header("Content-Type", "text/html; charset=utf-8");
                std::string htmlcode = eluacode.substr(end);
                conn << htmlcode;
                conn.flush();
            } else {
                std::string actual_mimetype = cserve::Parsing::getBestFileMimetype(path.string());
                //
                // first we get the filesize and time using fstat
                //
                struct stat fstatbuf{};

                if (stat(path.c_str(), &fstatbuf) != 0) {
                    throw Error(file_, __LINE__, "Cannot fstat file!");
                }
                size_t fsize = fstatbuf.st_size;
#ifdef __APPLE__
                struct timespec rawtime = fstatbuf.st_mtimespec;
#else
                struct timespec rawtime = fstatbuf.st_mtim;
#endif
                char timebuf[100];
                std::strftime(timebuf, sizeof timebuf, "%a, %d %b %Y %H:%M:%S %Z", std::gmtime(&rawtime.tv_sec));

                std::string range = conn.header("range");
                if (range.empty()) {
                    conn.header("Content-Type", actual_mimetype);
                    conn.header("Cache-Control", "public, must-revalidate, max-age=0");
                    conn.header("Pragma", "no-cache");
                    conn.header("Accept-Ranges", "bytes");
                    conn.header("Content-Length", std::to_string(fsize));
                    conn.header("Last-Modified", timebuf);
                    conn.header("Content-Transfer-Encoding: binary");
                    conn.sendFile(path.string());
                } else {
                    //
                    // now we parse the range
                    //
                    std::regex re(R"(bytes=\s*(\d+)-(\d*)[\D.*]?)");
                    std::cmatch m;
                    std::streamsize start;
                    auto end = static_cast<std::streamsize>(fsize - 1); // lets assume whole file
                    if (std::regex_match(range.c_str(), m, re)) {
                        if (m.size() < 2) {
                            throw Error(file_, __LINE__, "Range expression invalid!");
                        }
                        start = std::stoi(m[1]);
                        if ((m.size() > 1) && !m[2].str().empty()) {
                            end = std::stoi(m[2]);
                        }
                    } else {
                        throw Error(file_, __LINE__, "Range expression invalid!");
                    }

                    conn.status(Connection::PARTIAL_CONTENT);
                    conn.header("Content-Type", actual_mimetype);
                    conn.header("Cache-Control", "public, must-revalidate, max-age=0");
                    conn.header("Pragma", "no-cache");
                    conn.header("Accept-Ranges", "bytes");
                    conn.header("Content-Length", std::to_string(end - start + 1));
                    conn.header("Content-Length", std::to_string(end - start + 1));
                    std::stringstream ss;
                    ss << "bytes " << start << "-" << end << "/" << fsize;
                    conn.header("Content-Range", ss.str());
                    conn.header("Content-Disposition", std::string("inline; filename=") + path.string());
                    conn.header("Content-Transfer-Encoding: binary");
                    conn.header("Last-Modified", timebuf);
                    conn.sendFile(path.string(), 8192, start, end);
                }
                conn.flush();
                Server::logger()->info("Sent {} to {}:{}.", path.string(), conn.peer_ip(), conn.peer_port());
            }
        } catch (InputFailure &iofail) {
            return; // we have an io error => just return, the thread will exit
        } catch (Error &err) {
            try {
                conn.setBuffer();
                conn.status(Connection::INTERNAL_SERVER_ERROR);
                conn.header("Content-Type", "text/text; charset=utf-8");
                conn << err;
                conn.flush();
            } catch (InputFailure &iofail) {}

            Server::logger()->error("FileHandler: internal error: {}", err.to_string());
            return;
        }
    }
    //=========================================================================

    void FileHandler::set_config_variables(CserverConf &conf) {
        std::vector<RouteInfo> routes = {
                RouteInfo("GET:/:C++"),
                RouteInfo("POST:/:C++"),
                RouteInfo("PUT:/:C++"),
        };
        conf.add_config(_name, "routes",routes, "Route for filehandler");
        conf.add_config(_name, "docroot", "./docroot", "Path to directory containing Lua scripts to implement routes.");
    }

    void FileHandler::get_config_variables(const CserverConf &conf) {
        _docroot = conf.get_string("docroot").value_or("-- no scriptdir --");
    }

    void FileHandler::set_lua_globals(lua_State *L, cserve::Connection &conn) {
        lua_createtable(L, 0, 1);
        lua_pushstring(L, "docroot");
        lua_pushstring(L, _docroot.c_str());
        lua_rawset(L, -3); // table1
        lua_setglobal(L, _name.c_str());
    }
}

extern "C" cserve::FileHandler * create_filehandler() {
    return new cserve::FileHandler();
};

extern "C" void destroy_filehandler(cserve::FileHandler *handler) {
    delete handler;
}
