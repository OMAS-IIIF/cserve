//
// Created by Lukas Rosenthaler on 08.07.22.
//

#include "IIIFHandler.h"
#include "iiifparser/IIIFIdentifier.h"

namespace cserve {

    std::unordered_map<std::string, std::string> IIIFHandler::check_file_access(Connection &conn_obj,
                                                                                LuaServer &luaserver,
                                                                                std::vector<std::string> &params,
                                                                                bool prefix_as_path)
    {
        std::unordered_map<std::string, std::string> pre_flight_info;
        /*
        std::string infile;

        IIIFIdentifier sid(params[IIIF_IDENTIFIER]);
        if (luaserver.luaFunctionExists(_pre_flight_func_name))
        {
            pre_flight_info = call_pre_flight(conn_obj, luaserver, urldecode(params[iiif_prefix]), sid.getIdentifier()); // may throw SipiError
            infile = pre_flight_info["infile"];
        }
        else
        {
            if (prefix_as_path)
            {
                infile = serv->imgroot() + "/" + urldecode(params[iiif_prefix]) + "/" + sid.getIdentifier();
            }
            else
            {
                infile = serv->imgroot() + "/" + urldecode(sid.getIdentifier());
            }
            pre_flight_info["type"] = "allow";
        }
        //
        // test if we have access to the file
        //
        if (access(infile.c_str(), R_OK) != 0)
        { // test, if file exists
            throw SipiError(__file__, __LINE__, "Cannot read image file: " + infile);
        }
        pre_flight_info["infile"] = infile;
         */
        return pre_flight_info;
    }

}