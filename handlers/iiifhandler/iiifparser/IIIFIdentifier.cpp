//
// Created by Lukas Rosenthaler on 2019-05-23.
//
#include <cstring>

#include "Connection.h"
#include "IIIFIdentifier.h"

static const char file_[] = __FILE__;

namespace cserve {

    void IIIFIdentifier::parse(const std::string &str) {
        identifier = urldecode(str);
    }

    IIIFIdentifier::IIIFIdentifier(const std::string &str) {
        parse(str);
    }

    IIIFIdentifier::IIIFIdentifier(const IIIFIdentifier &other) {
        identifier = other.identifier;
    }

    IIIFIdentifier::IIIFIdentifier(IIIFIdentifier &&other) noexcept {
        identifier = other.identifier;
        other.identifier.clear();
    }

    IIIFIdentifier &IIIFIdentifier::operator=(const IIIFIdentifier &other) {
        if (this != &other) {
            identifier = other.identifier;
        }
        return *this;
    }

    IIIFIdentifier &IIIFIdentifier::operator=(IIIFIdentifier &&other) {
        if (this != &other) {
            identifier = other.identifier;
            other.identifier.clear();
        }
        return *this;
    }
}
