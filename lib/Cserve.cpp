/*
 * Copyright © 2016 Lukas Rosenthaler, Andrea Bianco, Benjamin Geer,
 * Ivan Subotic, Tobias Schweizer, André Kilchenmann, and André Fatton.
 * This file is part of Sipi.
 * Sipi is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * Sipi is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * Additional permission under GNU AGPL version 3 section 7:
 * If you modify this Program, or any covered work, by linking or combining
 * it with Kakadu (or a modified version of that library) or Adobe ICC Color
 * Profiles (or a modified version of that library) or both, containing parts
 * covered by the terms of the Kakadu Software Licence or Adobe Software Licence,
 * or both, the licensors of this Program grant you additional permission
 * to convey the resulting work.
 * See the GNU Affero General Public License for more details.
 * You should have received a copy of the GNU Affero General Public
 * License along with Sipi.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <algorithm>
#include <functional>
#include <cctype>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>      // Needed for memset
#include <utility>
#include <regex>

#include <sys/types.h>
#include <sys/select.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <signal.h>
#include <poll.h>
#include <netinet/in.h>
#include <arpa/inet.h> //inet_addr
#include <unistd.h>    //write
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <pthread.h>
#include <pwd.h>
//
// openssl includes
//#include "openssl/applink.c"

#include "spdlog/spdlog.h"

#include "Global.h"
#include "SockStream.h"
#include "Cserve.h"
#include "LuaServer.h"
#include "Parsing.h"
#include "makeunique.h"

#include "CserveVersion.h"

static const char __file__[] = __FILE__;

static std::mutex debugio; // mutex to protect debugging messages from threads

std::string cserve::Server::_loggername = "";
std::shared_ptr<spdlog::logger> cserve::Server::_logger = nullptr;

namespace cserve {

    typedef struct {
        int sock;
#ifdef CSERVE_ENABLE_SSL
        SSL *cSSL;
#endif
        std::string peer_ip;
        int peer_port;
        int commpipe_read;
        Server *serv;
    } TData;
    //=========================================================================


    /*!
     * Starts a thread just to catch all signals sent to the server process.
     * If it receives SIGINT or SIGTERM, tells the server to stop.
     */
    static void *sig_thread(void *arg) {
        Server *serverptr = static_cast<Server *>(arg);
        sigset_t set;
        sigemptyset(&set);
        sigaddset(&set, SIGPIPE);
        sigaddset(&set, SIGINT);
        sigaddset(&set, SIGTERM);

        int sig;

        while (true) {
            if ((sigwait(&set, &sig)) != 0) {
                return nullptr;
            }

            // If we get SIGINT or SIGTERM, shut down the server.
            // Ignore any other signals. We must in particular ignore
            // SIGPIPE.
            if (sig == SIGINT || sig == SIGTERM) {
                serverptr->stop();
                return nullptr;
            }
        }
    }
    //=========================================================================

    /**
     * This ist just the default handler that handles any unknown routes or requests. It
     * is called if the server does not know how to ahndle a requerst...
     *
     * @param conn Connection instance
     * @param lua Lua interpreter instance
     * @param user_data Hook to user data
     * @param hd not used
     */
    static void default_handler(Connection &conn, LuaServer &lua, void *user_data, void *hd) {
        conn.status(Connection::NOT_FOUND);
        conn.header("Content-Type", "text/text");
        conn.setBuffer();

        try {
            conn << "No handler available" << Connection::flush_data;
        } catch (InputFailure iofail) {
            Server::logger()->error("No handler and no connection available.");
            return;
        }

        Server::logger()->error("No handler available. Host: '{}' Uri: '{}'", conn.host(), conn.uri());
        return;
    }
    //=========================================================================

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
    void ScriptHandler(cserve::Connection &conn, LuaServer &lua, void *user_data, void *hd) {
        std::vector<std::string> headers = conn.header();
        std::string uri = conn.uri();

        std::string script = *((std::string *) hd);

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
                        } catch (InputFailure iofail) {
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
        } catch (InputFailure iofail) {
            return; // we have an io error => just return, the thread will exit
        } catch (Error &err) {
            try {
                conn.status(Connection::INTERNAL_SERVER_ERROR);
                conn.header("Content-Type", "text/text; charset=utf-8");
                conn << err;
                conn.flush();
            } catch (InputFailure iofail) {
                return;
            }

            Server::logger()->error("FileHandler: internal error: '{}'", err.to_string());
            return;
        }
    }
    //=========================================================================

    /**
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
            // docroot = *((std::string *) hd);
            route = tmp.first;
            docroot = tmp.second;
        }

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

                std::string htmlcode = eluacode.substr(end);
                conn << htmlcode;
                conn.flush();
            } else {
                std::string actual_mimetype = cserve::Parsing::getFileMimetype(infile).first;
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

    /**
     * Create an instance of the simple HTTP server.
     *
     * @param port_p Port number to listen on for incoming connections
     * @param nthreads_p Number of parallel threads that will be used to serve the requests.
     *                   If there are more requests then this number, these requests will
     *                   be put on hold on socket level
     * @param userid_str User id the server runs on. This requires the server is started as
     *                   root. It will then use setuid() to change to the given user id. If
     *                   the string is empty, the server is run as the user that is starting
     *                   the server.
     * @param logfile_p Name of the logfile
     * @param loglevel_p Loglevel, must be one of "DEBUG", "INFO", "NOTICE", "WARNING", "ERR",
     *                   "CRIT", "ALERT" or "EMERG".
     */
    Server::Server(int port,
                   unsigned nthreads,
                   const std::string &userid_str) :
                   _port(port),
                   _nthreads(nthreads) {
        _ssl_port = -1;
        _user_data = nullptr;
        running = false;
        _keep_alive_timeout = 20;

        std::shared_ptr<spdlog::logger> logger;
        if (_loggername.empty()) {
            logger = spdlog::stdout_color_mt("cserve_logger");
        }
        else if ((logger = spdlog::get(_loggername)) == nullptr) {
            logger = spdlog::stdout_color_mt(_loggername);
        }

        //openlog(loggername, LOG_CONS | LOG_PERROR, LOG_DAEMON);
        //openlog(loggername, LOG_PERROR, LOG_DAEMON);
        //setlogmask(LOG_UPTO(LOG_DEBUG));


        //
        // Her we check if we have to change to a different uid. This can only be done
        // if the server runs originally as root!
        //
        if (!userid_str.empty()) {
            if (getuid() == 0) { // must be root to setuid() !!
                struct passwd pwd, *res;
                size_t buffer_len = sysconf(_SC_GETPW_R_SIZE_MAX) * sizeof(char);
                auto buffer = std::make_unique<char[]>(buffer_len);
                getpwnam_r(userid_str.c_str(), &pwd, buffer.get(), buffer_len, &res);

                if (res != nullptr) {
                    if (setuid(pwd.pw_uid) == 0) {
                        int old_ll = setlogmask(LOG_MASK(LOG_INFO));
                        Server::logger()->info("Server will run as user {} ({}).", userid_str, getuid());
                        setlogmask(old_ll);

                        if (setgid(pwd.pw_gid) == 0) {
                            int old_ll = setlogmask(LOG_MASK(LOG_INFO));
                            Server::logger()->info("Server will run with group-id {}", getgid());
                            setlogmask(old_ll);
                        } else {
                            Server::logger()->error("setgid() failed: {}", strerror(errno));
                        }
                    } else {
                        Server::logger()->error("setgid() failed: {}", strerror(errno));
                    }
                } else {
                    Server::logger()->error("Could not get uid of user {}: you must start Sipi as root.", userid_str);
                }
            } else {
                Server::logger()->error("Could not get uid of user {}: you must start Sipi as root.", userid_str);
            }
        }
#ifdef CSERVE_ENABLE_SSL
        SSL_load_error_strings();
        SSL_library_init();
        OpenSSL_add_all_algorithms();
#endif

    }



#ifdef CSERVE_ENABLE_SSL
    /**
     * Set the Jason Web Token (jwt) secret that the server will use
     *
     * @param jwt_secret_p String containing the secret.
     */
    void Server::jwt_secret(const std::string &jwt_secret_p) {
        _jwt_secret = jwt_secret_p;
        auto secret_size = _jwt_secret.size();

        if (secret_size < 32) {
            for (int i = 0; i < (32 - secret_size); i++) {
                _jwt_secret.push_back('A' + i);
            }
        }
    }
    //=========================================================================
#endif

    /**
     * Get the correct handler to handle an incoming request. It seaches all
     * the supplied handlers to find the correct one. It returns the appropriate
     * handler and returns the handler data in handler_data_p
     *
     * @param conn Connection instance
     * @param handler_data_p Returns the pointer to the handler data
     * @return The appropriate handler for this request.
     */
    RequestHandler Server::getHandler(Connection &conn, void **handler_data_p) {
        std::map<std::string, RequestHandler>::reverse_iterator item;

        size_t max_match_len = 0;
        std::string matching_path;
        RequestHandler matching_handler = nullptr;

        for (item = handler[conn.method()].rbegin(); item != handler[conn.method()].rend(); ++item) {
          //TODO:: Selects wrong handler if the URI starts with the substring
            size_t len = conn.uri().length() < item->first.length() ? conn.uri().length() : item->first.length();

            if (item->first == conn.uri().substr(0, len)) {
                if (len > max_match_len) {
                    max_match_len = len;
                    matching_path = item->first;
                    matching_handler = item->second;
                }
            }
        }

        if (max_match_len > 0) {
            *handler_data_p = handler_data[conn.method()][matching_path];
            return matching_handler;
        }
        return default_handler;
    }
    //=============================================================================

    /**
     * Setup the server socket with the correct parameters
     * @param port Port number
     * @return Socket ID
     */
    static int prepare_socket(int port) {
        int sockfd;
        struct sockaddr_in serv_addr;

        sockfd = socket(AF_INET, SOCK_STREAM, 0);

        if (sockfd < 0) {
            Server::logger()->error("Could not create socket: {}", strerror(errno));
            exit(1);
        }

        int optval = 1;

        if (::setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval) < 0) {
            Server::logger()->error("Could not set socket option: {}", strerror(errno));
            exit(1);
        }

        /* Initialize socket structure */
        bzero((char *) &serv_addr, sizeof(serv_addr));

        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = INADDR_ANY;
        serv_addr.sin_port = htons(port);

        /* Now bind the host address using bind() call.*/
        if (::bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
            Server::logger()->error("Could not bind socket: {}", strerror(errno));
            exit(1);
        }

        if (::listen(sockfd, SOMAXCONN) < 0) {
            Server::logger()->error("Could not listen on socket: {}", strerror(errno));
            exit(1);
        }

        return sockfd;
    }
    //=========================================================================

    std::shared_ptr<spdlog::logger> Server::create_logger(spdlog::level::level_enum level,  bool consolelog, const std::string &logfile) {
        if (Server::_loggername.empty()) Server::_loggername = "cserver_logger";
        Server::_logger = spdlog::stdout_color_mt(Server::_loggername);
        return Server::_logger;
    }
    //=========================================================================

    std::shared_ptr<spdlog::logger> Server::logger() {
        if (Server::_logger == nullptr) {
            Server::_logger = Server::create_logger();
        }
        return Server::_logger;
    };
    //=========================================================================

    /**
     * Close a socket
     * @param tdata Pointer to thread data
     * @return
     */
    static int close_socket(const SocketControl::SocketInfo &sockid) {
#ifdef CSERVE_ENABLE_SSL
        if (sockid.ssl_sid != nullptr) {
            int sstat;
            while ((sstat = SSL_shutdown(sockid.ssl_sid)) == 0);
            if (sstat < 0) {
                Server::logger()->warn("SSL socket error: shutdown of socket failed at [{}: {}] with error code {}",
                                       __file__, __LINE__, SSL_get_error(sockid.ssl_sid, sstat));
            }
            SSL_free(sockid.ssl_sid);
        }
#endif
        if (shutdown(sockid.sid, SHUT_RDWR) < 0) {
            Server::logger()->debug("Debug: shutting down socket at [{}: {}]: {} failed (client terminated already?)",
                                    __file__, __LINE__, strerror(errno));
        }

        if (close(sockid.sid) == -1) {
            Server::logger()->debug("Debug: closing socket at [{}: {}]: {} failed (client terminated already?)",
                                    __file__, __LINE__, strerror(errno));
        }

        return 0;
    }
    //=========================================================================

    static void *process_request(void *arg) {
        ThreadControl::ThreadChildData *tdata = static_cast<ThreadControl::ThreadChildData *>(arg);
        //pthread_t my_tid = pthread_self();


        pollfd readfds[1];
        readfds[0] = {tdata->control_pipe, POLLIN, 0};

        int poll_status = -1;
        do {
            poll_status = poll(readfds, 1, -1);
            if (poll_status < 0) {
                Server::logger()->error("Blocking poll on control pipe failed at [{}: {}]", __file__, __LINE__);
                tdata->result = -1;
                return nullptr;
            }
            if (readfds[0].revents == POLLIN) {
                SocketControl::SocketInfo msg = SocketControl::receive_control_message(tdata->control_pipe);
                switch (msg.type) {
                    case SocketControl::ERROR:
                        break; // should never happen!
                    case SocketControl::PROCESS_REQUEST: {
                        //
                        // here we process the request
                        //
                        std::unique_ptr<SockStream> sockstream;
#ifdef CSERVE_ENABLE_SSL
                        if (msg.ssl_sid != nullptr) {
                            sockstream = std::make_unique<SockStream>(msg.ssl_sid);
                        } else {
                            sockstream = std::make_unique<SockStream>(msg.sid);
                        }
#else
                        sockstream = std::make_unique<SockStream>(receive_msg.sid);
#endif

                        std::istream ins(sockstream.get());
                        std::ostream os(sockstream.get());
                        //
                        // let's process the current request
                        //
                        cserve::ThreadStatus tstatus;
                        int keep_alive = 1;
                        std::string tmpstr(msg.peer_ip);
#ifdef CSERVE_ENABLE_SSL
                        if (msg.ssl_sid != nullptr) {
                            tstatus = tdata->serv->processRequest(&ins, &os, tmpstr,
                                                                  msg.peer_port, true, keep_alive);
                        } else {
                            tstatus = tdata->serv->processRequest(&ins, &os, tmpstr,
                                                                  msg.peer_port, false, keep_alive);
                        }
#else
                        tstatus = tdata->serv->processRequest(&ins, &os, tmpstr, msg.peer_port,
                                                              false, keep_alive);
#endif
                        //
                        // send the finished message
                        //
                        //SocketControl::SocketInfo send_msg = receive_msg;
                        if (tstatus == CONTINUE) {
                            msg.type = SocketControl::FINISHED_AND_CONTINUE;
                        } else {
                            msg.type = SocketControl::FINISHED_AND_CLOSE;
                        }
                        SocketControl::send_control_message(tdata->control_pipe, msg);
                        break;
                    }
                    case SocketControl::EXIT: {
                        //SocketControl::SocketInfo send_msg = receive_msg;
                        tdata->result = 0;
                        //close(tdata->control_pipe);
                        //SocketControl::send_control_message(tdata->control_pipe, msg);
                        return nullptr;
                    }
                    case SocketControl::NOOP: {
                        break;
                    }
                    default: {
                        ;
                    }
                }
            } else if (readfds[0].revents == POLLHUP) {
                return nullptr;
            } else if (readfds[0].revents == POLLERR) {
                Server::logger()->error("Thread pool got POLLERR message");
                return nullptr;
            } else {
                Server::logger()->error("Thread pool got UNKNONW(!) message");
                return nullptr;
            }
        } while (true);

        return nullptr;
    }


    SocketControl::SocketInfo Server::accept_connection(int sock, bool ssl) {
        SocketControl::SocketInfo socket_id;
        //
        // accepting new connection (normal socket=
        //
        struct sockaddr_storage cli_addr;
        socklen_t cli_size = sizeof(cli_addr);
        socket_id.sid = accept(sock, (struct sockaddr *) &cli_addr, &cli_size);

        if (socket_id.sid <= 0) {
            Server::logger()->error("Socket error  at [{}: {}]: {}", __file__, __LINE__, strerror(errno));
            // ToDo: Perform appropriate action!
        }
        socket_id.type = SocketControl::NOOP;
        socket_id.socket_type = SocketControl::DYN_SOCKET;

        //
        // get peer address
        //
        if (cli_addr.ss_family == AF_INET) {
            struct sockaddr_in *s = (struct sockaddr_in *) &cli_addr;
            socket_id.peer_port = ntohs(s->sin_port);
            inet_ntop(AF_INET, &s->sin_addr, socket_id.peer_ip, sizeof(socket_id.peer_ip));
        } else if (cli_addr.ss_family == AF_INET6) { // AF_INET6
            struct sockaddr_in6 *s = (struct sockaddr_in6 *) &cli_addr;
            socket_id.peer_port = ntohs(s->sin6_port);
            inet_ntop(AF_INET6, &s->sin6_addr, socket_id.peer_ip, sizeof(socket_id.peer_ip));
        } else {
            socket_id.peer_port = -1;
        }
#ifdef CSERVE_ENABLE_SSL
        SSL *cSSL = nullptr;

        if (ssl) {
            SSL_CTX *sslctx;
            try {
                if ((sslctx = SSL_CTX_new(SSLv23_server_method())) == nullptr) {
                    Server::logger()->error("OpenSSL error: SSL_CTX_new() failed");
                    throw SSLError(__file__, __LINE__, "OpenSSL error: SSL_CTX_new() failed");
                }
                SSL_CTX_set_options(sslctx, SSL_OP_SINGLE_DH_USE);
                if (SSL_CTX_use_certificate_file(sslctx, _ssl_certificate.c_str(), SSL_FILETYPE_PEM) != 1) {
                    Server::logger()->error("OpenSSL error [{}, {}]: SSL_CTX_use_certificate_file({}) failed.",
                                            __file__, __LINE__, _ssl_certificate);
                    throw SSLError(__file__, __LINE__,
                                   fmt::format("OpenSSL error: SSL_CTX_use_certificate_file({}) failed.", _ssl_certificate));
                }
                if (SSL_CTX_use_PrivateKey_file(sslctx, _ssl_key.c_str(), SSL_FILETYPE_PEM) != 1) {
                    Server::logger()->error("OpenSSL error [{}, {}]: SSL_CTX_use_PrivateKey_file({}) failed",
                                            __file__, __LINE__, _ssl_certificate);
                    throw SSLError(__file__, __LINE__,
                                   fmt::format("OpenSSL error: SSL_CTX_use_PrivateKey_file({}) failed", _ssl_certificate));
                }
                if (!SSL_CTX_check_private_key(sslctx)) {
                    Server::logger()->error("OpenSSL error [{}, {}]: SSL_CTX_check_private_key() failed",
                                            __file__, __LINE__);
                    throw SSLError(__file__, __LINE__, "OpenSSL error: SSL_CTX_check_private_key() failed");
                }
                if ((cSSL = SSL_new(sslctx)) == nullptr) {
                    Server::logger()->error("OpenSSL error [{}, {}]: SSL_new() failed", __file__, __LINE__);
                    throw SSLError(__file__, __LINE__, "OpenSSL error: SSL_new() failed");
                }
                if (SSL_set_fd(cSSL, socket_id.sid) != 1) {
                    Server::logger()->error("OpenSSL error [{}, {}]: SSL_set_fd() failed", __file__, __LINE__);
                    throw SSLError(__file__, __LINE__, "OpenSSL error: SSL_set_fd() failed");
                }

                //Here is the SSL Accept portion.  Now all reads and writes must use SS
                if ((SSL_accept(cSSL)) <= 0) {
                    Server::logger()->error("OpenSSL error [{}, {}]: SSL_accept() failed", __file__, __LINE__);
                    throw SSLError(__file__, __LINE__, "OpenSSL error: SSL_accept() failed");
                }
            } catch (SSLError &err) {
                Server::logger()->error(err.to_string());
                int sstat;

                while ((sstat = SSL_shutdown(cSSL)) == 0);

                if (sstat < 0) {
                    Server::logger()->warn("SSL socket error: shutdown (2) of socket failed: {}",
                                           SSL_get_error(cSSL, sstat));
                }

                SSL_free(cSSL);
                cSSL = nullptr;
            }
        }
        socket_id.ssl_sid = cSSL;
#endif
        return socket_id;
    }

    /**
     * Run the shttps server
     */
    void Server::run() {
        Server::logger()->debug("In Server::run");
        // Start a thread just to catch signals sent to the server process.
        pthread_t sighandler_thread;
        sigset_t set;
        sigemptyset(&set);
        sigaddset(&set, SIGINT);
        sigaddset(&set, SIGTERM);
        sigaddset(&set, SIGPIPE);

        int pthread_sigmask_result = pthread_sigmask(SIG_BLOCK, &set, nullptr);
        if (pthread_sigmask_result != 0) {
            Server::logger()->error("pthread_sigmask failed! (err={})", pthread_sigmask_result);
        }

        //
        // start the thread that handles incoming signals (SIGTERM, SIGPIPE etc.)
        //
        if (pthread_create(&sighandler_thread, nullptr, &sig_thread, (void *) this) != 0) {
            Server::logger()->error("Couldn't create thread: {}", strerror(errno));
            return;
        }

        int old_ll = setlogmask(LOG_MASK(LOG_INFO));
        Server::logger()->info("Starting cserve server with {} threads", _nthreads);
        setlogmask(old_ll);

        Server::logger()->info("Creating thread pool....");
        ThreadControl thread_control(_nthreads, process_request, this);
        SocketControl socket_control(thread_control);
        //
        // now we are adding the lua routes
        //
        for (auto &route : _lua_routes) {
            route.script = _scriptdir + "/" + route.script;
            addRoute(route.method, route.route, ScriptHandler, &(route.script));

            old_ll = setlogmask(LOG_MASK(LOG_INFO));
            Server::logger()->info("Added route '{}' with script '{}'", route.route.c_str(), route.script.c_str());
            setlogmask(old_ll);
        }

        _sockfd = prepare_socket(_port);
        old_ll = setlogmask(LOG_MASK(LOG_INFO));
        Server::logger()->info("Server listening on HTTP port {}", _port);
        setlogmask(old_ll);

        if (_ssl_port > 0) {
            _ssl_sockfd = prepare_socket(_ssl_port);
            old_ll = setlogmask(LOG_MASK(LOG_INFO));
            Server::logger()->info("Server listening on SSL port {}", _ssl_port);
            setlogmask(old_ll);
        }

        if (socketpair(PF_LOCAL, SOCK_STREAM, 0, stoppipe) != 0) {
            Server::logger()->error("Creating pipe failed at [{}: {}]: {}", __file__, __LINE__, strerror(errno));
            return;
        }

        socket_control.add_stop_socket(stoppipe[0]);
        socket_control.add_http_socket(_sockfd); // [1] -> normal socket we are listening on
        if (_ssl_port > 0) {
            socket_control.add_ssl_socket(_ssl_sockfd);
        }

        running = true;
        while (running) {
            //
            // blocking poll on input sockets waiting for *new* connections
            //
            pollfd *sockets = socket_control.get_sockets_arr();
            int nsocks;
            if ((nsocks = poll(sockets, socket_control.get_sockets_size(), -1)) < 0) {
                Server::logger()->error("Blocking poll failed at [{}: {}]: {}", __file__, __LINE__, strerror(errno));
                running = false;
                break;
            }

            for (int i = 0; i < socket_control.get_sockets_size(); i++) {
                if (sockets[i].revents) {
                    if ((sockets[i].revents & POLLIN) || (sockets[i].revents & POLLPRI)) {
                        //
                        // we've got input on one of the sockets
                        //
                        if (i < socket_control.get_n_msg_sockets()) {
                            //
                            // CONTROL_SOCKET: we got input from an internal thread...
                            //
                            SocketControl::SocketInfo msg = SocketControl::receive_control_message(sockets[i].fd);
                            switch (msg.type) {
                                case SocketControl::FINISHED_AND_CONTINUE: {
                                    //
                                    // A thread finished, but the socket should remain open...
                                    // If there are waiting sockets, the thread pops one from the queue, else,
                                    // if there are no waiting sockets, the thread will pause and be put on the list of
                                    // available threads.
                                    //
                                    msg.type = SocketControl::NOOP;
                                    socket_control.add_dyn_socket(msg); // add socket to list to pe polled ==> CHANGES open_sockets!!
                                    //
                                    // see if we have sockets waiting for a thread.If yes, we reuse this thread directly
                                    //
                                    SocketControl::SocketInfo sockid;
                                    if (socket_control.get_waiting(sockid)) {
                                        //
                                        // We have a waiting socket. Get it and make the thread processing it!
                                        //
                                        sockid.type = SocketControl::PROCESS_REQUEST;
                                        SocketControl::send_control_message(sockets[i].fd, sockid);
                                    } else {
                                        //
                                        // we have no waiting socket, so
                                        // push the thread to the list of available threads
                                        //
                                        ThreadControl::ThreadMasterData tinfo = thread_control[i]; // get thread info
                                        thread_control.thread_push(tinfo); // push thread to list of waiting threads
                                    }
                                    break;
                                }
                                case SocketControl::FINISHED_AND_CLOSE: {
                                    //
                                    // A thread finished and expects the socket to be closed
                                    //
                                    close_socket(msg); // close the socket
                                    //
                                    // see if we have sockets waiting for a thread.If yes, we reuse this thread directly
                                    //
                                    SocketControl::SocketInfo sockid;
                                    if (socket_control.get_waiting(sockid)) {
                                        //
                                        // We have a waiting socket. Get it and make the thread processing it!
                                        //
                                        sockid.type = SocketControl::PROCESS_REQUEST;
                                        SocketControl::send_control_message(sockets[i].fd, sockid);
                                    } else {
                                        //
                                        // we have no waiting socket, so
                                        // push the thread to the list of available threads
                                        //
                                        ThreadControl::ThreadMasterData tinfo = thread_control[i]; // get thread info
                                        thread_control.thread_push(tinfo); // push thread to list of waiting threads
                                    }
                                    break;
                                }
                                case SocketControl::SOCKET_CLOSED: {
                                    //
                                    // a control socket has been closed – we assume the thread just finished
                                    //
                                    ::close(thread_control[i].control_pipe);
                                    thread_control.thread_delete(i);
                                    break;
                                }
                                case SocketControl::EXIT: {
                                    //
                                    // a thread sent an EXIT message –– should not happen!!
                                    //
                                    Server::logger()->error("A worker thread sent an EXIT message! This should never happen!");
                                    break;
                                }
                                case SocketControl::ERROR: {
                                    Server::logger()->error("A worker thread sent an EXIT message! This should never happen!");
                                    // ToDo: React to this message
                                    break;
                                }
                                default: {
                                    Server::logger()->error("A worker thread sent an non-identifiable message! This should never happen!");
                                    // error handling
                                }
                            }
                            // we got input ready from thread....
                            // we expect  FINISHED or SOCKET_CLOSED
                        } else if (i == socket_control.get_stop_socket_id()) {
                            //
                            // STOP from interrupt thread: got input ready from stoppipe
                            //

                            SocketControl::SocketInfo msg = SocketControl::receive_control_message(sockets[i].fd); // read the message from the pipe
                            if (msg.type != SocketControl::EXIT) {
                                Server::logger()->error("Got unexpected message from interrupt");
                            }

                            SocketControl::SocketInfo sockid;
                            socket_control.remove(socket_control.get_http_socket_id(), sockid); // remove the HTTP socket
#ifdef SHTTPS_ENABLE_SSL
                            socket_control.remove(socket_control.get_ssl_socket_id(), sockid); // remove the SSL socket
#endif
                            socket_control.close_all_dynsocks(close_socket);
                            socket_control.broadcast_exit(); // broadcast EXIT to all worker threads
                            running = false;
                        } else if (i == socket_control.get_http_socket_id()) {
                            //
                            // external HTTP request coming in
                            //
                            // we got input ready from normal listener socket
                            SocketControl::SocketInfo sockid = accept_connection(sockets[i].fd, false);
                            socket_control.add_dyn_socket(sockid);
                            Server::logger()->info("Accepted connection from {}", sockid.peer_ip);
                        } else if (i == socket_control.get_ssl_socket_id()) {
                            //
                            // external SSL request coming in
                            //
                            SocketControl::SocketInfo sockid = accept_connection(sockets[i].fd, true);
                            socket_control.add_dyn_socket(sockid);
                            Server::logger()->info("Accepted SSL connection from {}", sockid.peer_ip);

                        } else {
                            //
                            // DYN_SOCKET: a client socket (already accepted) has data -> dispatch the processing to a free thread
                            // or put the request in the waiting queue (waiting for a free thread...)
                            //
                            ThreadControl::ThreadMasterData tinfo;
                            if (thread_control.thread_pop(tinfo)) { // thread available
                                SocketControl::SocketInfo sockid;
                                socket_control.remove(i, sockid); //  ==> CHANGES open_sockets!!
                                sockid.type = SocketControl::PROCESS_REQUEST;
                                int n = SocketControl::send_control_message(tinfo.control_pipe, sockid);
                                if (n < 0) {
                                    Server::logger()->warn("Got something unexpected...");
                                }
                            } else { // no thread available, push socket into waiting queue
                                socket_control.move_to_waiting(i); //  ==> CHANGES open_sockets!!
                            }
                        }
                    } else if (sockets[i].revents & POLLHUP) {
                        //
                        // we got a HANGUP from a socket
                        //

                        if (i >= socket_control.get_dyn_socket_base()) {
                            //
                            // ist a hangup from a dynamic client socket!
                            // we close and remove it
                            //
                            SocketControl::SocketInfo sockid;
                            socket_control.remove(i, sockid); //  ==> CHANGES open_sockets!!
                            close_socket(sockid);
                        } else if (i < socket_control.get_n_msg_sockets()) {
                            //
                            // it's a hangup from one of the thread sockets -> thread exited
                            //
                            SocketControl::SocketInfo sockid;
                            socket_control.remove(i, sockid); //  ==> CHANGES open_sockets!!
                            thread_control.thread_delete(i); // delete the thread
                            if (socket_control.get_n_msg_sockets() == 0) {
                                running = false;
                            }
                        } else if (i == socket_control.get_http_socket_id()) {
                            //
                            // The HTTP socked was being closed -> must be EXIT
                            //
                            SocketControl::SocketInfo sockid;
                            socket_control.remove(i, sockid); //  ==> CHANGES open_sockets!!
#ifdef SHTTPS_ENABLE_SSL
                        } else if (i == socket_control.get_ssl_socket_id()) {
                            //
                            // The HTTP socked was being closed -> must be EXIT - do nothing!
                            //
                            SocketControl::SocketInfo sockid;
                            socket_control.remove(i, sockid); //  ==> CHANGES open_sockets!!
                        } else {
#else
                        } else {
#endif
                            Server::logger()->error("We got a HANGUP from an unknown socket (socket_id = {})", i);
                        }
                    } else {
                        // we've got something else...
                        if (sockets[i].revents & POLLERR) {
                            Server::logger()->debug("-->POLLERR");
                        }
                        if (sockets[i].revents & POLLHUP) {
                            Server::logger()->debug("-->POLLHUP");
                        }
                        if (sockets[i].revents & POLLIN) {
                            Server::logger()->debug("-->POLLIN");
                        }
                        if (sockets[i].revents & POLLNVAL) {
                            Server::logger()->debug("-->POLLNVAL");
                        }
                        if (sockets[i].revents & POLLOUT) {
                            Server::logger()->debug("-->POLLOUT");
                        }
                        if (sockets[i].revents & POLLPRI) {
                            Server::logger()->debug("-->POLLPRI");
                        }
                        if (sockets[i].revents & POLLRDBAND) {
                            Server::logger()->debug("-->POLLRDBAND");
                        }
                        if (sockets[i].revents & POLLRDNORM) {
                            Server::logger()->debug("-->POLLRDNORM");
                        }
                        if (sockets[i].revents & POLLWRBAND) {
                            Server::logger()->debug("-->POLLWRBAND");
                        }
                        if (sockets[i].revents & POLLWRNORM) {
                            Server::logger()->debug("-->POLLWRNORM");
                        }
                        //running = false;
                        //break; // accept returned something strange – probably we want to shutdown the server
                    }
                }
            }
        }

        old_ll = setlogmask(LOG_MASK(LOG_INFO));
        Server::logger()->info("Server shutting down");
        setlogmask(old_ll);

        //close(stoppipe[0]);
        //close(stoppipe[1]);
    }
    //=========================================================================


    void Server::addRoute(Connection::HttpMethod method_p, const std::string &path_p, RequestHandler handler_p,
                          void *handler_data_p) {
        handler[method_p][path_p] = handler_p;
        handler_data[method_p][path_p] = handler_data_p;
    }
    //=========================================================================


    cserve::ThreadStatus
    Server::processRequest(std::istream *ins, std::ostream *os, std::string &peer_ip, int peer_port, bool secure,
                           int &keep_alive, bool socket_reuse) {
        if (_tmpdir.empty()) {
            Server::logger()->warn("_tmpdir is empty [{}, {}].", __file__, __LINE__);
            throw Error(__file__, __LINE__, "_tmpdir is empty");
        }
        if (ins->eof() || os->eof()) return CLOSE;
        try {
            Connection conn(this, ins, os, _tmpdir);

            if (keep_alive <= 0) {
                conn.keepAlive(false);
            }
            keep_alive = conn.setupKeepAlive(_keep_alive_timeout);

            conn.peer_ip(peer_ip);
            conn.peer_port(peer_port);
            conn.secure(secure);

            if (conn.resetConnection()) {
                if (conn.keepAlive()) {
                    return CONTINUE;
                } else {
                    return CLOSE;
                }
            }

            //
            // Setting up the Lua server
            //
            // pattern to be added to the Lua package.path
            // includes Lua files in the Lua script directory
            std::string lua_scriptdir = _scriptdir + "/?.lua";

            LuaServer luaserver(conn, _initscript, true, lua_scriptdir);

            for (auto &global_func : lua_globals) {
                global_func.func(luaserver.lua(), conn, global_func.func_dataptr);
            }

            void *hd = nullptr;

            try {
                RequestHandler handler = getHandler(conn, &hd);
                handler(conn, luaserver, _user_data, hd);
            } catch (InputFailure iofail) {
                Server::logger()->error("Possibly socket closed by peer");
                return CLOSE; // or CLOSE ??
            }

            if (!conn.cleanupUploads()) {
                Server::logger()->error("Cleanup of uploaded files failed");
            }

            if (conn.keepAlive()) {
                return CONTINUE;
            } else {
                return CLOSE;
            }
        } catch (InputFailure iofail) { // "error" is thrown, if the socket was closed from the main thread...
            Server::logger()->debug("Socket connection: timeout or socket closed from main");
            return CLOSE;
        } catch (Error &err) {
            Server::logger()->warn("Internal server error: {}", err.to_string());
            try {
                *os << "HTTP/1.1 500 INTERNAL_SERVER_ERROR\r\n";
                *os << "Content-Type: text/plain\r\n";
                std::stringstream ss;
                ss << err;
                *os << "Content-Length: " << ss.str().length() << "\r\n\r\n";
                *os << ss.str();
            } catch (InputFailure iofail) {
                Server::logger()->debug("Possibly socket closed by peer");
            }
            return CLOSE;
        }
    }
    //=========================================================================

    void Server::debugmsg(const int line, const std::string &msg) {
        std::lock_guard<std::mutex> debug_mutex_guard(debugio);

        std::cerr << "DBG> " << line << " " << msg << std::endl;
    }
    //=========================================================================

}
