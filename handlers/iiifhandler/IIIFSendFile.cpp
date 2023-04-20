//
// Created by Lukas Rosenthaler on 26.07.22.
//

#include "../lib/Cserve.h"
#include "../lib/Parsing.h"
#include "HttpSendError.h"
#include "IIIFHandler.h"
#include "IIIFImage.h"
#include "iiifparser/IIIFIdentifier.h"
#include "iiifparser/IIIFRotation.h"
#include "iiifparser/IIIFQualityFormat.h"


static const char file_[] = __FILE__;


namespace cserve {

    void IIIFHandler::send_iiif_file(Connection &conn, LuaServer &luaserver,
                                     const std::unordered_map<Parts, std::string> &params) const {

        //
        // getting the identifier (which in case of a PDF or multipage TIFF my contain a page id (identifier@pagenum)
        //
        IIIFIdentifier sid{urldecode(params.at(IIIF_IDENTIFIER))};
        //
        // getting IIIF parameters
        //
        auto region = std::make_shared<IIIFRegion>();
        auto size = std::make_shared<IIIFSize>();
        IIIFRotation rotation;
        IIIFQualityFormat quality_format;
        try {
            region = std::make_shared<IIIFRegion>(params.at(IIIF_REGION));
            size = std::make_shared<IIIFSize>(params.at(IIIF_SIZE), _iiif_max_image_width, _iiif_max_image_height);
            rotation = IIIFRotation(params.at(IIIF_ROTATION));
            quality_format = IIIFQualityFormat(params.at(IIIF_QUALITY), params.at(IIIF_FORMAT));
        }
        catch (IIIFError &err) {
            send_error(conn, Connection::BAD_REQUEST, err);
            return;
        }
        //
        // here we start the lua script which checks for permissions
        //
        std::string infile;                                   // path to the input file on the server
        std::string watermark;                                // path to watermark file, or empty, if no watermark required
        auto restriction_size = std::make_shared<IIIFSize>(); // size of restricted image... (SizeType::FULL if unrestricted)

        if (luaserver.luaFunctionExists(_iiif_preflight_funcname)) {
            std::unordered_map<std::string, std::string> pre_flight_info;
            try {
                pre_flight_info = call_iiif_preflight(conn, luaserver, params.at(IIIF_PREFIX), sid.get_identifier());
            }
            catch (IIIFError &err) {
                send_error(conn, Connection::INTERNAL_SERVER_ERROR, err.to_string());
                return;
            }

            infile = pre_flight_info["infile"];
            if (pre_flight_info["type"] != "allow") {
                if (pre_flight_info["type"] == "restrict") {
                    bool ok = false;
                    try {
                        watermark = pre_flight_info.at("watermark");
                        ok = true;
                    }
                    catch (const std::out_of_range &err) { ; // do nothing, no watermark...
                    }
                    try {
                        std::string tmpstr = pre_flight_info.at("size");
                        restriction_size = std::make_shared<IIIFSize>(tmpstr, _iiif_max_image_width, _iiif_max_image_height);
                        ok = true;
                    }
                    catch (const std::out_of_range &err) { ; // do nothing, no size restriction
                    }
                    if (!ok) {
                        send_error(conn, Connection::UNAUTHORIZED, "Unauthorized access (#1)");
                        return;
                    }
                } else {
                    send_error(conn, Connection::UNAUTHORIZED, "Unauthorized access (#2)");
                    return;
                }
            }
        } else {
            if (_prefix_as_path && (!params.at(IIIF_PREFIX).empty())) {
                infile = _imgroot + "/" + params.at(IIIF_PREFIX) + "/" + sid.get_identifier();
            } else {
                infile = _imgroot + "/" + sid.get_identifier();
            }
        }

        //
        // determine the mimetype of the file in the IIIF repo
        //
        IIIFQualityFormat::FormatType in_format = IIIFQualityFormat::UNSUPPORTED;

        std::string actual_mimetype = Parsing::getBestFileMimetype(infile);
        if (actual_mimetype == "image/tiff")
            in_format = IIIFQualityFormat::TIF;
        if (actual_mimetype == "image/jpeg")
            in_format = IIIFQualityFormat::JPG;
        if (actual_mimetype == "image/png")
            in_format = IIIFQualityFormat::PNG;
        if ((actual_mimetype == "image/jpx") || (actual_mimetype == "image/jp2"))
            in_format = IIIFQualityFormat::JP2;
        if (actual_mimetype == "application/pdf")
            in_format = IIIFQualityFormat::PDF;

        if (access(infile.c_str(), R_OK) != 0) { // test, if file exists
            Server::logger()->info("File '{}' not found", infile);
            send_error(conn, Connection::NOT_FOUND);
            return;
        }

        float angle;
        bool mirror = rotation.get_rotation(angle);

        //
        // get cache info
        //
        size_t img_w = 0, img_h = 0;
        //size_t tile_w = 0, tile_h = 0;
        //int clevels = 0;
        //int numpages = 0;
        std::vector<SubImageInfo> resolutions;

        //
        // get image dimensions, needed for get_canonical...
        //
        if ((_cache == nullptr) || !_cache->getSize(infile, img_w, img_h, resolutions)) {
            IIIFImage tmpimg;
            IIIFImgInfo info;
            try {
                info = tmpimg.getDim(infile);
            }
            catch (IIIFImageError &err) {
                send_error(conn, Connection::INTERNAL_SERVER_ERROR, err.to_string());
                return;
            }
            if (info.success == IIIFImgInfo::FAILURE) {
                send_error(conn, Connection::INTERNAL_SERVER_ERROR, "Couldn't get image dimensions!");
                return;
            }
            img_w = info.width;
            img_h = info.height;
            resolutions = info.resolutions;
        }

        uint32_t tmp_r_w{0L}, tmp_r_h{0L};
        uint32_t tmp_red{0};
        bool tmp_ro{false};
        try {
            size->get_size(img_w, img_h, tmp_r_w, tmp_r_h, tmp_red, tmp_ro);
            restriction_size->get_size(img_w, img_h, tmp_r_w, tmp_r_h, tmp_red, tmp_ro);
        }
        catch (IIIFSizeError &err) {
            send_error(conn, Connection::BAD_REQUEST, err.to_string());
            return;
        }
        catch (IIIFImageError &err) {
            send_error(conn, Connection::BAD_REQUEST, err);
            return;
        }

        catch (IIIFError &err) {
            send_error(conn, Connection::BAD_REQUEST, err);
            return;
        }

        if (!restriction_size->undefined() && (*size > *restriction_size)) {
            size = restriction_size;
        }

        //.....................................................................
        // here we start building the canonical URL
        //
        std::pair<std::string, std::string> tmppair;
        try {
            tmppair = get_canonical_url(img_w, img_h, conn.host(), params.at(IIIF_PREFIX),
                                              sid.get_identifier(), region, size, rotation, quality_format);
        }
        catch (IIIFError &err) {
            send_error(conn, Connection::BAD_REQUEST, err);
            return;
        }

        std::string canonical_header = tmppair.first;
        std::string canonical = tmppair.second;

        // now we check if we can send the file directly
        //
        if ((region->getType() == IIIFRegion::FULL) && (size->get_type() == IIIFSize::FULL) && (angle == 0.0) &&
            (!mirror) && watermark.empty() && (quality_format.format() == in_format) &&
            (quality_format.quality() == IIIFQualityFormat::DEFAULT)) {

            conn.status(Connection::OK);
            conn.header("Cache-Control", "must-revalidate, post-check=0, pre-check=0");
            conn.header("Link", canonical_header);

            // set the header (mimetype)
            switch (quality_format.format()) {
                case IIIFQualityFormat::TIF:
                    conn.header("Content-Type", "image/tiff");
                    break;
                case IIIFQualityFormat::JPG:
                    conn.header("Content-Type", "image/jpeg");
                    break;
                case IIIFQualityFormat::PNG:
                    conn.header("Content-Type", "image/png");
                    break;
                case IIIFQualityFormat::JP2:
                    conn.header("Content-Type", "image/jp2");
                    break;
                case IIIFQualityFormat::PDF:
                    conn.header("Content-Type", "application/pdf");
                    break;
                default: {
                }
            }
            try {
                conn.sendFile(infile);
            }
            catch (const InputFailure &iofail) {
                Server::logger()->warn("Client unexpectedly closed connection");
            }
            catch (const IIIFError &err) {
                Server::logger()->warn("Internal error: {}", err.to_string());
                send_error(conn, Connection::INTERNAL_SERVER_ERROR, err);
            }
            return;
        } // finish sending unmodified file in toto

        if (_cache != nullptr) {
            //!>
            //!> here we check if the file is in the cache. If so, it's being blocked from deletion
            //!>
            std::string cachefile = _cache->check(infile, canonical,
                                                 true); // we block the file from being deleted if successfull

            if (!cachefile.empty()) {
                Server::logger()->debug("Using cachefile {}", cachefile);
                conn.status(Connection::OK);
                conn.header("Cache-Control", "must-revalidate, post-check=0, pre-check=0");
                conn.header("Link", canonical_header);

                // set the header (mimetype)
                switch (quality_format.format()) {
                    case IIIFQualityFormat::TIF:
                        conn.header("Content-Type", "image/tiff");
                        break;
                    case IIIFQualityFormat::JPG:
                        conn.header("Content-Type", "image/jpeg");
                        break;
                    case IIIFQualityFormat::PNG:
                        conn.header("Content-Type", "image/png");
                        break;
                    case IIIFQualityFormat::JP2:
                        conn.header("Content-Type", "image/jp2");
                        break;
                    case IIIFQualityFormat::PDF: {
                        conn.header("Content-Type", "application/pdf"); // set the header (mimetype)
                        break;
                    }
                    default: {
                    }
                }

                try {
                    //!> send the file from cache
                    conn.sendFile(cachefile);
                    //!> from now on the cache file can be deleted again
                }
                catch (const InputFailure &err) {
                    // -1 was thrown
                    Server::logger()->warn("Client unexpectedly closed connection");
                    _cache->deblock(cachefile);
                    return;
                }
                catch (const IIIFError &err) {
                    Server::logger()->error("Error sending cache file: \"{}\": {}", cachefile, err.to_string());
                    send_error(conn, Connection::INTERNAL_SERVER_ERROR, err);
                    _cache->deblock(cachefile);
                    return;
                }
                _cache->deblock(cachefile);
                return;
            }
            _cache->deblock(cachefile);
        }

        IIIFImage img;
        try {
            img = IIIFImage::read(infile, region, size, quality_format.format() == IIIFQualityFormat::JPG, _scaling_quality);
        }
        catch (const IIIFImageError &err) {
            send_error(conn, Connection::INTERNAL_SERVER_ERROR, err);
            return;
        }
        catch (const IIIFSizeError &err) {
            send_error(conn, Connection::BAD_REQUEST, err.to_string());
            return;
        }

        //
        // now we rotate
        //
        if (mirror || (angle != 0.0)) {
            try {
                img.rotate(angle, mirror);
            }
            catch (IIIFError &err) {
                send_error(conn, Connection::INTERNAL_SERVER_ERROR, err);
                return;
            }
        }

        if (quality_format.quality() != IIIFQualityFormat::DEFAULT) {
            switch (quality_format.quality()) {
                case IIIFQualityFormat::COLOR:
                    img.convertToIcc(IIIFIcc(icc_sRGB), 8);
                    break; // for now, force 8 bit/sample
                case IIIFQualityFormat::GRAY:
                    img.convertToIcc(IIIFIcc(icc_GRAY_D50), 8);
                    break; // for now, force 8 bit/sample
                case IIIFQualityFormat::BITONAL:
                    img.toBitonal();
                    break;
                default: {
                    send_error(conn, Connection::BAD_REQUEST, "Invalid quality specificer");
                    return;
                }
            }
        }

        //
        // let's add a watermark if necessary
        //
        if (!watermark.empty()) {
            watermark = "watermark.tif";
            try {
                img.add_watermark(watermark);
            }
            catch (const IIIFError &err) {
                send_error(conn, Connection::INTERNAL_SERVER_ERROR, err);
                return;
            }
            Server::logger()->info("GET {}: adding watermark", conn.uri());
        }

        img.connection(&conn);
        conn.header("Cache-Control", "must-revalidate, post-check=0, pre-check=0");
        std::string cachefile;

        try {
            if (_cache != nullptr) {
                try {
                    //!> open the cache file to write into.
                    cachefile = _cache->getNewCacheFileName();
                    conn.openCacheFile(cachefile);
                }
                catch (const Error &err) {
                    send_error(conn, Connection::INTERNAL_SERVER_ERROR, err);
                    return;
                }
            }
            switch (quality_format.format()) {
                case IIIFQualityFormat::JPG: {
                    conn.status(Connection::OK);
                    conn.header("Link", canonical_header);
                    conn.header("Content-Type", "image/jpeg"); // set the header (mimetype)

                    if ((img.getNc() > 3) && (img.getNalpha() > 0)) { // we have an alpha channel....
                        for (size_t i = 3; i < (img.getNalpha() + 3); i++)
                            img.removeChan(i);
                    }

                    IIIFIcc icc = IIIFIcc(icc_sRGB); // force sRGB !!
                    img.convertToIcc(icc, 8);
                    conn.setChunkedTransfer();
                    IIIFCompressionParams qp = {{JPEG_QUALITY, std::to_string(_jpeg_quality)}};
                    img.write("jpg", "HTTP", qp);
                    break;
                }

                case IIIFQualityFormat::JP2: {
                    conn.status(Connection::OK);
                    conn.header("Link", canonical_header);
                    conn.header("Content-Type", "image/jp2"); // set the header (mimetype)
                    conn.setChunkedTransfer();
                    img.write("jpx", "HTTP");
                    break;
                }

                case IIIFQualityFormat::TIF: {
                    conn.status(Connection::OK);
                    conn.header("Link", canonical_header);
                    conn.header("Content-Type", "image/tiff"); // set the header (mimetype)
                    // no chunked transfer needed...

                    img.write("tif", "HTTP");
                    break;
                }

                case IIIFQualityFormat::PNG: {
                    conn.status(Connection::OK);
                    conn.header("Link", canonical_header);
                    conn.header("Content-Type", "image/png"); // set the header (mimetype)
                    conn.setChunkedTransfer();

                    img.write("png", "HTTP");
                    break;
                }

                default: {
                    // HTTP 400 (format not supported)
                    Server::logger()->warn("Unsupported file format requested! Supported are .jpg, .jp2, .tif, .png");
                    conn.setBuffer();
                    conn.status(Connection::BAD_REQUEST);
                    conn.header("Content-Type", "text/plain");
                    conn << "Not Implemented!\n";
                    conn << "Unsupported file format requested! Supported are .jpg, .jp2, .tif, .png, .pdf\n";
                    conn.flush();
                }
            }

            if (conn.isCacheFileOpen()) {
                conn.closeCacheFile();
                //!>
                //!> ATTENTION!!! Here we change the list of available cache files
                //!>
                _cache->add(infile, canonical, cachefile, img_w, img_h, resolutions);
            }
        }
        catch (const IIIFError &err) {
            if (_cache != nullptr) {
                conn.closeCacheFile();
                unlink(cachefile.c_str());
            }
            send_error(conn, Connection::INTERNAL_SERVER_ERROR, err);
            return;
        }

        conn.flush();
   }
}