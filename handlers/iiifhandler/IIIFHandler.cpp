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
#include <regex>


#include "Global.h"
#include "Cserve.h"
#include "HttpSendError.h"
#include "IIIFHandler.h"

namespace cserve {

    const std::string IIIFHandler::_name = "iiifhandler";

    const std::string& IIIFHandler::name() const {
        return _name;
    }

    void IIIFHandler::handler(Connection &conn, LuaServer &lua, const std::string &route) {
        //
        // IIIF URi schema:
        // {scheme}://{server}{/pre/fix/...}/{identifier}/{region}/{size}/{rotation}/{quality}.{format}[?options]
        // {scheme}://{server}/{pre/fix/...}/{identifier}/info.json
        // {scheme}://{server}/{pre/fix/...}/{identifier}/knora.json
        // {scheme}://{server}/{pre/fix/...}/{identifier}" -> redirect to {scheme}://{server}/{pre/fix}/{identifier}/info.json
        // {scheme}://{server}/{pre/fix/...}/{identifier}/file" -> serve as blob
        //


        std::vector<std::string> headers = conn.header();
        std::string uri = conn.uri();

        std::vector<std::string> parts;
        {
            std::vector<std::string> tmpparts = split(uri, '/');
            for (auto &part: tmpparts) {
                parts.push_back(part);
            }
        }

        //
        // below are regex expressions for the different parts of the IIIF URL
        //
        std::string qualform_ex = "^(color|gray|bitonal|default)\\.(jpg|tif|png|jp2)$";
        std::string rotation_ex = "^!?[-+]?[0-9]*\\.?[0-9]*$";
        std::string size_ex = "^(\\^?max)|(\\^?pct:[0-9]*\\.?[0-9]*)|(\\^?[0-9]*,)|(\\^?,[0-9]*)|(\\^?!?[0-9]*,[0-9]*)$";
        std::string region_ex = "^(full)|(square)|([0-9]+,[0-9]+,[0-9]+,[0-9]+)|(pct:[0-9]*\\.?[0-9]*,[0-9]*\\.?[0-9]*,[0-9]*\\.?[0-9]*,[0-9]*\\.?[0-9]*)$";


        std::unordered_map<Parts,std::string> iiif_str_params;
        bool options_ok = false;
        bool format_ok = false;
        bool quality_ok = false;
        bool rotation_ok = false;
        bool size_ok = false;
        bool region_ok = false;
        bool info_ok = false;
        bool file_ok = false;
        int partspos = parts.size() - 1;

        if (partspos < 0) {
            send_error(conn, Connection::BAD_REQUEST, "Empty path not allowed for IIIF request.");
            return;
        }
        size_t pos = parts[parts.size() - 1].find('?');
        std::string tmpstr;

        if (partspos < 0) {
            send_error(conn, Connection::BAD_REQUEST, "Empty path not allowed for IIIF request");
            return;
        }
        if (pos != std::string::npos) {
            options_ok = true;
            tmpstr = parts[partspos].substr(0, pos);
            iiif_str_params[IIIF_OPTIONS] = parts[partspos].substr(pos + 1, std::string::npos);
        }
        else {
            tmpstr = parts[partspos];
        }
        if (tmpstr == "file") {
            file_ok = true;
            partspos--;
            if (partspos < 0) {
                send_error(conn, Connection::BAD_REQUEST, "Invalid IIIF URL. Parts before '/file' are missing.");
                return;
            }
        }
        else {
            format_ok = quality_ok = std::regex_match(tmpstr, std::regex(qualform_ex));
            if (format_ok && quality_ok) {
                pos = tmpstr.rfind('.');
                if (pos != std::string::npos) {
                    std::string filename_body(tmpstr.substr(0, pos));
                    iiif_str_params[IIIF_QUALITY] = tmpstr.substr(0, pos);
                    iiif_str_params[IIIF_FORMAT] = tmpstr.substr(pos + 1, std::string::npos);
                }
                partspos--;
                if (partspos < 0) {
                    send_error(conn, Connection::BAD_REQUEST, "Invalid IIIF URL. Parts before 'quality.format' missing.");
                    return;
                }
                rotation_ok = std::regex_match(parts[partspos], std::regex(rotation_ex));
                if (!rotation_ok) {
                    std::string errormsg = fmt::format("IIIF rotation parameter '{}' not valid.", parts[partspos]);
                    send_error(conn, Connection::BAD_REQUEST, errormsg);
                    return;
                }
                iiif_str_params[IIIF_ROTATION] = parts[partspos];
                partspos--;
                if (partspos < 0) {
                    send_error(conn, Connection::BAD_REQUEST, "Invalid IIIF URL. Parts before 'rotation' missing.");
                    return;
                }
                size_ok = std::regex_match(parts[partspos], std::regex(size_ex));
                if (!size_ok) {
                    std::string errormsg = fmt::format("IIIF size parameter '{}' not valid.", parts[partspos]);
                    send_error(conn, Connection::BAD_REQUEST, errormsg);
                    return;
                }
                iiif_str_params[IIIF_SIZE] = parts[partspos];
                partspos--;
                if (partspos < 0) {
                    send_error(conn, Connection::BAD_REQUEST, "Invalid IIIF URL. Parts befire 'size' are missing.");
                    return;
                }
                region_ok = std::regex_match(parts[partspos], std::regex(region_ex));
                if (!region_ok) {
                    std::string errormsg = fmt::format("IIIF region parameter '{}' not valid.", parts[partspos]);
                    send_error(conn, Connection::BAD_REQUEST, errormsg);
                    return;
                }
                iiif_str_params[IIIF_REGION] = parts[partspos];
                partspos--;
                if (partspos < 0) {
                    send_error(conn, Connection::BAD_REQUEST, "Invalid IIIF URL. Identifier is missing.");
                    return;
                }
            }
            else if (tmpstr == "info.json") {
                info_ok = true;
                partspos--;
                if (partspos < 0) {
                    send_error(conn, Connection::BAD_REQUEST, "Invalid IIIF URL. Parts before '/file' are missing.");
                    return;
                }
            }
        }
        iiif_str_params[IIIF_IDENTIFIER] = urldecode(parts[partspos]);
        if (partspos > 0) { // we have a prefix
            std::stringstream prefix;
            for (int i = 0; i < partspos; i++) {
                if (i > 0) prefix << "/";
                prefix << urldecode(parts[i]);
            }
            iiif_str_params[IIIF_PREFIX] = prefix.str();
        }

        try {
            conn.header("Content-Type", "text/text; charset=utf-8");
            conn.setBuffer();
            conn << "International Image Interoperability Framework (IIIF)" << Connection::endl;
            if (file_ok) {
                conn << ">>>Send file as blob" << Connection::endl;
                conn << "prefix=" << iiif_str_params[IIIF_PREFIX] << Connection::endl;
                conn << "file=" << iiif_str_params[IIIF_IDENTIFIER] << Connection::endl;
            }
            else if (info_ok) {
                conn << ">>>Send 'info.json'" << Connection::endl;
                conn << "prefix=" << iiif_str_params[IIIF_PREFIX] << Connection::endl;
                conn << "file=" << iiif_str_params[IIIF_IDENTIFIER] << Connection::endl;
            } else if (format_ok && quality_ok && region_ok && size_ok && rotation_ok) {
                conn << ">>>Send IIIF image" << Connection::endl;
                conn << "prefix=" << iiif_str_params[IIIF_PREFIX] << Connection::endl;
                conn << "id=" << iiif_str_params[IIIF_IDENTIFIER] << Connection::endl;
                conn << "region=" << iiif_str_params[IIIF_REGION] << Connection::endl;
                conn << "size=" << iiif_str_params[IIIF_SIZE] << Connection::endl;
                conn << "rotation=" << iiif_str_params[IIIF_ROTATION] << Connection::endl;
                conn << "quailty=" << iiif_str_params[IIIF_QUALITY] << Connection::endl;
                conn << "formar=" << iiif_str_params[IIIF_FORMAT] << Connection::endl;
            } else {
                conn << ">>>Send UNKNOWN!" << Connection::endl;
            }
            conn << Connection::flush_data;
        }
        catch (InputFailure &err) {
            Server::logger()->debug("conn write error: {}", err.what());
            return;
        }
    }

    void IIIFHandler::set_config_variables(CserverConf &conf) {
        std::vector<RouteInfo> routes = {
                RouteInfo("GET:/iiif:C++"),
        };
        conf.add_config(_name, "routes",routes, "Route for iiifhandler");
        conf.add_config(_name, "imgroot", "./imgroot", "Root directory containing the images for the IIIF server.");
        conf.add_config(_name, "max_tmp_age", 86400, "The maximum allowed age of temporary files (in seconds) before they are deleted.");
        conf.add_config(_name, "path_prefix", false, "Flag, if set indicates that the IIIF prefix is part of the path to the image file (deprecated).");
        conf.add_config(_name, "cachedir", "./cache", "Path to cache folder.");
        conf.add_config(_name, "cachesize", DataSize("200MB"), "Maximal size of cache, e.g. '500M'.");
        conf.add_config(_name, "preflight_name", "pre_flight", "Name of the preflight lua function.");
        conf.add_config(_name, "max_num_cache_files", 200, "The maximal number of files to be cached.");
        conf.add_config(_name, "cache_hysteresis", 0.15f, "If the cache becomes full, the given percentage of file space is marked for reuse (0.0 - 1.0).");
        conf.add_config(_name, "thumbsize", "!128,128", "Size of the thumbnails (to be used within Lua).");
    }

    void IIIFHandler::get_config_variables(const CserverConf &conf) {
        _imgroot = conf.get_string("imgroot").value_or("-- no imgroot --");
        _max_tmp_age = conf.get_int("max_tmp_age").value_or(86400);
        _path_prefix = conf.get_bool("path_prefix").value_or(false);
        _cachedir = conf.get_string("cachedir").value_or("./cache");
        _cache_size = conf.get_datasize("cachesize").value_or(DataSize("200MB"));
        _pre_flight_func_name = conf.get_string("preflight_name").value_or("pre_flight");
        _max_num_chache_files = conf.get_int("max_num_cache_files").value_or(200);
        _cache_hysteresis = conf.get_float("cache_hysteresis").value_or(0.15f);
        _thumbnail_size = conf.get_string("thumbsize").value_or("!128,128");
    }

}

extern "C" cserve::IIIFHandler * create_iiifhandler() {
    return new cserve::IIIFHandler();
};

extern "C" void destroy_iiifhandler(cserve::IIIFHandler *handler) {
    delete handler;
}
