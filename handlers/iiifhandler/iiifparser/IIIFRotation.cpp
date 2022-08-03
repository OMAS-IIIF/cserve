
#include <string>
#include <iostream>
#include <vector>

#include "../IIIFError.h"
#include "IIIFRotation.h"
#include "../../../lib/Parsing.h"
#include "fmt/format.h"

static const char file_[] = __FILE__;

namespace cserve {

    IIIFRotation::IIIFRotation() {
        mirror = false;
        rotation = 0.F;
   }

    IIIFRotation::IIIFRotation(std::string str) {
        try {
            if (str.empty()) {
                mirror = false;
                rotation = 0.F;
                return;
            }

            mirror = str[0] == '!';
            if (mirror) {
                str.erase(0, 1);
            }

            rotation = Parsing::parse_float(str);
        } catch (const Error &err) {
            throw IIIFError(file_, __LINE__, "Could not parse IIIF rotation parameter: " + str + " -> " + err.to_string());
        }
        if ((rotation < 0.0F) || (rotation >= 360.0F)) {
            throw IIIFError(file_, __LINE__, fmt::format("Angle {} not allowed - must be [0.0-360.0|", rotation));
        }
    }
    //-------------------------------------------------------------------------


    //-------------------------------------------------------------------------
    // Output to stdout for debugging etc.
    //
    std::ostream &operator<<(std::ostream &outstr, const IIIFRotation &rhs) {
        outstr << "IIIF-Server Rotation parameter:";
        outstr << "  Mirror " << rhs.mirror;
        outstr << " | rotation = " << std::to_string(rhs.rotation);
        return outstr;
    }
    //-------------------------------------------------------------------------
}
