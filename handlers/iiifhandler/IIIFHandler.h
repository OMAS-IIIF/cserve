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
#ifndef CSERVER_IIIFHANDLER_H
#define CSERVER_IIIFHANDLER_H

#include <string>

#include "../../lib/LuaServer.h"
#include "../../lib/RequestHandlerData.h"
#include "../../lib/RequestHandler.h"

namespace cserve {

    enum Parts {
        IIIF_PREFIX = 0,
        IIIF_IDENTIFIER = 1,
        IIIF_REGION = 2,
        IIIF_SIZE = 3,
        IIIF_ROTATION = 4,
        IIIF_QUALITY = 5,
        IIIF_FORMAT = 6,
        IIIF_OPTIONS = 7
    };

    class IIIFHandler: public RequestHandler {
    private:
        const static std::string _name;
        std::string _imgroot;
        std::string _cachedir;
        std::string _pre_flight_func_name;
        int _max_tmp_age;
        bool _path_prefix;
        DataSize _cache_size;
        int _max_num_chache_files;
        float _cache_hysteresis;
        std::string _thumbnail_size;
    public:
        [[maybe_unused]] IIIFHandler() : RequestHandler() {}

        const std::string& name() const override;

        void handler(Connection& conn, LuaServer &lua, const std::string &route) override;

        void set_config_variables(CserverConf &conf) override;

        void get_config_variables(const CserverConf &conf) override;

        std::unordered_map<std::string, std::string>
        call_pre_flight(Connection &conn_obj,
                        LuaServer &luaserver,
                        const std::string &prefix,
                        const std::string &identifier);

        std::unordered_map<std::string, std::string> check_file_access(Connection &conn_obj,
                                                                       LuaServer &luaserver,
                                                                       std::vector<std::string> &params,
                                                                       bool prefix_as_path);

    };

}



#endif //CSERVER_IIIFHANDLER_H
