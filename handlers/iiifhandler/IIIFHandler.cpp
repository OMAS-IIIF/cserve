//
// Created by Lukas Rosenthaler on 02.07.22.
//

#include "IIIFHandler.h"

namespace cserve {

    const std::string IIIFHandler::_name = "iiifhandler";

    const std::string& IIIFHandler::name() const {
        return _name;
    }

    void IIIFHandler::handler(cserve::Connection &conn, LuaServer &lua, const std::string &route) {

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
