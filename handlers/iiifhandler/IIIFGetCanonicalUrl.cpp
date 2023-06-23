//
// Created by Lukas Rosenthaler on 27.07.22.
//

#include "IIIFHandler.h"
#include "iiifparser/IIIFRotation.h"
#include "iiifparser/IIIFQualityFormat.h"

static const char file_[] = __FILE__;

namespace cserve {

    std::pair<std::string, std::string> IIIFHandler::get_canonical_url (
            uint32_t tmp_w,
            uint32_t tmp_h,
            bool secure,
            const std::string &host,
            const std::string &route,
            const std::string &prefix,
            const std::string &identifier,
            const std::shared_ptr<IIIFRegion>& region,
            const std::shared_ptr<IIIFSize>& size,
            IIIFRotation &rotation,
            IIIFQualityFormat &quality_format) const
    {
        static const int canonical_len = 127;

        char canonical_region[canonical_len + 1];
        char canonical_size[canonical_len + 1];

        int tmp_r_x = 0, tmp_r_y = 0;
        uint32_t tmp_red = 0;
        uint32_t tmp_r_w = 0, tmp_r_h = 0;
        bool tmp_ro = false;

        if (region->getType() != IIIFRegion::FULL) {
            region->crop_coords(tmp_w, tmp_h, tmp_r_x, tmp_r_y, tmp_r_w, tmp_r_h);
        }

        region->canonical(canonical_region, canonical_len);

        if (size->get_type() != IIIFSize::FULL) {
            try {
                size->get_size(tmp_w, tmp_h, tmp_r_w, tmp_r_h, tmp_red, tmp_ro);
            }
            catch (const IIIFSizeError &err) {
                throw IIIFError(file_, __LINE__, "SipiSize error!");
            }
        }

        size->canonical(canonical_size, canonical_len);
        float angle;
        bool mirror = rotation.get_rotation(angle);
        char canonical_rotation[canonical_len + 1];

        if (mirror || (angle != 0.0)) {
            if ((angle - floorf(angle)) < 1.0e-6) { // it's an integer
                if (mirror) {
                    (void) snprintf(canonical_rotation, canonical_len, "!%ld", lround(angle));
                }
                else {
                    (void) snprintf(canonical_rotation, canonical_len, "%ld", lround(angle));
                }
            }
            else {
                if (mirror) {
                    (void)snprintf(canonical_rotation, canonical_len, "!%1.1f", angle);
                }
                else {
                    (void)snprintf(canonical_rotation, canonical_len, "%1.1f", angle);
                }
            }
        }
        else {
            (void)snprintf(canonical_rotation, canonical_len, "0");
        }

        //const unsigned canonical_header_len = 511;
        //char canonical_header[canonical_header_len + 1];
        char ext[5];

        switch (quality_format.format()) {
            case IIIFQualityFormat::JPG: {
                ext[0] = 'j';
                ext[1] = 'p';
                ext[2] = 'g';
                ext[3] = '\0';
                break; // jpg
            }
            case IIIFQualityFormat::JP2: {
                ext[0] = 'j';
                ext[1] = 'p';
                ext[2] = '2';
                ext[3] = '\0';
                break; // jp2
            }
            case IIIFQualityFormat::TIF: {
                ext[0] = 't';
                ext[1] = 'i';
                ext[2] = 'f';
                ext[3] = '\0';
                break; // tif
            }
            case IIIFQualityFormat::PNG: {
                ext[0] = 'p';
                ext[1] = 'n';
                ext[2] = 'g';
                ext[3] = '\0';
                break; // png
            }

            default:{
                throw IIIFError(file_, __LINE__, "Unsupported file format requested! Supported are .jpg, .jp2, .tif, .png");
            }
        }

        std::string format;

        if (quality_format.quality() != IIIFQualityFormat::DEFAULT){
            switch (quality_format.quality()) {
                case IIIFQualityFormat::COLOR: {
                    format = "color.";
                    break;
                }
                case IIIFQualityFormat::GRAY: {
                    format = "gray.";
                    break;
                }
                case IIIFQualityFormat::BITONAL: {
                    format = "bitonal.";
                    break;
                }
                default: {
                    format = "default.";
                }
            }
        }
        else {
            format = "default.";
        }

        const std::string& fullid = identifier;
        std::string canonical = host;
        if (!route.empty()) canonical += "/" + route;
        if (!prefix.empty()) canonical += "/" + prefix;
        canonical += "/" + fullid +
                "/" + std::string(canonical_region) +
                "/" + std::string(canonical_size) +
                "/" + std::string(canonical_rotation) +
                "/" + format + std::string(ext);
        std::string canonical_header = secure ? "<https://" : "<http://" + canonical + ">";

        return make_pair(canonical_header, canonical);
    }

}