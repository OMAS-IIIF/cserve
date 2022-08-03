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
#include "../IIIFError.h"
#include "IIIFIptc.h"
#include <stdlib.h>

static const char __file__[] = __FILE__;

namespace cserve {

    IIIFIptc::IIIFIptc(const IIIFIptc &rhs) {
        iptcData = rhs.iptcData;
    }

    IIIFIptc::IIIFIptc(IIIFIptc &&rhs) {
        iptcData = std::move(rhs.iptcData);
    }

    IIIFIptc::IIIFIptc(const unsigned char *iptc, unsigned int len) {
        if (Exiv2::IptcParser::decode(iptcData, iptc, (uint32_t) len) != 0) {
            throw IIIFError(__file__, __LINE__, "No valid IPTC data!");
        }
    }
    //============================================================================

    IIIFIptc::IIIFIptc(const std::vector<unsigned char> &iptc) {
        if (Exiv2::IptcParser::decode(iptcData, iptc.data(), iptc.size()) != 0) {
            throw IIIFError(__file__, __LINE__, "No valid IPTC data!");
        }
    }

    IIIFIptc::~IIIFIptc() {}

    IIIFIptc &IIIFIptc::operator=(const IIIFIptc &rhs) {
        if (this != &rhs) {
            iptcData = rhs.iptcData;
        }
        return *this;
    }

    IIIFIptc &IIIFIptc::operator=(IIIFIptc &&rhs) {
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

    std::vector<unsigned char> IIIFIptc::iptcBytes(void) {
        unsigned int len = 0;
        auto buf = iptcBytes(len);
        std::vector<unsigned char> data(buf.get(), buf.get() + len);
        return data;
    }
    //============================================================================

    std::ostream &operator<< (std::ostream &outstr, IIIFIptc &rhs) {
        Exiv2::IptcData::iterator end = rhs.iptcData.end();
        for (Exiv2::IptcData::iterator md = rhs.iptcData.begin(); md != end; ++md) {
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
