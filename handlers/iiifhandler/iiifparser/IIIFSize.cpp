
#include <string>
#include <iostream>
#include <sstream>

#include <vector>
#include <cmath>
#include <climits>

#include <cstdio>

#include "Parsing.h"
#include "../IIIFError.h"
#include "IIIFSize.h"
#include "fmt/format.h"

static const char file_[] = __FILE__;

namespace cserve {

    size_t IIIFSize::limitdim = 32000;

    IIIFSize::IIIFSize(std::string str, size_t iiif_max_width, size_t iiif_max_height, size_t iiif_max_area)
            : iiif_max_width(iiif_max_width), iiif_max_height(iiif_max_height), iiif_max_area(iiif_max_area) {
        nx = ny = w = h =0;
        percent = 0.F;
        canonical_ok = false;
        reduce = 0;
        redonly = false;
        size_type = SizeType::UNDEFINED;

        upscaling = str[0] == '^';
        if (upscaling) {
            str.erase(0, 1);
        }

        bool exclamation_mark = str[0] == '!';
        if (exclamation_mark) {
            str.erase(0, 1);
        }

        try {
            if (str.empty() || str == "max") {
                size_type = SizeType::FULL;
            } else if (str.find("pct") != std::string::npos) {
                if (exclamation_mark) throw IIIFError(file_, __LINE__, "Invalid IIIF size parameter: \"!" + str + "\": \"!\" not allowed here!");
                size_type = SizeType::PERCENTS;
                std::string percent_str = str.substr(4);
                percent = Parsing::parse_float(percent_str);
                if (percent < 0.0) percent = 1.0;
            } else if (str.find("red") != std::string::npos) {
                if (exclamation_mark) throw IIIFError(file_, __LINE__, "Invalid IIIF size parameter: \"!" + str + "\": \"!\" not allowed here!");
                size_type = SizeType::REDUCE;
                std::string reduce_str = str.substr(4);
                reduce = static_cast<int>(Parsing::parse_int(reduce_str));
                if (reduce < 0) reduce = 0;
            } else {
                size_t comma_pos = str.find(',');

                if (comma_pos == std::string::npos) {
                    throw IIIFError(file_, __LINE__, "Could not parse IIIF size parameter: \"" + str + "\"");
                }

                std::string width_str = str.substr(0, comma_pos);
                std::string height_str = str.substr(comma_pos + 1);

                if ((width_str.empty() && height_str.empty()) ||
                    (size_type == SizeType::MAXDIM && (width_str.empty() || height_str.empty()))) {
                    throw IIIFError(file_, __LINE__, "Could not parse IIIF size parameter: \"" + str + "\" ");
                }

                if (width_str.empty()) { // ",h" or "^,h"
                    if (exclamation_mark) throw IIIFError(file_, __LINE__, "Invalid IIIF size parameter: \"!" + str + "\": \"!\" not allowed here!");
                    ny = Parsing::parse_int(height_str);
                    if (ny == 0) {
                        throw IIIFError(file_, __LINE__, "IIIF height cannot be zero");
                    }
                    size_type = SizeType::PIXELS_Y;
                } else if (height_str.empty()) { // "w," or "^w,"
                    if (exclamation_mark) throw IIIFError(file_, __LINE__, "Invalid IIIF size parameter: \"!" + str + "\": \"!\" not allowed here!");
                    nx = Parsing::parse_int(width_str);
                    if (nx == 0) {
                        throw IIIFError(file_, __LINE__, "IIIF width cannot be zero");
                    }

                    size_type = SizeType::PIXELS_X;
                } else { // "w,h" or "^w,h" or "!w,h" or "^!w,h"
                    nx = Parsing::parse_int(width_str);
                    ny = Parsing::parse_int(height_str);

                    if (nx == 0 || ny == 0) {
                        throw IIIFError(file_, __LINE__,
                                        "IIIF size would result in a width or height of zero: " + str);
                    }

                    if (exclamation_mark) {
                        size_type = SizeType::MAXDIM;
                    } else {
                        size_type = SizeType::PIXELS_XY;
                    }
                }

                /*
                size_t limit_nx = limitdim;
                size_t limit_ny = limitdim;
                if (iiif_max_width > 0) {
                    limit_nx = iiif_max_width;
                    limit_ny = iiif_max_width;
                    if (iiif_max_height > 0) {
                        limit_ny = iiif_max_height;
                    }
                }
                if (nx > limit_nx) nx = limit_nx;
                if (ny > limit_ny) ny = limit_ny;
                */
            }
        } catch (const Error &error) {
            throw IIIFError(file_, __LINE__, "Could not parse IIIF size parameter: " + error.to_string());
        }
    }

    IIIFSize::IIIFSize(const IIIFSize &other) {
        size_type = other.size_type;
        upscaling = other.upscaling;
        percent = other.percent;
        reduce = other.reduce;
        redonly = other.redonly;
        nx = other.nx;
        ny = other.ny;
        w = other.w;
        h = other.h;
        iiif_max_width = other.iiif_max_width;
        iiif_max_height = other.iiif_max_height;
        canonical_ok = other.iiif_max_width;
    }

    IIIFSize::IIIFSize(IIIFSize &&other) noexcept
            : size_type(SizeType::UNDEFINED), upscaling(false), percent(0.0F), reduce(0), redonly(false), nx(0), ny(0),
              w(0), h(0), iiif_max_height(0), iiif_max_width(0), canonical_ok(false) {
        size_type = other.size_type;
        upscaling = other.upscaling;
        percent = other.percent;
        reduce = other.reduce;
        redonly = other.redonly;
        nx = other.nx;
        ny = other.ny;
        w = other.w;
        h = other.h;
        iiif_max_width = other.iiif_max_width;
        iiif_max_height = other.iiif_max_height;
        canonical_ok = other.canonical_ok;

        other.size_type = SizeType::UNDEFINED;
        other.upscaling = false;
        other.percent = 0.0F;
        other.reduce = 0;
        other.redonly = false;
        other.nx = 0;
        other.ny = 0;
        other.w = 0;
        other.h = 0;
        other.iiif_max_width = 0;
        other.iiif_max_height = 0;
        other.canonical_ok = false;
    }

    //-------------------------------------------------------------------------

    IIIFSize &IIIFSize::operator=(const IIIFSize &other) {
        if (this != &other) {
            size_type = other.size_type;
            upscaling = other.upscaling;
            percent = other.percent;
            reduce = other.reduce;
            redonly = other.redonly;
            nx = other.nx;
            ny = other.ny;
            w = other.w;
            h = other.h;
            iiif_max_width = other.iiif_max_width;
            iiif_max_height = other.iiif_max_height;
            canonical_ok = other.iiif_max_width;
        }
        return *this;
    }

    IIIFSize &IIIFSize::operator=(IIIFSize &&other) {
        if (this != &other) {
            size_type = SizeType::UNDEFINED;
            upscaling = false;
            percent = 0.0F;
            reduce = 0;
            redonly = false;
            nx = 0;
            ny = 0;
            w = 0;
            h = 0;
            iiif_max_width = 0;
            iiif_max_height = 0;
            canonical_ok = false;

            size_type = other.size_type;
            upscaling = other.upscaling;
            percent = other.percent;
            reduce = other.reduce;
            redonly = other.redonly;
            nx = other.nx;
            ny = other.ny;
            w = other.w;
            h = other.h;
            iiif_max_width = other.iiif_max_width;
            iiif_max_height = other.iiif_max_height;
            canonical_ok = other.iiif_max_width;

            other.size_type = SizeType::UNDEFINED;
            other.upscaling = false;
            other.percent = 0.0F;
            other.reduce = 0;
            other.redonly = false;
            other.nx = 0;
            other.ny = 0;
            other.w = 0;
            other.h = 0;
            other.iiif_max_width = 0;
            other.iiif_max_height = 0;
            other.canonical_ok = false;
        }
        return *this;
    }

    bool IIIFSize::operator>(const IIIFSize &s) const {
        if (!canonical_ok) {
            throw IIIFError(file_, __LINE__, "Final size not yet determined");
        }
        return ((w > s.w) || (h > s.h));
    }
    //-------------------------------------------------------------------------

    bool IIIFSize::operator>=(const IIIFSize &s) const {
        if (!canonical_ok) {
            throw IIIFError(file_, __LINE__, "Final size not yet determined");
        }

        return ((w >= s.w) || (h >= s.h));
    }
    //-------------------------------------------------------------------------

    bool IIIFSize::operator<(const IIIFSize &s) const {
        if (!canonical_ok) {
            throw IIIFError(file_, __LINE__, "Final size not yet determined");
        }

        return ((w < s.w) && (h < s.h));
    }
    //-------------------------------------------------------------------------

    bool IIIFSize::operator<=(const IIIFSize &s) const {
        if (!canonical_ok) {
            throw IIIFError(file_, __LINE__, "Final size not yet determined");
        }

        return ((w <= s.w) && (h <= s.h));
    }
    //-------------------------------------------------------------------------

    IIIFSize::SizeType IIIFSize::get_size(
            size_t img_w,
            size_t img_h,
            size_t &w_p,
            size_t &h_p,
            int &reduce_p,
            bool &redonly_p) {
        redonly = false;
        if (reduce_p == -1) reduce_p = INT_MAX;
        int max_reduce = reduce_p;
        reduce_p = 0;

        auto img_w_float = static_cast<float>(img_w);
        auto img_h_float = static_cast<float>(img_h);

        switch (size_type) {
            case IIIFSize::UNDEFINED: {
                w = 0;
                h = 0;
                reduce_p = 0;
                redonly_p = true;
                canonical_ok = true;
                break;
            }
            case IIIFSize::PIXELS_XY: {
                //
                // first we check how far the new image width and image height can be reached by a reduce factor
                //
                int sf_w = 1;
                int reduce_w = 0;
                bool exact_match_w = true;
                if (nx > img_w) {
                    if (!upscaling) {
                        throw IIIFSizeError(400, "Upscaling not allowed!");
                    }
                } else {
                    w = static_cast<size_t>(ceilf(img_w_float / static_cast<float>(sf_w)));

                    while ((w > nx) && (reduce_w < max_reduce)) {
                        sf_w *= 2;
                        w = static_cast<size_t>(ceilf(img_w_float / static_cast<float>(sf_w)));
                        reduce_w++;
                    }

                    if (w < nx) {
                        // we do not have an exact match. Go back one level with reduce
                        exact_match_w = false;
                        sf_w /= 2;
                        reduce_w--;
                    }
                    if (w > nx) exact_match_w = false;
                }

                int sf_h = 1;
                int reduce_h = 0;
                bool exact_match_h = true;
                if (ny > img_h) {
                    if (!upscaling) {
                        throw IIIFSizeError(400, "Upscaling not allowed!");
                    }
                } else {
                    h = static_cast<size_t>(ceilf(img_h_float / static_cast<float>(sf_h)));

                    while ((h > ny) && (reduce_w < max_reduce)) {
                        sf_h *= 2;
                        h = static_cast<size_t>(ceilf(img_h_float / static_cast<float>(sf_h)));
                        reduce_h++;
                    }
                    if (h < ny) {
                        // we do not have an exact match. Go back one level with reduce
                        exact_match_h = false;
                        sf_h /= 2;
                        reduce_h--;
                    }
                    if (h > ny) exact_match_h = false;
                }

                if (exact_match_w && exact_match_h && (reduce_w == reduce_h)) {
                    reduce_p = reduce_w;
                    redonly = true;
                } else {
                    reduce_p = reduce_w < reduce_h ? reduce_w : reduce_h; // min()
                    redonly = false;
                }

                w = nx;
                h = ny;
                break;
            }

            case IIIFSize::PIXELS_X: {
                //
                // first we check how far the new image width can be reached by a reduce factor
                //
                int sf_w = 1;
                int reduce_w = 0;
                bool exact_match_w = true;
                w = static_cast<size_t>(ceilf(img_w_float / static_cast<float>(sf_w)));
                while ((w > nx) && (reduce_w < max_reduce)) {
                    sf_w *= 2;
                    w = static_cast<size_t>(ceilf(img_w_float / static_cast<float>(sf_w)));
                    reduce_w++;
                }
                if (w < nx) {
                    // we do not have an exact match. Go back one level with reduce
                    exact_match_w = false;
                    sf_w /= 2;
                    reduce_w--;
                }
                if (w > nx) exact_match_w = false;

                w = nx;
                reduce_p = reduce_w;
                redonly = exact_match_w; // if exact_match, then reduce only

                if (exact_match_w) {
                    h = static_cast<size_t>(ceilf(img_h_float / static_cast<float>(sf_w)));
                } else {
                    h = static_cast<size_t>(ceilf(static_cast<float>(img_h * nx) / img_w_float));
                }

                if (!upscaling && (w > img_w || h > img_h)) {
                    throw IIIFSizeError(400, "Upscaling not allowed!");
                }
                break;
            }

            case IIIFSize::PIXELS_Y: {
                //
                // first we check how far the new image height can be reached by a reduce factor
                //
                int sf_h = 1;
                int reduce_h = 0;
                bool exact_match_h = true;
                h = static_cast<size_t>(img_h_float);

                while ((h > ny) && (reduce_h < max_reduce)) {
                    sf_h *= 2;
                    h = static_cast<size_t>(ceilf(img_h_float / static_cast<float>(sf_h)));
                    reduce_h++;
                }

                if (h < ny) {
                    // we do not have an exact match. Go back one level with reduce
                    exact_match_h = false;
                    sf_h /= 2;
                    reduce_h--;
                }
                if (h > ny) exact_match_h = false;

                h = ny;
                reduce_p = reduce_h;
                redonly = exact_match_h; // if exact_match, then reduce only

                if (exact_match_h) {
                    w = static_cast<size_t>(ceilf(img_w_float / static_cast<float>(sf_h)));
                } else {
                    w = static_cast<size_t>(ceilf(static_cast<float>(img_w * ny) / img_h_float));
                }
                if (!upscaling && (w > img_w || h > img_h)) {
                    throw IIIFSizeError(400, "Upscaling not allowed!");
                }
                break;
            }

            case IIIFSize::PERCENTS: {
                w = static_cast<size_t>(ceilf(img_w * percent / 100.F));
                h = static_cast<size_t>(ceilf(img_h * percent / 100.F));

                if (!upscaling && (percent > 100.0F)) {
                    throw IIIFSizeError(400, "Upscaling not allowed!");
                }

                reduce_p = 0;
                float r = 100.F / percent;
                float s = 1.0;

                while ((2.0 * s <= r) && (reduce_p < max_reduce)) {
                    s *= 2.0;
                    reduce_p++;
                }

                if (fabs(s - r) < 1.0e-5) {
                    redonly = true;
                }
                break;
            }
            case IIIFSize::REDUCE: {
                if (reduce == 0) {
                    w = img_w;
                    h = img_h;
                    reduce_p = 0;
                    redonly = true;
                    break;
                }

                int sf = 1;
                for (int i = 0; i < reduce; i++) sf *= 2;

                w = static_cast<size_t>(ceilf(img_w_float / static_cast<float>(sf)));
                h = static_cast<size_t>(ceilf(img_h_float / static_cast<float>(sf)));
                if (reduce > max_reduce) {
                    reduce_p = max_reduce;
                    redonly = false;
                }
                else {
                    reduce_p = reduce;
                    redonly = true;
                }
                break;
            }

            case IIIFSize::MAXDIM: {
                float fx = static_cast<float>(nx) / img_w_float;
                float fy = static_cast<float>(ny) / img_h_float;

                float r;
                if (fx < fy) { // scaling has to be done by fx
                    w = nx;
                    h = static_cast<size_t>(ceilf(img_h * fx));

                    r = img_w_float / static_cast<float>(w);
                } else { // scaling has to be done fy
                    w = static_cast<size_t>(ceilf(img_w * fy));
                    h = ny;

                    r = img_h_float / static_cast<float>(h);
                }
                if (!upscaling && (w > img_w || h > img_h)) {
                    throw IIIFSizeError(400, "Upscaling not allowed!");
                }
                float s = 1.0;
                reduce_p = 0;

                while ((2.0 * s <= r) && (reduce_p < max_reduce)) {
                    s *= 2.0;
                    reduce_p++;
                }

                if (fabsf(s - r) < 1.0e-5) {
                    redonly = true;
                }
                break;
            }

            case IIIFSize::FULL: {
                w = img_w;
                h = img_h;
                if (upscaling) {
                    if ((iiif_max_width > 0) && (iiif_max_height == 0)) {
                        if (img_w > img_h) {
                            w = iiif_max_width;
                            h = static_cast<size_t>(floorf(static_cast<float>(img_h*iiif_max_width) / static_cast<float>(img_w)));
                        }
                        else {
                            h = iiif_max_width;
                            w = static_cast<size_t>(floorf(static_cast<float>(img_w*iiif_max_width) / static_cast<float>(img_w)));
                        }
                    }
                    else if ((iiif_max_width > 0) && (iiif_max_height > 0)) {
                        if ((static_cast<float>(img_w) / static_cast<float>(iiif_max_width)) > (static_cast<float>(img_h) / (static_cast<float>(iiif_max_height)))) {
                            w = iiif_max_width;
                            float s = static_cast<float>(iiif_max_width) / static_cast<float>(img_w);
                            h = static_cast<size_t>(floorf(static_cast<float>(img_h)*s));
                        }
                        else {
                            h = iiif_max_height;
                            float s = static_cast<float>(iiif_max_height) / static_cast<float>(img_h);
                            w = static_cast<size_t>(floorf(static_cast<float>(img_w)*s));
                        }
                    }
                    else if (iiif_max_area > 0) {
                        float s = sqrtf(static_cast<float>(iiif_max_area) / static_cast<float>(img_w*img_h));
                        w = static_cast<size_t>(floorf(static_cast<float>(img_w)*s));
                        h = static_cast<size_t>(floorf(static_cast<float>(img_h)*s));
                    }
                }
                reduce_p = 0;
                redonly = true;
                break;
            }
        }

        w_p = w;
        h_p = h;

        size_t limit_nx = limitdim;
        size_t limit_ny = limitdim;
        if (iiif_max_width > 0) {
            limit_nx = iiif_max_width;
            limit_ny = iiif_max_height;
            if (iiif_max_height > 0) {
                limit_ny = iiif_max_height;
            }
        }
        if ((w_p > limit_nx) || (h_p > limit_ny)) {
            throw IIIFSizeError(400, "Requested image dimensions to big! (dims)!");
        }
        if (iiif_max_area > 0) {
            if (w_p*h_p > iiif_max_area) {
                throw IIIFSizeError(400, "Requested image dimensions to big (area)! ");
            }
        }

        if (reduce_p < 0) reduce_p = 0;
        redonly_p = redonly;
        canonical_ok = true;

        return size_type;
    }
    //-------------------------------------------------------------------------


    void IIIFSize::canonical(char *buf, int buflen) {
        int n;

        if (!canonical_ok && (size_type != IIIFSize::FULL)) {
            throw IIIFError(file_, __LINE__, "Canonical size not determined!");
        }

        switch (size_type) {
            case IIIFSize::UNDEFINED: {
                throw IIIFError(file_, __LINE__, "Error creating size string!");
            }
            case IIIFSize::PERCENTS:
            case IIIFSize::REDUCE:
            case IIIFSize::PIXELS_X:
            case IIIFSize::PIXELS_Y:
            case IIIFSize::PIXELS_XY:
            case IIIFSize::MAXDIM: {
                if (upscaling) {
                    n = snprintf(buf, buflen, "^%ld,%ld", w, h);
                } else {
                    n = snprintf(buf, buflen, "%ld,%ld", w, h);
                }
                break;
            }

            case IIIFSize::FULL: {
                if (upscaling) {
                    n = snprintf(buf, buflen, "^max");
                } else {
                    n = snprintf(buf, buflen, "max");
                }
            }
        }

        if ((n < 0) || (n >= buflen)) {
            throw IIIFError(file_, __LINE__, "Error creating size string!");
        }
    }
    //-------------------------------------------------------------------------

    //-------------------------------------------------------------------------
    // Output to stdout for debugging etc.
    //
    std::ostream &operator<<(std::ostream &outstr, const IIIFSize &rhs) {
        switch(rhs.size_type) {
            case IIIFSize::UNDEFINED:
                outstr << "  Size type: UNDEFINED";
                break;
            case IIIFSize::FULL:
                outstr << "  Size type: FULL";
                break;
            case IIIFSize::PIXELS_XY:
                outstr << "  Size type: PIXELS_XY";
                break;
            case IIIFSize::PIXELS_X:
                outstr << "  Size type: PIXELS_X";
                break;
            case IIIFSize::PIXELS_Y:
                outstr << "  Size type: PIXELS_Y";
                break;
            case IIIFSize::MAXDIM:
                outstr << "  Size type: MAXDIM";
                break;
            case IIIFSize::PERCENTS:
                outstr << "  Size type: PERCENTS";
                break;
            case IIIFSize::REDUCE:
                outstr << "  Size type: REDUCE";
                break;
        }
        outstr << " | percent = " << rhs.percent << " | nx = " << rhs.nx << " | ny = " << rhs.ny;
        outstr << " | w = " << rhs.w << " | h = " << rhs.h;
        outstr << " | reduce = " << rhs.reduce;
        return outstr;
    };
    //-------------------------------------------------------------------------

}
