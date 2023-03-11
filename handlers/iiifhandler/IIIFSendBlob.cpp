//
// Created by Lukas Rosenthaler on 10.03.23.
//

#include <sys/stat.h>
#include <filesystem>
#include <unistd.h>

#include "Cserve.h"
#include "Parsing.h"
#include "HttpSendError.h"
#include "IIIFHandler.h"
#include "IIIFError.h"
#include "iiifparser/IIIFIdentifier.h"
#include "nlohmann/json.hpp"
#include "IIIFImage.h"

static const char file_[] = __FILE__;

namespace cserve {
    void IIIFHandler::send_iiif_blob(Connection &conn, LuaServer &luaserver,
                                     const std::unordered_map<Parts, std::string> &params) const {

        std::filesystem::path infile{_imgroot};
        if (_prefix_as_path) {
            infile /= params.at(IIIF_PREFIX);
            infile /= params.at(IIIF_IDENTIFIER);
        } else {
            infile /= params.at(IIIF_IDENTIFIER);
        }
        if (luaserver.luaFunctionExists(_file_preflight_funcname)) {
            std::unordered_map<std::string, std::string> pre_flight_info;
            try {
                pre_flight_info = call_blob_preflight(conn, luaserver, infile);
            }
            catch (IIIFError &err) {
                send_error(conn, Connection::INTERNAL_SERVER_ERROR, err);
                return;
            }
            if (pre_flight_info["type"] == "allow") {
                infile = pre_flight_info["infile"];
            }
            else if (pre_flight_info["type"] == "restrict") {
                infile = pre_flight_info["infile"];
            }
            else {
                send_error(conn, Connection::UNAUTHORIZED, "Unauthorized access");
                return;
            }
        }
        if (access(infile.c_str(), R_OK) == 0) {
            std::string actual_mimetype = Parsing::getBestFileMimetype(infile);

            //
            // first we get the filesize and time using fstat
            //
            struct stat fstatbuf{};

            if (stat(infile.c_str(), &fstatbuf) != 0) {
                Server::logger()->debug("Cannot fstat file: {}", infile.c_str());
                send_error(conn, Connection::INTERNAL_SERVER_ERROR);
            }
            size_t fsize = fstatbuf.st_size;
#ifdef __APPLE__
            struct timespec rawtime = fstatbuf.st_mtimespec;
#else
            struct timespec rawtime = fstatbuf.st_mtim;
#endif
            char timebuf[100];
            std::strftime(timebuf, sizeof timebuf, "%a, %d %b %Y %H:%M:%S %Z", std::gmtime(&rawtime.tv_sec));

            std::string range = conn.header("range");
            if (range.empty()) {
                // no "Content-Length" since send_file() will add this
                conn.header("Content-Type", actual_mimetype);
                conn.header("Cache-Control", "public, must-revalidate, max-age=0");
                conn.header("Pragma", "no-cache");
                conn.header("Accept-Ranges", "bytes");
                conn.header("Last-Modified", timebuf);
                conn.header("Content-Transfer-Encoding: binary");
                conn.sendFile(infile);
            } else {
                //
                // now we parse the range
                //
                std::regex re("bytes=\\s*(\\d+)-(\\d*)[\\D.*]?");
                std::cmatch m;
                int start = 0;       // lets assume beginning of file
                int end = fsize - 1; // lets assume whole file
                if (std::regex_match(range.c_str(), m, re)) {
                    if (m.size() < 2) {
                        send_error(conn, Connection::BAD_REQUEST, "Range expression invalid!");
                        return;
                    }
                    start = std::stoi(m[1]);
                    if ((m.size() > 1) && !m[2].str().empty()) {
                        end = std::stoi(m[2]);
                    }
                }
                else {
                    send_error(conn, Connection::BAD_REQUEST, "Range expression invalid!");
                    return;
                }

                // no "Content-Length" since send_file() will add this
                conn.status(Connection::PARTIAL_CONTENT);
                conn.header("Content-Type", actual_mimetype);
                conn.header("Cache-Control", "public, must-revalidate, max-age=0");
                conn.header("Pragma", "no-cache");
                conn.header("Accept-Ranges", "bytes");
                std::stringstream ss;
                ss << "bytes " << start << "-" << end << "/" << fsize;
                conn.header("Content-Range", ss.str());
                conn.header("Content-Disposition", std::string("inline; filename=") + params.at(IIIF_IDENTIFIER));
                conn.header("Content-Transfer-Encoding: binary");
                conn.header("Last-Modified", timebuf);
                conn.sendFile(infile, 8192, start, end);
            }
            conn.flush();
        } else {
            Server::logger()->warn("File '{}' not accessible.", infile.c_str());
            send_error(conn, Connection::NOT_FOUND, fmt::format("File '{}' not accessible.", infile.c_str()));
        }
    }
}