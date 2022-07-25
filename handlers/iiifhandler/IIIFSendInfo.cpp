//
// Created by Lukas Rosenthaler on 07.07.22.
//

#include <sys/stat.h>
#include "Parsing.h"
#include "HttpSendError.h"
#include "IIIFHandler.h"
#include "IIIFError.h"
#include "iiifparser/IIIFIdentifier.h"
#include "nlohmann/json.hpp"

static const char file_[] = __FILE__;

namespace cserve {

    void IIIFHandler::send_iiif_info(Connection &conn, LuaServer &luaserver,
                                     const std::unordered_map<Parts, std::string> &params) const {
        Connection::StatusCodes http_status = Connection::StatusCodes::OK;
        std::unordered_map<std::string, std::string> access;
        try {
            access = check_file_access(conn, luaserver, params, _prefix_as_path);
        }
        catch (IIIFError &err) {
            send_error(conn, Connection::INTERNAL_SERVER_ERROR, err);
            return;
        }

        std::string actual_mimetype = Parsing::getBestFileMimetype(access["infile"]);
        bool is_image_file = ((actual_mimetype == "image/tiff") ||
                              (actual_mimetype == "image/jpeg") ||
                              (actual_mimetype == "image/png") ||
                              (actual_mimetype == "image/jpx") ||
                              (actual_mimetype == "image/jp2") ||
                              (actual_mimetype == "application/pdf"));

        nlohmann::json root_obj = {
                {"@context",
                 is_image_file ? "http://iiif.io/api/image/3/context.json" : "http://sipi.io/api/file/3/context.json"},
        };

        IIIFIdentifier sid = IIIFIdentifier(params.at(IIIF_IDENTIFIER));
        int pagenum = sid.get_page();

        std::string host = conn.header("host");
        std::string id;
        if (params.at(IIIF_PREFIX) == "") {
            if (conn.secure()) {
                id = std::string("https://") + host + "/" + params.at(IIIF_IDENTIFIER);
            } else {
                id = std::string("http://") + host + "/" + params.at(IIIF_IDENTIFIER);
            }
        } else {
            if (conn.secure()) {
                id = std::string("https://") + host + "/" + params.at(IIIF_PREFIX) + "/" + params.at(IIIF_IDENTIFIER);
            } else {
                id = std::string("http://") + host + "/" + params.at(IIIF_PREFIX) + "/" + params.at(IIIF_IDENTIFIER);
            }
        }
        root_obj["id"] = id;

        if (is_image_file) {
            root_obj["type"] = "ImageService3";
            root_obj["protocol"] = "http://iiif.io/api/image";
            root_obj["profile"] = "level2";
        } else {
            root_obj["internalMimeType"] = actual_mimetype;

            struct stat fstatbuf;
            if (stat(access["infile"].c_str(), &fstatbuf) != 0) {
                throw IIIFError(file_, __LINE__, "Cannot fstat file!");
            }
            root_obj["fileSize"] = fstatbuf.st_size;
        }

        //
        // IIIF Authentication API stuff
        //
        if ((access["type"] == "login") || (access["type"] == "clickthrough") || (access["type"] == "kiosk") || (access["type"] == "external")) {
            nlohmann::json service;
            try {
                service["@context"] = "http://iiif.io/api/auth/1/context.json";
                std::string cookieUrl = access.at("cookieUrl");
                service["@id"] = cookieUrl;
                if (access["type"] == "login") {
                    service["profile"] = "http://iiif.io/api/auth/1/login";
                }
                else if (access["type"] == "clickthrough") {
                    service["profile"] = "http://iiif.io/api/auth/1/clickthrough";
                }
                else if (access["type"] == "kiosk") {
                    service["profile"] = "http://iiif.io/api/auth/1/kiosk";
                }
                else if (access["type"] == "external") {
                    service["profile"] = "http://iiif.io/api/auth/1/external";
                }
                for (auto &item : access) {
                    if (item.first == "cookieUrl")
                        continue;
                    if (item.first == "tokenUrl")
                        continue;
                    if (item.first == "logoutUrl")
                        continue;
                    if (item.first == "infile")
                        continue;
                    if (item.first == "type")
                        continue;
                    service[item.first] = item.second;
                }
                std::string tokenUrl{};
                try {
                    tokenUrl = access.at("tokenUrl");
                }
                catch (const std::out_of_range &err) {
                    send_error(conn, Connection::INTERNAL_SERVER_ERROR, "Pre_flight_script has login type but no tokenUrl!");
                    return;
                }
                std::string logoutUrl{};
                try {
                    std::string logoutUrl = access.at("logoutUrl");
                }
                catch (const std::out_of_range &err) {}
                if (logoutUrl.empty()) {
                    service["service"] = {
                            {
                                    {"@id", tokenUrl},
                                    {"profile", "http://iiif.io/api/auth/1/token"}
                            }
                    };
                }
                else {
                    service["service"] = {
                            {
                                    {"@id", tokenUrl},
                                    {"profile", "http://iiif.io/api/auth/1/token"}
                            },
                            {
                                    {"@id", logoutUrl},
                                    {"profile", "http://iiif.io/api/auth/1/logout"}
                            }
                    };
                }
            }
            catch (const std::out_of_range &err) {
                send_error(conn, Connection::INTERNAL_SERVER_ERROR, "Pre_flight_script has login type but no cookieUrl!");
                return;
            }
            root_obj["service"] = service;
            http_status = Connection::StatusCodes::UNAUTHORIZED;
        }

        if (is_image_file) {
            size_t width, height;
            size_t t_width, t_height;
            int clevels;
            int numpages = 0;

            if (!_cache || !_cache->getSize(access["infile"], width, height, t_width, t_height, clevels, pagenum)) {

            }

        }
        conn.status(http_status);
        conn.setBuffer(); // we want buffered output, since we send JSON text...

        conn.header("Access-Control-Allow-Origin", "*");
        const std::string contenttype = conn.header("accept");
        if (is_image_file) {
            if (!contenttype.empty() && (contenttype == "application/ld+json")) {
                conn.header("Content-Type", "application/ld+json;profile=\"http://iiif.io/api/image/3/context.json\"");
            }
            else {
                conn.header("Content-Type", "application/json");
                conn.header("Link",
                            R"(<http://iiif.io/api/image/3/context.json>; rel="http://www.w3.org/ns/json-ld#context"; type="application/ld+json")");
            }
        }
        else {
            if (!contenttype.empty() && (contenttype == "application/ld+json")) {
                conn.header("Content-Type", "application/ld+json;profile=\"http://sipi.io/api/file/3/context.json\"");
            }
            else {
                conn.header("Content-Type", "application/json");
                conn.header("Link",
                            R"(<http://sipi.io/api/file/3/context.json>; rel="http://www.w3.org/ns/json-ld#context"; type="application/ld+json")");
            }
        }

        std::string json_str = root_obj.dump(3);
        conn.sendAndFlush(json_str.c_str(), json_str.size());
   };
}
