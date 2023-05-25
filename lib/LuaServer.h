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
#ifndef _cserve_lua_server_h
#define _cserve_lua_server_h

#include <iostream>
#include <vector>
#include <variant>
#include <map>
#include <unordered_map>
#include <variant>
#include <stdexcept>
#include <memory>
#include <spdlog/common.h>
#include <nlohmann/json.hpp>
#include "NlohmannTraits.h"


#include "Error.h"
#include "Connection.h"
#include "Global.h"
#include "lua.hpp"


namespace cserve {


    /*!
     * Implementation of a C representation for Lua values which may be nested tables and arrays
     */
    class LuaValstruct {
    public:
        /*!
         * Supported data types
         */
        enum LuaValType {
            INT_TYPE, FLOAT_TYPE, STRING_TYPE, BOOLEAN_TYPE, ARRAY_TYPE, TABLE_TYPE, UNDEFINED_TYPE
        };
    private:
        LuaValType type;
        int i;
        float f;
        std::string s;
        bool b;
        std::vector<LuaValstruct> array;
        std::unordered_map<std::string, LuaValstruct> table;
    public:
        /*!
         * Default constructor
         */
        explicit LuaValstruct() : i(0), f(0.0F), b(false), type(UNDEFINED_TYPE) {}

        /*!
         * Constructor for integer
         * @param i Integer value
         */
        explicit LuaValstruct(int i) : i(i), f(0.0F), b(false), type(INT_TYPE) {}

        /*!
         * Constructor for float values
         * @param f Float value
         */
        explicit LuaValstruct(float f): f(f), i(0), b(false), type(FLOAT_TYPE) {}

        /*!
         * Constructor for strings
         * @param str String value
         */
        explicit LuaValstruct(const std::string &str) : s(str), i(0), f(0.0F), b(false), type(STRING_TYPE) {}

        /*!
         * Constructor for booleans
         * @param b Boolean value
         */
        explicit LuaValstruct(bool b) : b(b), i(0), f(0.0F), type(BOOLEAN_TYPE) {}

        /*!
         * Constructor for vector of LuaValstructs (aka "array")
         * @param v Vector of LuaValstructs
         */
        explicit LuaValstruct(const std::vector<LuaValstruct> &v) : array(v), i(0), f(0.0F), b(false), type(ARRAY_TYPE) {}

        /*!
         * Constructor for table of LuaValstructs. Indices must be of std::string type!
         * @param table Unordered map of LuaValstructs
         */
        explicit LuaValstruct(const std::unordered_map<std::string, LuaValstruct> &table) : table(table), i(0), f(0.0F), b(false), type(TABLE_TYPE) {}

        /*!
         * Copy constructor
         * @param lv
         */
        LuaValstruct(const LuaValstruct &lv);

        /*!
         * Move constructor
         * @param lv
         */
        LuaValstruct(LuaValstruct &&lv) noexcept;

        /*!
         * Assignment constructor with copy
         * @param lv
         * @return
         */
        LuaValstruct& operator=(const LuaValstruct &lv);

        /*!
         * Assigment constructor with move
         * @param lv
         * @return
         */
        LuaValstruct& operator=(LuaValstruct &&lv) noexcept;

        /*!
         * Test if LuaValstruct is defined
         * @return True, if is defined
         */
        auto is_undefined() const  {return (type == UNDEFINED_TYPE); }

        /*!
         * Get the data type represented by the LuaValstruct
         * @return A LuaValType
         */
        auto get_type() const { return type; }

        /*!
         * Get an optional integer
         * @return std::optional
         */
        auto get_int() const { return (type == INT_TYPE) ? std::optional<int>{i} : std::nullopt; }

        /*!
         * Get an optional float
         * @return std::optional
         */
        auto get_float() const { return (type == FLOAT_TYPE) ? std::optional<float>{i} : std::nullopt; }

        /*!
         * Get an optional std::string
         * @return std::optional
         */
        auto get_string() const { return (type == STRING_TYPE) ? std::optional<std::string>{s} : std::nullopt; }

        /*!
         * Get an optional boolean
         * @return std::optional
         */
        auto get_boolean() const { return (type == BOOLEAN_TYPE) ? std::optional<bool>{b} : std::nullopt; }

        /*!
         * Get an optional array aka std::vector
         * @return std::optional
         */
        auto get_array() const { return (type == ARRAY_TYPE) ? std::optional<std::vector<LuaValstruct>>{array} : std::nullopt; }

        /*!
         * Get an optional table aka std::unordered_map
         * @return std::optional
         */
        auto get_table() const { return (type == TABLE_TYPE) ? std::optional<std::unordered_map<std::string, LuaValstruct>>{table} : std::nullopt; }

        /*!
         * Return a nlohmann::json object representing the LuaValstruct
         * @param vs A LuaValstruct instance
         * @return nlohmann::json instance
         */
        nlohmann::json get_json(const LuaValstruct &vs);
    };

    typedef struct RouteInfo {
        Connection::HttpMethod method;
        std::string route;
        std::string additional_data;
        RouteInfo() = default;
        explicit RouteInfo(const std::string &lua_route_str);
        [[nodiscard]] std::string method_as_string() const;
        [[nodiscard]] std::string to_string() const;
        inline bool empty() { return route.empty() || additional_data.empty(); }
        inline bool operator==(const RouteInfo &lr) const { return method == lr.method && route == lr.route && additional_data == lr.additional_data; }
        inline friend std::ostream &operator<<(std::ostream &os, const RouteInfo &rhs) { return os << "ROUTE METHOD=" << rhs.method << " ROUTE=" << rhs.route << " SCRIPT=" << rhs.additional_data; };
    } LuaRoute;

    // [[maybe_unused]] typedef std::unordered_map<std::string, LuaValstruct> LuaKeyValStore;

    typedef void (*LuaSetGlobalsFunc)(lua_State *L, cserve::Connection &, void *);

    extern char luaconnection[];

    class LuaServer {
    private:
        lua_State *L{};
        //std::vector<LuaSetGlobalsFunc> setGlobals;

    public:
        /*!
         * Instantiates a lua interpreter
         */
        LuaServer();

        /*!
         * Instantiates a lua interpreter which has access to the HTTP connection
         *
         * \param[in] conn HTTP Connection object
         */
        explicit LuaServer(Connection &conn);


        /*!
         * Instantiates a lua interpreter an executes the given lua script
         *
         * \param[in] luafile A script containing lua commands
         * \param[in] iscode If true, the string contains lua-code to be executed directly
         */
        explicit LuaServer(const std::string &luafile, bool iscode = false);

        /*!
         * Instantiates a lua interpreter an executes the given lua script
         *
         * \param[in] conn HTTP Connection object
         * \param[in] luafile A script containing lua commands
         * \param[in] iscode If true, the string contains lua-code to be executed directly
         * \param[in] lua_scriptdir Pattern to be added to the Lua package.path (directory with Lua scripts)
         */
        LuaServer(Connection &conn, const std::string &luafile, bool iscode, const std::string &lua_scriptdir);

        /*!
         * Copy constructor not allowed!
         */
        LuaServer(const LuaServer &other) = delete;

        /*!
         * Assignment operator not allowed!
         */
        inline LuaServer &operator=(const LuaServer &other) = delete;

        /*!
         * Destroys the lua interpreter and all associated resources
         */
        ~LuaServer();

        /*!
         * Getter for the Lua state
         */
        inline lua_State *lua() { return L; }

        /*!
         * Adds a value to the server tabe.
         *
         * The server table contains constants and functions which are related
         * to the cserve HTTP server. The server table a a globally accessible
         * table.
         *
         * \param[in] name Name of variable
         * \param[in] value Value (only String allowed)
         */
        void add_servertableentry(const std::string &name, const std::string &value);

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

        std::vector<RouteInfo> configRoute(const std::string& table, const std::string& variable, const std::vector<cserve::RouteInfo> &defval);

        /*!
        * Add path to the lua package path for "require"
        *
        * \param[in] path The path
        */
        void setLuaPath(const std::string &path);


        /*!
         * Create the global values and functions
         *
         * \param[in] conn HTTP connection object
         */
        void createGlobals(Connection &conn);



        /*!
         * Execute a chunk of Lua code
         *
         * \param[in] luastr String containing the Lua code
         * \param[in] scriptname String containing the Lua script name
         * \returns Either the value 1 or an integer result that the Lua code provides
         */
        int executeChunk(const std::string &luastr, const std::string &scriptname);

        /*!
         * Executes a Lua function that either is defined in C or in Lua
         *
         * \param[in] funcname the name of the function to be called
         * \param[in] lvals vector of parameters to be passed to the function
         * \returns vector of LuaValstruct containing the result of the execution of the lua function
         */
        [[maybe_unused]] std::vector<LuaValstruct> executeLuafunction(const std::string &funcname, const std::vector<LuaValstruct>& lvals);

        /*!
         * Executes a Lua function that either is defined in C or in Lua
         *
         * \param[in] funcname Name of the function
         * \param[in] n Number of arguments
         * \returns true if function with given name exists
         */
        [[maybe_unused]] bool luaFunctionExists(const std::string &funcname);
    };

}

#endif
