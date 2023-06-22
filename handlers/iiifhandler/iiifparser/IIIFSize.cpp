
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

//
// Todo: Check side effect for ceilf -> floorf
//
static const char file_[] = __FILE__;

namespace cserve {

    static const float epsilon = 1.0e-4;

    size_t IIIFSize::limitdim = 32000;

    IIIFSize::IIIFSize(std::string str, uint32_t iiif_max_width, uint32_t iiif_max_height, uint32_t iiif_max_area)
            : iiif_max_width(iiif_max_width), iiif_max_height(iiif_max_height), iiif_max_area(iiif_max_area) {
        nx = 0;
        ny = 0;
        w = 0;
        h =0;
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
                if (exclamation_mark) throw IIIFError(file_, __LINE__, "Invalid IIIF size parameter: \"!" + str + R"(": "!" not allowed here!)");
                size_type = SizeType::PERCENTS;
                std::string percent_str = str.substr(4);
                percent = Parsing::parse_float(percent_str);
                if (percent < 0.0) percent = 1.0;
            } else if (str.find("red") != std::string::npos) {
                if (exclamation_mark) throw IIIFError(file_, __LINE__, "Invalid IIIF size parameter: \"!" + str + R"(": "!" not allowed here!)");
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
                    if (exclamation_mark) throw IIIFError(file_, __LINE__, "Invalid IIIF size parameter: \"!" + str + R"(": "!" not allowed here!)");
                    ny = Parsing::parse_int(height_str);
                    if (ny == 0) {
                        throw IIIFError(file_, __LINE__, "IIIF height cannot be zero");
                    }
                    size_type = SizeType::PIXELS_Y;
                } else if (height_str.empty()) { // "w," or "^w,"
                    if (exclamation_mark) throw IIIFError(file_, __LINE__, "Invalid IIIF size parameter: \"!" + str + R"(": "!" not allowed here!)");
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
        iiif_max_area = other.iiif_max_area;
        canonical_ok = other.iiif_max_width;
    }

    IIIFSize::IIIFSize(IIIFSize &&other) noexcept
            : size_type(SizeType::UNDEFINED), upscaling(false), percent(0.0F), reduce(0), redonly(false), nx(0), ny(0),
              w(0), h(0), iiif_max_height(0), iiif_max_width(0), iiif_max_area(0), canonical_ok(false) {
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
        iiif_max_area = other.iiif_max_area;
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

    IIIFSize &IIIFSize::operator=(IIIFSize &&other) noexcept{
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
            iiif_max_area = 0;
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
            iiif_max_area = other.iiif_max_area;
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
            other.iiif_max_area = 0;
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

    size_t IIIFSize::epsilon_ceil(float a) {
        if (fabs(floor(a) - a) < epsilon) {
            return static_cast<size_t>(floorf(a));
        }
        return static_cast<size_t>(ceilf(a));
    }

    size_t IIIFSize::epsilon_ceil_division(float a, float b) {
        //
        // epsilontic:
        // if a/b is x.00002, the result will be floorf(x), otherwise ceilf(x)
        //
        if (fabs(floorf(a/b) - (a/b)) < epsilon) {
            return static_cast<size_t>(floorf(a/b));
        }
        return static_cast<size_t>(ceilf(a/b));
    }

    size_t IIIFSize::epsilon_floor(float a) {
        if (fabs(ceilf(a) - a) < epsilon) {
            return static_cast<size_t>(ceilf(a));
        }
        return static_cast<size_t>(floorf(a));
    }

    size_t IIIFSize::epsilon_floor_division(float a, float b) {
        //
        // epsilontic:
        // if a/b is x.9998, the result will be ceilf(x), otherwise floor(x)
        //
        if (fabs(ceilf(a/b) - (a/b)) < epsilon) {
            return static_cast<size_t>(ceilf(a/b));
        }
        return static_cast<size_t>(floorf(a/b));
    }

    IIIFSize::SizeType IIIFSize::get_size(
            uint32_t img_w,
            uint32_t img_h,
            uint32_t &w_p,
            uint32_t &h_p,
            uint32_t &reduce_p,
            bool &redonly_p,
            bool is_j2k) {
        redonly = false;
        if (reduce_p <= 0) reduce_p = INT_MAX;
        size_t max_reduce = reduce_p;
        reduce_p = 0;
        redonly_p = false;

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
                uint32_t reduce_w;
                bool exact_match_w;
                if (nx > img_w) {
                    if (!upscaling) {
                        throw IIIFSizeError(400, "Upscaling not allowed!");
                    }
                    reduce_w = 1;
                    exact_match_w = false;
                } else {
                    reduce_w = epsilon_floor_division(img_w_float, static_cast<float>(nx));
                    if (is_j2k) {
                        size_t rtmp = 1;
                        do {
                            rtmp *= 2;
                        } while (rtmp < reduce_w);
                        reduce_w = rtmp / 2;
                    }
                    exact_match_w = img_w % nx == 0;
                }

                size_t reduce_h;
                bool exact_match_h;
                if (ny > img_h) {
                    if (!upscaling) {
                        throw IIIFSizeError(400, "Upscaling not allowed!");
                    }
                    reduce_h = 1;
                    exact_match_h = false;
                } else {
                    reduce_h = static_cast<uint32_t>(epsilon_floor_division(img_h_float, static_cast<float>(ny)));
                    if (is_j2k) {
                        size_t rtmp = 1;
                        do {
                            rtmp *= 2;
                        } while (rtmp < reduce_h);
                        reduce_h = rtmp / 2;
                    }
                    exact_match_h = img_h % ny == 0;
                }

                if (exact_match_w && exact_match_h && (reduce_w == reduce_h)) {
                    reduce_p = reduce_w;
                    redonly = true;
                } else {
                    reduce_p = reduce_w < reduce_h ? reduce_w : reduce_h; // min()
                }

                w = nx;
                h = ny;
                break;
            }
            case IIIFSize::PIXELS_X: {
                //
                // first we check how far the new image width can be reached by a reduce factor
                //
                uint32_t reduce_w;
                bool exact_match_w;
                if (nx > img_w) {
                    if (!upscaling) {
                        throw IIIFSizeError(400, "Upscaling not allowed!");
                    }
                    reduce_w = 1;
                    exact_match_w = false;
                } else {
                    reduce_w = static_cast<uint32_t>(epsilon_floor_division(img_w_float, static_cast<float>(nx)));
                    if (is_j2k) {
                        size_t rtmp = 1;
                        do {
                            rtmp *= 2;
                        } while (rtmp < reduce_w);
                        reduce_w = rtmp / 2;
                    }
                    exact_match_w = img_w % nx == 0;
                }
                w = nx;
                reduce_p = reduce_w;
                redonly = exact_match_w; // if exact_match, then reduce only

                if (exact_match_w) {
                    h = epsilon_ceil_division(img_h_float, static_cast<float>(reduce_w));
                } else {
                    h = epsilon_ceil_division(img_h_float * static_cast<float>(nx), img_w_float);
                }
                break;
            }
            case IIIFSize::PIXELS_Y: {
                //
                // first we check how far the new image height can be reached by a reduce factor
                //

                uint32_t reduce_h;
                bool exact_match_h;
                if (ny > img_h) {
                    if (!upscaling) {
                        throw IIIFSizeError(400, "Upscaling not allowed!");
                    }
                    reduce_h = 1;
                    exact_match_h = false;
                } else {
                    reduce_h = static_cast<uint32_t>(epsilon_floor_division(img_h_float, static_cast<float>(ny)));
                    if (is_j2k) {
                        size_t rtmp = 1;
                        do {
                            rtmp *= 2;
                        } while (rtmp < reduce_h);
                        reduce_h = rtmp / 2;
                    }
                    exact_match_h = img_h % ny == 0;
                }

                h = ny;
                reduce_p = reduce_h;
                redonly = exact_match_h; // if exact_match, then reduce only

                if (exact_match_h) {
                    w = epsilon_ceil_division(img_w_float, static_cast<float>(reduce_h));
                } else {
                    w = epsilon_ceil_division(img_w_float * static_cast<float>(ny), img_h_float);
                }
                break;
            }
            case IIIFSize::PERCENTS: {
                if (!upscaling && (percent > 100.0F)) {
                    throw IIIFSizeError(400, "Upscaling not allowed!");
                }
                w = epsilon_ceil_division(img_w_float * percent, 100.0F);
                h = epsilon_ceil_division(img_h_float * percent, 100.0F);

                reduce_p = static_cast<uint32_t>(epsilon_floor_division(100.0F, percent));
                if (reduce_p < 1) reduce_p = 1;
                if (is_j2k) {
                    size_t rtmp = 1;
                    do {
                        rtmp *= 2;
                    } while (rtmp < reduce_p);
                    reduce_p = rtmp / 2;
                }

                redonly = (abs(static_cast<int>(reduce_p*w - img_w)) < reduce_p) && (abs(static_cast<int>(reduce_p*h - img_h)) < reduce_p);
                break;
            }
            case IIIFSize::REDUCE: {
                if (reduce <= 1) {
                    w = img_w;
                    h = img_h;
                    reduce_p = 1;
                    redonly = true;
                    break;
                }

                w = epsilon_ceil_division(img_w_float, static_cast<float>(reduce));
                h = epsilon_ceil_division(img_h_float, static_cast<float>(reduce));
                if (reduce > max_reduce) {
                    reduce_p = max_reduce;
                    redonly = false;
                }
                else {
                    reduce_p = reduce;
                    if (is_j2k) {
                        size_t rtmp = 1;
                        do {
                            rtmp *= 2;
                        } while (rtmp < reduce_p);
                        reduce_p = rtmp / 2;
                    }
                    redonly = reduce == reduce_p;
                }
                break;
            }
            case IIIFSize::MAXDIM: {
                float fx = static_cast<float>(nx) / img_w_float;
                float fy = static_cast<float>(ny) / img_h_float;

                size_t reduce_w, reduce_h;
                if (fx < fy) { // scaling has to be done by fx
                    w = nx;
                    h = epsilon_ceil(img_h_float*fx);
                    reduce_w = epsilon_floor_division(img_w_float, static_cast<float>(w));
                    reduce_h = epsilon_floor_division(img_h_float, static_cast<float>(h));
                } else { // scaling has to be done fy
                    w = epsilon_ceil(img_w_float * fy);
                    h = ny;
                    reduce_w = epsilon_floor_division(img_w_float, static_cast<float>(w));
                    reduce_h = epsilon_floor_division(img_h_float, static_cast<float>(h));
                }
                if (reduce_w < 1) reduce_w = 1;
                if (reduce_h < 1) reduce_h = 1;
                if (is_j2k) {
                    size_t rtmp = 1;
                    do {
                        rtmp *= 2;
                    } while (rtmp < reduce_w);
                    reduce_w = rtmp / 2;
                    rtmp = 1;
                    do {
                        rtmp *= 2;
                    } while (rtmp < reduce_h);
                    reduce_h = rtmp / 2;
                }

                if (!upscaling && (w > img_w || h > img_h)) {
                    throw IIIFSizeError(400, "Upscaling not allowed!");
                }
                reduce_p = reduce_w < reduce_h ? reduce_w : reduce_h; // min()

                redonly = (reduce_w == reduce_h) &&
                        (abs(static_cast<int>(reduce_p*w - img_w)) < reduce_p) &&
                        (abs(static_cast<int>(reduce_p*h - img_h)) < reduce_p);
                break;
            }

            case IIIFSize::FULL: {
                w = img_w;
                h = img_h;
                if (upscaling) {
                    if ((iiif_max_width > 0) && (iiif_max_height == 0)) {
                        if (img_w > img_h) {
                            w = iiif_max_width;
                            h = epsilon_floor_division(static_cast<float>(img_h*iiif_max_width), static_cast<float>(img_w));
                        }
                        else {
                            h = iiif_max_width;
                            w = epsilon_floor_division(static_cast<float>(img_w*iiif_max_width), static_cast<float>(img_w));
                        }
                    }
                    else if ((iiif_max_width > 0) && (iiif_max_height > 0)) {
                        if ((img_w_float / static_cast<float>(iiif_max_width)) > (img_h_float / static_cast<float>(iiif_max_height))) {
                            w = iiif_max_width;
                            float s = static_cast<float>(iiif_max_width) / img_w_float;
                            h = epsilon_floor(img_h_float*s);
                        }
                        else {
                            h = iiif_max_height;
                            float s = static_cast<float>(iiif_max_height) / img_h_float;
                            w = epsilon_floor(img_w_float*s);
                        }
                    }
                    else if (iiif_max_area > 0) {
                        float s = sqrtf(static_cast<float>(iiif_max_area) / (img_w_float*img_h_float));
                        w = epsilon_floor(img_w_float*s);
                        h = epsilon_floor(img_h_float*s);
                    }
                    reduce_p = 1;
                    redonly = false;
                }
                else {
                    reduce_p = 1;
                    redonly = true;
                }
                break;
            }
        }

        w_p = w;
        h_p = h;

        uint32_t limit_nx = limitdim;
        uint32_t limit_ny = limitdim;
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
                    n = snprintf(buf, buflen, "^%u,%u", w, h);
                } else {
                    n = snprintf(buf, buflen, "%u,%u", w, h);
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
        outstr << " | reduce = " << rhs.reduce << " | upscaling = " << rhs.upscaling;
        return outstr;
    }
    //-------------------------------------------------------------------------

}
