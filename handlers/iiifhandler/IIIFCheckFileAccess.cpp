//
// Created by Lukas Rosenthaler on 08.07.22.
//

#include <unistd.h>

#include "IIIFHandler.h"
#include "IIIFError.h"
#include "iiifparser/IIIFIdentifier.h"

static const char file_[] = __FILE__;

namespace cserve {

    std::unordered_map<std::string, std::string> IIIFHandler::check_file_access(Connection &conn_obj,
                                                                                LuaServer &luaserver,
                                                                                const std::unordered_map<Parts,std::string> &params,
                                                                                bool prefix_as_path) const {
        std::unordered_map<std::string, std::string> pre_flight_info;
        std::string infile;

        IIIFIdentifier sid(params.at(IIIF_IDENTIFIER));
        if (luaserver.luaFunctionExists(_pre_flight_func_name)) {
            pre_flight_info = call_pre_flight(conn_obj, luaserver, params.at(IIIF_PREFIX), sid.get_identifier()); // may throw SipiError
            infile = pre_flight_info["infile"];
        }
        else {
            if (prefix_as_path) {
                infile = _imgroot + "/" + params.at(IIIF_PREFIX) + "/" + sid.get_identifier();
            }
            else {
                infile = _imgroot + "/" + sid.get_identifier();
            }
            pre_flight_info["type"] = "allow";
        }
        //
        // test if we have access to the file
        //
        if (access(infile.c_str(), R_OK) != 0) {
            throw IIIFError(file_, __LINE__, "Cannot read image file: " + infile);
        }
        pre_flight_info["infile"] = infile;
        return pre_flight_info;
    }

}