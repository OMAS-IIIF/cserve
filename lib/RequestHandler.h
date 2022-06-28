//
// Created by Lukas Rosenthaler on 07.06.22.
//

#ifndef CSERVER_REQUESTHANDLER_H
#define CSERVER_REQUESTHANDLER_H

#include "Connection.h"
#include "LuaServer.h"
#include "CserverConf.h"

namespace cserve {

    /*!
     * Abstract class for defining a request handler
     */
    class RequestHandler {
    private:
        std::string _route;
        std::string basepath;
    public:

        /*!
         * Default constructur
         */
        RequestHandler() = default;
        
        /*!
         * Default destructor
         */
        virtual ~RequestHandler() = default;

        virtual const std::string& name() const = 0;

        inline void set_route(const std::string &route_p) { _route = route_p; }

        inline std::string get_route() { return _route; }

        /*!
         * Pure virtual function that gives the template for implementing a request handler
         * @param conn Connection reference
         * @param lua Lua interpreter
         * @param user_data Additional unspecified data that can be given to the handler
         */
        virtual void handler(Connection& conn, LuaServer& lua, void* user_data) = 0;

        virtual inline void set_config_variables(CserverConf &conf) {}

        virtual inline void get_config_variables(const CserverConf &conf) {}
    };

} // cserve

#endif //CSERVER_REQUESTHANDLER_H
