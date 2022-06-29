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

 /*!
 * \brief Implements a simple HTTP server.
 *
 */
#ifndef __cserve_server_h
#define __cserve_server_h


#include <map>
#include <vector>
#include <queue>
#include <mutex>
#include <csignal>

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h> //for threading , link with lpthread
#include <semaphore.h>
#include <syslog.h>
#include <poll.h>

#include <atomic>
#include <netdb.h>      // Needed for the socket functions
#include <sstream>      // std::stringstream
#include <iostream>

#include "openssl/bio.h"
#include "openssl/ssl.h"
#include "openssl/err.h"

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"

#include "Global.h"
#include "Error.h"

#include "Connection.h"
#include "LuaServer.h"

#include "ThreadControl.h"
#include "SocketControl.h"

#include "RequestHandlerData.h"

#include "FileHandler.h"

#include "lua.hpp"

/*
 * How to create a self-signed certificate
 *
 * openssl genrsa -out key.pem 2048
 * openssl req -new -key key.pem -out csr.pem
 * openssl req -x509 -days 365 -key key.pem -in csr.pem -out certificate.pem
 */

namespace cserve {

    typedef enum {
        CONTINUE, CLOSE
    } ThreadStatus;

    /*!
     * \brief Implements a simple, even primitive HTTP server with routes and handlers
     *
     * Implementation of a simple, almost primitive, multithreaded HTTP server. The user can define for
     * request types and different paths handlers which will be called. The handler gets an Connection
     * instance which will be used to receive and send data.
     *
     *     void MirrorHandler(cserve::Connection &conn, void *user_data)
     *     {
     *         conn.setBuffer();
     *         std::vector<std::string> headers = conn.header();
     *         if (!conn.query("html").empty()) {
     *             conn.header("Content-Type", "text/html; charset=utf-8");
     *             conn << "<html><head>";
     *             conn << "<title>Mirror, mirror – on the wall...</title>";
     *             conn << "</head>" << cserve::Connection::flush_data;
     *
     *             conn << "<body><h1>Header fields</h1>";
     *             conn << "<table>";
     *             conn << <tr><th>Fieldname</th><th>Value</th></tr>";
     *             for (unsigned i = 0; i < headers.size(); i++) {
     *               conn << "<tr><td>" << headers[i] << "</td><td>" << conn.header(headers[i]) << "</td></tr>";
     *             }
     *             conn << "</table>"
     *             conn << "</body></html>" << cserve::Connection::flush_data;
     *         }
     *         else {
     *             conn.header("Content-Type", "text/plain; charset=utf-8");
     *             for (unsigned i = 0; i < headers.size(); i++) {
     *                 conn << headers[i] << " : " << conn.header(headers[i]) << "\n";
     *             }
     *         }
     *     }
     *
     *     cserve::Server server(4711);
     *     server.addRoute(cserve::Connection::GET, "/", RootHandler);
     *     server.addRoute(cserve::Connection::GET, "/mirror", MirrorHandler);
     *     server.run();
     *
     */
    class Server {
        /*!
         * Struct to hold Global Lua function and associated userdata
         */
        typedef struct {
            LuaSetGlobalsFunc func;
            void *func_dataptr;
        } GlobalFunc;


        /*!
         * Error handling class for SSL functions
         */
        class SSLError : Error {
        protected:
            SSL *cSSL{};
        public:
            inline SSLError(const char *file,
                            const int line,
                            const char *msg,
                            SSL *cSSL_p = nullptr) : Error(file, line, msg), cSSL(cSSL_p) {};

            inline SSLError(const char *file, const int line, const std::string &msg, SSL *cSSL_p = nullptr) : Error(
                    file, line, msg), cSSL(cSSL_p) {};

            inline std::string to_string() {
                std::stringstream ss;
                ss << "SSL-ERROR at [" << file << ": " << line << "] ";
                BIO *bio = BIO_new(BIO_s_mem());
                ERR_print_errors(bio);
                char *buf = nullptr;
                long n = BIO_get_mem_data (bio, &buf);
                if (n > 0) {
                    ss << buf << " : ";
                }
                BIO_free(bio);
                //ss << "Description: " << message;
                return ss.str();
            };
        };

    public:
        static std::string _loggername; //!< global logger name
        static std::shared_ptr<spdlog::logger> _logger; //!< shared pointer to logger

    private:
        int _port; //!< listening Port for server
        int _ssl_port; //!< listening port for openssl
        int _sockfd; //!< socket id
        int _ssl_sockfd; //!< SSL socket id

        std::string _ssl_certificate; //!< Path to SSL certificate
        std::string _ssl_key; //!< Path to SSL certificate
        std::string _jwt_secret;

        int stoppipe[2]{};

        std::string _tmpdir; //!< path to directory, where uplaods are being stored
        std::string _scriptdir; //!< Path to directory, where scripts for the "Lua"-routes are found
        unsigned _nthreads; //!< maximum number of parallel threads for processing requests
        std::map<pthread_t, SocketControl::SocketInfo> thread_ids; //!< Map of active worker threads
        int _keep_alive_timeout;
        bool running; //!< Main runloop should keep on going
        std::map<std::string, std::shared_ptr<RequestHandler>> handler[Connection::NumHttpMethods]; // request handlers for the different 9 request methods
        std::shared_ptr<RequestHandler> default_handler;
        void *_user_data; //!< Some opaque user data that can be given to the Connection (for use within the handler)
        std::string _initscript;
        std::vector<cserve::LuaRoute> _lua_routes; //!< This vector holds the routes that are served by lua scripts
        std::vector<GlobalFunc> lua_globals;
        size_t _max_post_size;

        std::tuple<std::shared_ptr<RequestHandler>, std::string> get_handler(Connection &conn);

        SocketControl::SocketInfo accept_connection(int sock, bool ssl = false);

    public:
        /*!
        * Create a server listening on the given port with the maximal number of threads
        *
        * \param[in] port_p Listening port of HTTP server
        * \param[in] nthreads_p Maximal number of parallel threads serving the requests
        */
        explicit Server(int port,
                        unsigned nthreads = 4,
                        const std::string &userid_str = "");

        [[maybe_unused]] inline static void loggername(const std::string &loggername) { _loggername = loggername; }

        [[maybe_unused]] inline static const std::string &loggername() { return _loggername; }

        static std::shared_ptr<spdlog::logger> create_logger(spdlog::level::level_enum level = spdlog::level::debug,
                                                             bool consolelog = true,
                                                             const std::string &logfile = "");

        static std::shared_ptr<spdlog::logger> logger(spdlog::level::level_enum level = spdlog::level::debug);

        [[maybe_unused]] [[nodiscard]] inline int port() const { return _port; }


        /*!
         * Sets the port number for the SSL socket
         *
         * \param[in] ssl_port_p Port number
         */
        inline void ssl_port(int ssl_port_p) { _ssl_port = ssl_port_p; }

        /*!
         * Gets the port number of the SSL socket
         *
         * \returns SSL socket portnumber
         */
        [[maybe_unused]] [[nodiscard]] inline int ssl_port() const { return _ssl_port; }

        /*!
         * Sets the file path to the SSL certficate necessary for OpenSSL to work
         *
         * \param[in] path File path to th SSL certificate
         */
        inline void ssl_certificate(const std::string &path) { _ssl_certificate = path; }

        /*!
         * Returns the path of the SSL certificate
         *
         * \returns Path to the SSL certificate
         */
        [[maybe_unused]] inline std::string ssl_certificate() { return _ssl_certificate; }

        /*!
         * Sets the path to the SSP key
         *
         * \param[in] path Path to the SSL key necessary for OpenSSL to work
         */
        [[maybe_unused]] inline void ssl_key(const std::string &path) { _ssl_key = path; }

        /*!
         * Returns the path of the OpenSSL key
         *
         * \returns Path to the OpenSSL key
         */
        inline std::string ssl_key() { return _ssl_key; }

        /*!
         * Return the version string
         * \return Version string
         */
        static std::string version_string();

        /*!
         * Sets the secret for the generation JWT's (JSON Web Token). It must be a string
         * of length 32, since we're using currently SHA256 encoding.
         *
         * \param[in] jwt_secret_p String with 32 characters for the key for JWT's
         */
        void jwt_secret(const std::string &jwt_secret_p);

        /*!
         * Returns the secret used for JWT's
         *
         * \returns String of length 32 with the secret used for JWT's
         */
        inline std::string jwt_secret() { return _jwt_secret; }

        /*!
         * Returns the maximum number of parallel threads allowed
         *
         * \returns Number of parallel threads allowed
         */
        [[maybe_unused]] [[nodiscard]] inline unsigned nthreads() const { return _nthreads; }

        /*!
         * Return the path where to store temporary files (for uploads)
         *
         * \returns Path to directory for temporary files
         */
        [[maybe_unused]] inline std::string tmpdir() { return _tmpdir; }

        /*!
         * set the path to the  directory where to store temporary files during uploads
         *
         * \param[in] path to directory without trailing '/'
         */
        inline void tmpdir(const std::string &tmpdir_p) { _tmpdir = tmpdir_p; }

        /*!
        * Return the path where the Lua scripts for "Lua"-routes are found
        *
        * \returns Path to directory for script directory
        */
        [[maybe_unused]] inline std::string scriptdir() { return _scriptdir; }

        /*!
         * set the path to the  directory where to store temporary files during uploads
         *
         * \param[in] path to directory without trailing '/'
         */
        inline void scriptdir(const std::string &scriptdir_p) { _scriptdir = scriptdir_p; }

        /*!
         * Get the maximum size of a post request in bytes
         *
         * \returns Actual maximal size of  post request
         */
        [[nodiscard]] inline size_t max_post_size() const { return _max_post_size; }

        /*!
         * Set the maximal size of a post request
         *
         * \param[in] sz Maximal size of a post request in bytes
         */
        inline void max_post_size(size_t sz) { _max_post_size = sz; }

        /*!
        * Returns the routes defined for being handletd by Lua scripts
        *
        * \returns Vector of Lua _route infos
        */
        [[maybe_unused]] inline std::vector<cserve::LuaRoute> luaRoutes() { return _lua_routes; }

        /*!
         * set the routes that should be handled by Lua scripts
         *
         * \param[in] Vector of lua _route infos
         */
        inline void luaRoutes(const std::vector<cserve::LuaRoute> &lua_routes_p) { _lua_routes = lua_routes_p; }

        /*!
         * Run the server handling requests in an infinite loop
         */
        virtual void run();

        /*!
         * Set the default value for the keep alive timout. This is the time in seconds
         * a HTTP connection (socket) remains up without action before being closed by
         * the server. A keep-alive header will change this value
         */
        inline void keep_alive_timeout(int keep_alive_timeout) { _keep_alive_timeout = keep_alive_timeout; }

        /*!
         * Returns the default keep alive timeout
         *
         * \returns Keep alive timeout in seconds
         */
        [[maybe_unused]] [[nodiscard]] inline int keep_alive_timeout() const { return _keep_alive_timeout; }

        /*!
         * Sets the path to the initialization script (lua script) which is executed for each request
         *
         * \param[in] initscript_p Path of initialization script
         */
        inline void initscript(const std::string &initscript_p) {
            std::ifstream t(initscript_p);
            if (t.fail()) {
                throw Error(__FILE__, __LINE__, "initscript \"" + initscript_p + "\" not found!");
            }

            t.seekg(0, std::ios::end);
            _initscript.reserve(t.tellg());
            t.seekg(0, std::ios::beg);

            _initscript.assign((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
        }

        /*!
         * adds a function which is called before processing each request to initialize
         * special Lua variables and add special Lua functions
         *
         * \param[in] func C++ function which extends the Lua
         */
        inline void add_lua_globals_func(LuaSetGlobalsFunc func, void *user_data = nullptr) {
            GlobalFunc gf;
            gf.func = func;
            gf.func_dataptr = user_data;
            lua_globals.push_back(gf);
        }

        /*!
         * Add a request handler for the given request method and _route
         *
         * \param[in] method_p Request method (GET, PUT, POST etc.)
         * \param[in] path Route that this handler should serve
         * \param[in] handler_p Handler function which serves this method/_route combination.
         *            The handler has the form
         *
         *      void (*RequestHandler)(Connection::Connection &, void *);
         *
         * \param[in] handler_data_p Pointer to arbitrary data given to the handler when called
         *
         */
        void addRoute(Connection::HttpMethod method_p, const std::string &path, std::shared_ptr<RequestHandler> handler_p);


        /*!
         * Process a request... (Eventually should be private method)
         *
         * \param[in] sock Socket id
         * \param[in] peer_ip String containing IP (IP4 or IP6) of client/peer
         * \param[in] peer_port Port number of peer/client
         */
        ThreadStatus
        processRequest(std::istream *ins, std::ostream *os, std::string &peer_ip, int peer_port, bool secure,
                       int &keep_alive, bool socket_reuse = false);

        /*!
        * Return the user data that has been added previously
        */
        inline void *user_data() { return _user_data; }

        /*!
        * Add a pointer to user data which will be made available to the handler
        *
        * \param[in] User data
        */
        inline void user_data(void *user_data_p) { _user_data = user_data_p; }

        /*!
         * Stop the server gracefully (all destructors are called etc.) and the
         * cache file is updated. This function is asynchronous-safe, so it may be called
         * from within a signal handler.
         */
        inline void stop() {
            SocketControl::SocketInfo sockid(SocketControl::EXIT, SocketControl::STOP_SOCKET);
            (void) SocketControl::send_control_message(stoppipe[1], sockid);
            logger()->debug("Sent stop message to stoppipe[1]={}", stoppipe[1]);
        }


    };

}

#endif
