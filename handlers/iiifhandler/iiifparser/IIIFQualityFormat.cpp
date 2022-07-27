#include <assert.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <cmath>

#include <stdio.h>
#include <string.h>

#include "../IIIFError.h"
#include "IIIFQualityFormat.h"


#include <string>

static const char __file__[] = __FILE__;

namespace cserve {

    IIIFQualityFormat::IIIFQualityFormat(const std::string &quality, const std::string &format) {
        if (quality.empty() || format.empty()) {
            quality_type = IIIFQualityFormat::DEFAULT;
            format_type = IIIFQualityFormat::JPG;
            return;
        }


        if (quality == "default") {
            quality_type = IIIFQualityFormat::DEFAULT;
        } else if (quality == "color") {
            quality_type = IIIFQualityFormat::COLOR;
        } else if (quality == "gray") {
            quality_type = IIIFQualityFormat::GRAY;
        } else if (quality == "bitonal") {
            quality_type = IIIFQualityFormat::BITONAL;
        } else {
            throw IIIFError(__file__, __LINE__, "IIIF Error reading Quality parameter  \"" + quality + "\" !");
        }

        if (format == "jpg") {
            format_type = IIIFQualityFormat::JPG;
        } else if (format == "tif") {
            format_type = IIIFQualityFormat::TIF;
        } else if (format == "png") {
            format_type = IIIFQualityFormat::PNG;
        } else if (format == "gif") {
            format_type = IIIFQualityFormat::GIF;
        } else if (format == "jp2") {
            format_type = IIIFQualityFormat::JP2;
        } else if (format == "pdf") {
            format_type = IIIFQualityFormat::PDF;
        } else if (format == "webp") {
            format_type = IIIFQualityFormat::WEBP;
        } else {
            format_type = IIIFQualityFormat::UNSUPPORTED;
        }
    }


    //-------------------------------------------------------------------------
    // Output to stdout for debugging etc.
    //
    std::ostream &operator<<(std::ostream &outstr, const IIIFQualityFormat &rhs) {
        outstr << "IIIF-Server QualityFormat parameter: ";
        outstr << "  Quality: " << rhs.quality_type;
        outstr << " | Format: " << rhs.format_type;
        return outstr;
    }
    //-------------------------------------------------------------------------

}
