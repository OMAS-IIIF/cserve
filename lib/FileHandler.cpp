//
// Created by Lukas Rosenthaler on 29.06.21.
//
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <vector>
#include "Cserve.h"
#include "FileHandler.h"
#include "Parsing.h"

static const char __file__[] = __FILE__;

namespace cserve {

    /*!
     * This is the normal file handler that just sends the contents of the file.
     * It is being activated in the main program (e.g. in shttps.cpp or sipi.cpp)
     * using "server.addRoute()". For binary objects (images, video etc.) this handler
     * supports the HTTP range header.
     *
     * @param conn Connection instance
     * @param lua Lua interpreter instance
     * @param user_data Hook to user data
     * @param hd nullptr or pair (docroot, route)
     */
    void FileHandler(cserve::Connection &conn, LuaServer &lua, void *user_data, void *hd) {
        std::vector<std::string> headers = conn.header();
        std::string uri = conn.uri();

        std::string docroot;
        std::string route;

        if (hd == nullptr) {
            route = "/";
            docroot = ".";
        } else {
            std::pair<std::string, std::string> tmp = *((std::pair<std::string, std::string> *) hd);
            route = tmp.first;
            docroot = tmp.second;
        }
        Server::debugmsg(__LINE__, fmt::format("IN FILEHANDLER: route='{}' docroot='{}'", route, docroot));

        lua.add_servertableentry("docroot", docroot);
        if (uri.find(route) == 0) {
            uri = uri.substr(route.length());
            if (uri[0] != '/') uri = "/" + uri;
        }

        std::string infile = docroot + uri;

        if (access(infile.c_str(), R_OK) != 0) { // test, if file exists
            conn.status(Connection::NOT_FOUND);
            conn.header("Content-Type", "text/text; charset=utf-8");
            conn << "File not found\n";
            conn.flush();
            Server::logger()->error("FileHandler: '{}' not readable.", infile);
            return;
        }

        struct stat s;

        if (stat(infile.c_str(), &s) == 0) {
            if (!(s.st_mode & S_IFREG)) { // we have not a regular file, do nothing!
                conn.status(Connection::NOT_FOUND);
                conn.header("Content-Type", "text/text; charset=utf-8");
                conn << infile << " not aregular file\n";
                conn.flush();
                Server::logger()->error("FileHandler: '{}' is not regular file.", infile);
                return;
            }
        } else {
            conn.status(Connection::NOT_FOUND);
            conn.header("Content-Type", "text/text; charset=utf-8");
            conn << "Could not stat file" << infile << "\n";
            conn.flush();
            Server::logger()->error("FileHandler: Could not stat '{}'", infile);
            return;
        }

        std::pair<std::string, std::string> mime = Parsing::getFileMimetype(infile);

        size_t extpos = uri.find_last_of('.');
        std::string extension;

        if (extpos != std::string::npos) {
            extension = uri.substr(extpos + 1);
        }

        try {
            if ((extension == "html") && (mime.first == "text/html")) {
                conn.header("Content-Type", "text/html; charset=utf-8");
                conn.sendFile(infile);
            } else if (extension == "js") {
                conn.header("Content-Type", "application/javascript; charset=utf-8");
                conn.sendFile(infile);
            } else if (extension == "css") {
                conn.header("Content-Type", "text/css; charset=utf-8");
                conn.sendFile(infile);
            } else if (extension == "lua") { // pure lua
                conn.setBuffer();
                std::ifstream inf;
                inf.open(infile);//open the input file

                std::stringstream sstr;
                sstr << inf.rdbuf();//read the file
                std::string luacode = sstr.str();//str holds the content of the file

                try {
                    if (lua.executeChunk(luacode, infile) < 0) {
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
                inf.open(infile);//open the input file

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
                        if (lua.executeChunk(luastr, infile) < 0) {
                            conn.flush();
                            return;
                        }
                    } catch (Error &err) {
                        try {
                            conn.status(Connection::INTERNAL_SERVER_ERROR);
                            conn.header("Content-Type", "text/text; charset=utf-8");
                            conn << "Lua Error:\r\n==========\r\n" << err << "\r\n";
                            conn.flush();
                        } catch (InputFailure iofail) {}

                        Server::logger()->error("FileHandler: error executing lua chunk: {}", err.to_string());
                        return;
                    }
                }

                conn.header("Content-Type", "text/html; charset=utf-8");
                std::string htmlcode = eluacode.substr(end);
                conn << htmlcode;
                conn.flush();
            } else {
                std::string actual_mimetype = cserve::Parsing::getBestFileMimetype(infile);
                //
                // first we get the filesize and time using fstat
                //
                struct stat fstatbuf;

                if (stat(infile.c_str(), &fstatbuf) != 0) {
                    throw Error(__file__, __LINE__, "Cannot fstat file!");
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
                    conn.sendFile(infile);
                } else {
                    //
                    // now we parse the range
                    //
                    std::regex re("bytes=\\s*(\\d+)-(\\d*)[\\D.*]?");
                    std::cmatch m;
                    int start = 0; // lets assume beginning of file
                    int end = fsize - 1; // lets assume whole file
                    if (std::regex_match(range.c_str(), m, re)) {
                        if (m.size() < 2) {
                            throw Error(__file__, __LINE__, "Range expression invalid!");
                        }
                        start = std::stoi(m[1]);
                        if ((m.size() > 1) && !m[2].str().empty()) {
                            end = std::stoi(m[2]);
                        }
                    } else {
                        throw Error(__file__, __LINE__, "Range expression invalid!");
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
                    conn.header("Content-Disposition", std::string("inline; filename=") + infile);
                    conn.header("Content-Transfer-Encoding: binary");
                    conn.header("Last-Modified", timebuf);
                    conn.sendFile(infile, 8192, start, end);
                }
                conn.flush();
            }
        } catch (InputFailure iofail) {
            return; // we have an io error => just return, the thread will exit
        } catch (Error &err) {
            try {
                conn.status(Connection::INTERNAL_SERVER_ERROR);
                conn.header("Content-Type", "text/text; charset=utf-8");
                conn << err;
                conn.flush();
            } catch (InputFailure iofail) {}

            Server::logger()->error("FileHandler: internal error: {}", err.to_string());
            return;
        }
    }
    //=========================================================================

}
