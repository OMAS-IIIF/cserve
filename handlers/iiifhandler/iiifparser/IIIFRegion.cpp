#include <string>
#include <iostream>
#include <sstream>
#include <vector>
#include <cmath>

#include <cstdio>

#include "fmt/format.h"

#include "../IIIFError.h"
#include "IIIFRegion.h"

static const char file_[] = __FILE__;

namespace cserve {

    /**
     * IIIF Version 3 compliant Region class
     *
     * @param str Region string from IIIF URL
     */
    IIIFRegion::IIIFRegion(std::string str) {
        int n;
        reduce = 1.0F;
        if (str.empty() || (str == "full")) {
            coord_type = FULL;
            rx = 0.F;
            ry = 0.F;
            rw = 0.F;
            rh = 0.F;
            canonical_ok = true; // "full" is a canonical value
        } else if (str == "square") {
            coord_type = IIIFRegion::SQUARE;
            rx = 0.F;
            ry = 0.F;
            rw = 0.F;
            rh = 0.F;
            canonical_ok = false;
        } else if (str.find("pct:") != std::string::npos) {
            coord_type = IIIFRegion::PERCENTS;
            std::string tmpstr = str.substr(4);
            n = sscanf(tmpstr.c_str(), "%f,%f,%f,%f", &rx, &ry, &rw, &rh);

            if (n != 4) {
                throw IIIFError(file_, __LINE__, "IIIF Error reading Region parameter  \"" + str + "\"");
            }

            canonical_ok = false;
        } else {
            coord_type = IIIFRegion::COORDS;
            n = sscanf(str.c_str(), "%f,%f,%f,%f", &rx, &ry, &rw, &rh);

            if (n != 4) {
                throw IIIFError(file_, __LINE__, "IIIF Error reading Region parameter  \"" + str + "\"");
            }

            canonical_ok = false;
        }
    }
    //-------------------------------------------------------------------------

    /**
     * Get the cropping coordinates
     *
     * @param nx [in] Width of image
     * @param ny [in] Height of image
     * @param p_x [out] Start of cropping X
     * @param p_y [out] Start of cropping Y
     * @param p_w [out] Width of cropping
     * @param p_h [out] Height of cropping
     * @return Region type
     */
    IIIFRegion::CoordType IIIFRegion::crop_coords(uint32_t nx, uint32_t ny, int32_t &p_x, int32_t &p_y, uint32_t &p_w, uint32_t &p_h) {
        switch (coord_type) {
            case COORDS: {
                x = static_cast<int32_t>(lroundf(rx/reduce));
                y = static_cast<int32_t>(lroundf(ry/reduce));
                w = lroundf(rw/reduce);
                h = lroundf(rh/reduce);
                break;
            }
            case SQUARE: {
                if (nx > ny) { // landscape format
                    x = floor(static_cast<float>(nx - ny) / 2.0F);
                    y = 0;
                    w = h = ny;
                } else { // portrait format or square
                    x = 0;
                    y = floor(static_cast<float>(ny - nx) / 2.0F);
                    w = h = nx;
                }
                break;
            }
            case PERCENTS: {
                x = static_cast<int32_t>(lroundf((rx * static_cast<float>(nx) / 100.F)));
                y = static_cast<int32_t>(lroundf((ry * static_cast<float>(ny) / 100.F)));
                w = lroundf((rw * static_cast<float>(nx) / 100.F));
                h = lroundf((rh * static_cast<float>(ny) / 100.F));
                break;
            }
            case FULL: {
                x = 0;
                y = 0;
                w = nx;
                h = ny;
                break;
            }
        }

        if (x < 0) {
            w += x;
            x = 0;
        } else if (x >= nx) {
            throw IIIFError(file_, __LINE__, fmt::format("Invalid cropping region outside of image (x={} nx={})", x, nx));
        }

        if (y < 0) {
            h += y;
            y = 0;
        } else if (y >= ny) {
            std::stringstream msg;
            msg << "Invalid cropping region outside of image (y=" << y << " ny=" << ny << ")";
            throw IIIFError(file_, __LINE__, msg.str());
        }

        if (w == 0) {
            w = nx - x;
        } else if ((x + w) > nx) {
            w = nx - x;
        }

        if (h == 0) {
            h = ny - y;
        } else if ((y + h) > ny) {
            h = ny - y;
        }

        p_x = x;
        p_y = y;
        p_w = w;
        p_h = h;

        canonical_ok = true;

        return coord_type;
    }
    //-------------------------------------------------------------------------

    /**
     * Get canonical region definition
     *
     * @param buf String buffer to write result into
     * @param buflen  Length of string buffer
     */
    void IIIFRegion::canonical(char *buf, int buflen) {
        if (!canonical_ok && (coord_type != FULL)) {
            std::string msg = "Canonical coordinates not determined";
            throw IIIFError(file_, __LINE__, msg);
        }

        switch (coord_type) {
            case FULL: {
                (void) snprintf(buf, buflen, "full");
                break;
            }
            case SQUARE:
            case COORDS:
            case PERCENTS: {
                (void) snprintf(buf, buflen, "%d,%d,%ld,%ld", x, y, w, h);
                break;
            }
        }
    }
    //-------------------------------------------------------------------------

    std::string IIIFRegion::canonical() {
        if (!canonical_ok && (coord_type != FULL)) {
            std::string msg = "Canonical coordinates not determined";
            throw IIIFError(file_, __LINE__, msg);
        }
        switch (coord_type) {
            case FULL: {
                return std::string("full");
            }
            case SQUARE:
            case COORDS:
            case PERCENTS: {
                return fmt::format("{},{},{},{}", x, y, w, h);
            }
        }
    }
    //-------------------------------------------------------------------------
    // Output to stdout for debugging etc.
    //
    std::ostream &operator<<(std::ostream &outstr, const IIIFRegion &rhs) {
        std::string ct;
        switch (rhs.coord_type) {
            case IIIFRegion::FULL: ct = "FULL"; break;
            case IIIFRegion::SQUARE: ct = "SQUARE"; break;
            case IIIFRegion::COORDS: ct = "COORDS"; break;
            case IIIFRegion::PERCENTS: ct = "PERCENTS"; break;
        }
        outstr << "IIIF-Server Region:";
        outstr << "  Coordinate type: " << ct;
        outstr << " | rx = " << rhs.rx << " | ry = " << rhs.ry << " | rw = "
               << rhs.rw << " | rh = " << rhs.rh;
        return outstr;
    }
    //-------------------------------------------------------------------------

}

