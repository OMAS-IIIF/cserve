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

#include "IIIFCache.h"
#include "IIIFImage.h"
#include "iiifparser/IIIFRotation.h"
#include "iiifparser/IIIFQualityFormat.h"

namespace cserve {

    enum Parts {
        IIIF_ROUTE = 0,
        IIIF_PREFIX = 1,
        IIIF_IDENTIFIER = 2,
        IIIF_REGION = 3,
        IIIF_SIZE = 4,
        IIIF_ROTATION = 5,
        IIIF_QUALITY = 6,
        IIIF_FORMAT = 7,
        IIIF_OPTIONS = 8
    };

    class IIIFHandler: public RequestHandler {
    private:
        const static std::string _name;
        std::string _scriptdir;
        std::string _imgroot;
        std::string _cachedir;
        std::string _iiif_preflight_funcname;
        std::string _file_preflight_funcname;
        std::vector<std::string> _iiif_specials;
        int _max_tmp_age;
        bool _prefix_as_path;
        DataSize _cache_size;
        int _max_num_chache_files;
        float _cache_hysteresis;
        std::string _thumbnail_size;
        int _jpeg_quality;
        ScalingQuality _scaling_quality;
        size_t _iiif_max_image_width;
        size_t _iiif_max_image_height;

        std::shared_ptr<IIIFCache> _cache;
    public:
        /**
         * Initializes the libraries the IIF handler needs
         */
        [[maybe_unused]]
        IIIFHandler();

        [[nodiscard]]
        const std::string& name() const override;

        void handler(Connection& conn, LuaServer &lua, const std::string &route) override;

        void set_config_variables(CserverConf &conf) override;

        void get_config_variables(const CserverConf &conf) override;

        void set_lua_globals(lua_State *L, cserve::Connection &conn) override;

        std::unordered_map<std::string, std::string> call_iiif_preflight(Connection &conn_obj,
                                                                         LuaServer &luaserver,
                                                                         const std::string &prefix,
                                                                         const std::string &identifier) const;

        std::unordered_map<std::string, std::string> call_blob_preflight(Connection &conn_obj,
                                                                         LuaServer &luaserver,
                                                                         const std::string &filepath) const;

        std::unordered_map<std::string, std::string> check_file_access(Connection &conn_obj,
                                                                       LuaServer &luaserver,
                                                                       const std::unordered_map<Parts,std::string> &params,
                                                                       bool prefix_as_path) const;

        void send_iiif_info(Connection &conn_obj, LuaServer &luaserver, const std::unordered_map<Parts,std::string> &params) const;

        void send_iiif_file(Connection &conn_obj, LuaServer &luaserver, const std::unordered_map<Parts,std::string> &params) const;

        void send_iiif_blob(Connection &conn_obj, LuaServer &luaserver, const std::unordered_map<Parts,std::string> &params) const;

        void send_iiif_special(Connection &conn_obj,
                               LuaServer &luaserver,
                               const std::unordered_map<Parts,std::string> &params,
                               const std::string &lua_function_name) const;

        inline std::shared_ptr<IIIFCache> cache() const { return _cache; }

        std::pair<std::string, std::string> get_canonical_url  (
                uint32_t tmp_w,
                uint32_t tmp_h,
                const std::string &host,
                const std::string &prefix,
                const std::string &identifier,
                const std::shared_ptr<IIIFRegion>& region,
                const std::shared_ptr<IIIFSize>& size,
                IIIFRotation &rotation,
                IIIFQualityFormat &quality_format) const;

    };

}



#endif //CSERVER_IIIFHANDLER_H
