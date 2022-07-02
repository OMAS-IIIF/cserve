//
// Created by Lukas Rosenthaler on 02.07.22.
//

#ifndef CSERVER_IIIFHANDLER_H
#define CSERVER_IIIFHANDLER_H

#include <string>

#include "../../lib/LuaServer.h"
#include "../../lib/RequestHandlerData.h"
#include "../../lib/RequestHandler.h"

namespace cserve {

    class IIIFHandler: public RequestHandler {
    private:
        const static std::string _name;
        std::string _imgroot;
        std::string _cachedir;
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

    };

}



#endif //CSERVER_IIIFHANDLER_H
