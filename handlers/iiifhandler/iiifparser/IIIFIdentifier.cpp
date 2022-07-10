//
// Created by Lukas Rosenthaler on 2019-05-23.
//
#include <cstdlib>
#include <cstring>

#include "Connection.h"
#include "../IIIFError.h"
#include "IIIFIdentifier.h"

static const char __file__[] = __FILE__;

namespace cserve {

    IIIFIdentifier::IIIFIdentifier(const std::string &str) {
        size_t pos;
        if ((pos = str.find("@")) != std::string::npos) {
            identifier = urldecode(str.substr(0, pos));
            try {
                page = stoi(str.substr(pos + 1));
            }
            catch(const std::invalid_argument& ia) {
                page = 0;
            }
            catch(const std::out_of_range& oor) {
                page = 0;
            }
        }
        else {
            identifier = urldecode(str);
            page = 0;
        }
    }
}
