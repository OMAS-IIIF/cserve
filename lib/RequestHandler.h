//
// Created by Lukas Rosenthaler on 07.06.22.
//

#ifndef CSERVER_REQUESTHANDLER_H
#define CSERVER_REQUESTHANDLER_H

#include "Connection.h"
#include "LuaServer.h"

namespace cserve {

    /*!
     * Abstract class for defining a request handler
     */
    class RequestHandler {
        std::string route;
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

        inline virtual std::string name() { return std::string("RequestHandler"); }

        inline void set_route(const std::string &route_p) { route = route_p; }

        inline std::string get_route() { return route; }

        /*!
         * Pure virtual function that gives the template for implementing a request handler
         * @param conn Connection reference
         * @param lua Lua interpreter
         * @param user_data Additonal unspecified data that cann be given to the handler
         */
        virtual void handler(Connection& conn, LuaServer& lua, void* user_data) = 0;
    };

} // cserve

#endif //CSERVER_REQUESTHANDLER_H
