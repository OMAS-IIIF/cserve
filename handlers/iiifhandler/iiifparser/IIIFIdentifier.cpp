//
// Created by Lukas Rosenthaler on 2019-05-23.
//
#include <cstring>

#include "Connection.h"
#include "IIIFIdentifier.h"

static const char file_[] = __FILE__;

namespace cserve {

    void IIIFIdentifier::parse(const std::string &str) {
        size_t pos;
        if ((pos = str.find('@')) != std::string::npos) {
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

    IIIFIdentifier::IIIFIdentifier(const std::string &str) {
        parse(str);
    }

    IIIFIdentifier::IIIFIdentifier(const IIIFIdentifier &other) {
        identifier = other.identifier;
        page = other.page;
    }

    IIIFIdentifier::IIIFIdentifier(IIIFIdentifier &&other) noexcept {
        identifier = other.identifier;
        page = other.page;
        other.identifier.clear();
        other.page = 0;
    }

    IIIFIdentifier &IIIFIdentifier::operator=(const IIIFIdentifier &other) {
        if (this != &other) {
            identifier = other.identifier;
            page = other.page;
        }
        return *this;
    }

    IIIFIdentifier &IIIFIdentifier::operator=(IIIFIdentifier &&other) {
        if (this != &other) {
            identifier = other.identifier;
            page = other.page;
            other.identifier.clear();
            other.page = 0;
        }
        return *this;
    }
}
