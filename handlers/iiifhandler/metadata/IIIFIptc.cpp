/*
 * Copyright © 2022 Lukas Rosenthaler
 * This file is part of OMAS/cserve
 * OMAS/cserve is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * OMAS/cserve is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#include <cstdlib>

#include "../IIIFError.h"
#include "IIIFIptc.h"


static const char file_[] = __FILE__;

namespace cserve {


    IIIFIptc::IIIFIptc(const IIIFIptc &rhs) {
        iptcData = rhs.iptcData;
    }

    IIIFIptc::IIIFIptc(IIIFIptc &&rhs) noexcept{
        iptcData = std::move(rhs.iptcData);
    }

    IIIFIptc::IIIFIptc(const unsigned char *iptc, unsigned int len) {
        if (Exiv2::IptcParser::decode(iptcData, iptc, (uint32_t) len) != 0) {
            throw IIIFError(file_, __LINE__, "No valid IPTC data!");
        }
    }

    IIIFIptc::IIIFIptc(const std::vector<unsigned char> &iptc) {
        if (Exiv2::IptcParser::decode(iptcData, iptc.data(), iptc.size()) != 0) {
            throw IIIFError(file_, __LINE__, "No valid IPTC data!");
        }
    }

    IIIFIptc::IIIFIptc(const char *hexbuf) {
        if ((strlen(hexbuf) % 2) != 0) {
            throw IIIFError(file_, __LINE__, "No valid HEX-form IPTC data !");
        }
        char hexnum[]{'\0', '\0', '\0'};
        std::vector<unsigned char> buf(strlen(hexbuf)/2);
        for (int i = 0; i < strlen(hexbuf); i += 2) {
            hexnum[0] = hexbuf[i];
            hexnum[1] = hexbuf[i + 1];
            char *e;
            long c = strtol(hexnum, &e, 16);
            buf.push_back(c);
        }
        if (Exiv2::IptcParser::decode(iptcData, buf.data(), buf.size()) != 0) {
            throw IIIFError(file_, __LINE__, "No valid IPTC data!");
        }
    }

    IIIFIptc::~IIIFIptc() = default;

    IIIFIptc &IIIFIptc::operator=(const IIIFIptc &rhs) {
        if (this != &rhs) {
            iptcData = rhs.iptcData;
        }
        return *this;
    }

    IIIFIptc &IIIFIptc::operator=(IIIFIptc &&rhs) noexcept {
        if (this != &rhs) {
            iptcData = std::move(rhs.iptcData);
        }
        return *this;
    }

    std::unique_ptr<unsigned char[]> IIIFIptc::iptcBytes(unsigned int &len) {
        Exiv2::DataBuf databuf = Exiv2::IptcParser::encode(iptcData);
        auto buf = std::make_unique<unsigned char[]>(databuf.size());
        //unsigned char *buf = new unsigned char[databuf.size()];
        memcpy (buf.get(), databuf.data(), databuf.size());
        len = databuf.size();
        return buf;
    }
    //============================================================================

    std::vector<unsigned char> IIIFIptc::iptcBytes() {
        unsigned int len = 0;
        auto buf = iptcBytes(len);
        std::vector<unsigned char> data(buf.get(), buf.get() + len);
        return data;
    }
    //============================================================================

    std::unique_ptr<char[]> IIIFIptc::iptcHexBytes(unsigned int &len) {
        char hex[]{'0', '1', '2', '3', '4', '5', '6','7',
                   '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

        Exiv2::DataBuf databuf = Exiv2::IptcParser::encode(iptcData);
        auto buf = std::make_unique<char[]>(2*databuf.size() + 1);
        int i = 0;
        for (auto b: databuf) {
            buf[2*i] = hex[(b & 0xF0) >> 4];
            buf[2*i + 1] = hex[b & 0x0F];
            ++i;
        }
        buf[2*i] = '\0';
        len = 2*databuf.size();
        return buf;
    }
    //============================================================================


    std::ostream &operator<< (std::ostream &outstr, IIIFIptc &rhs) {
        auto end = rhs.iptcData.end();
        for (auto md = rhs.iptcData.begin(); md != end; ++md) {
            outstr << std::setw(44) << std::setfill(' ') << std::left
                << md->key() << " "
                << "0x" << std::setw(4) << std::setfill('0') << std::right
                << std::hex << md->tag() << " "
                << std::setw(9) << std::setfill(' ') << std::left
                << md->typeName() << " "
                << std::dec << std::setw(3)
                << std::setfill(' ') << std::right
                << md->count() << "  "
                << std::dec << md->value()
                << std::endl;
        }
        return outstr;
    }
    //============================================================================

}
