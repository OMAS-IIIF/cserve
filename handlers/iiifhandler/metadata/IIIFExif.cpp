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
#include <climits>
#include <cmath>
#include "../IIIFError.h"
#include "IIIFExif.h"

static const char file_[] = __FILE__;

namespace cserve {

    IIIFExif::IIIFExif() {
        binaryExif = nullptr;
        binary_size = 0;
        byteorder = Exiv2::littleEndian; // that's today's default....
    }
    //============================================================================

    IIIFExif::IIIFExif(const IIIFExif &other) {
        binary_size = other.binary_size;
        byteorder = other.byteorder;
        binaryExif = std::make_unique<unsigned char[]>(binary_size);
        memcpy (binaryExif.get(), other.binaryExif.get(), binary_size);
        exifData = other.exifData;
    }

    IIIFExif::IIIFExif(IIIFExif &&other) noexcept {
        binary_size = other.binary_size;
        byteorder = other.byteorder;
        binaryExif = other.binaryExif;
        exifData = std::move(other.exifData);

        other.binary_size = 0;
        other.byteorder = Exiv2::littleEndian;
        other.binaryExif = nullptr;
        other.exifData.clear();
    }

    IIIFExif::IIIFExif(const unsigned char *exif, unsigned int len) {
        //
        // first we save the binary exif... we use it later for constructing a binary exif again!
        //
        binaryExif = std::make_unique<unsigned char[]>(len);
        memcpy (binaryExif.get(), exif, len);
        binary_size = len;

        //
        // now we decode the binary exif
        //
        try {
            byteorder = Exiv2::ExifParser::decode(exifData, exif, (uint32_t) len);
        }
        catch(const Exiv2::Error &exiverr) {
            throw IIIFError(file_, __LINE__, exiverr.what());
        }
    }
    //============================================================================

    IIIFExif::~IIIFExif() = default;

    IIIFExif &IIIFExif::operator=(const IIIFExif &other) {
        if (this != &other) {
            binary_size = other.binary_size;
            byteorder = other.byteorder;
            binaryExif = std::make_unique<unsigned char[]>(binary_size);
            memcpy(binaryExif.get(), other.binaryExif.get(), binary_size);
            exifData = other.exifData;
        }
        return *this;
    }

    IIIFExif &IIIFExif::operator=(IIIFExif &&other) noexcept{
        if (this != &other) {
            binary_size = other.binary_size;
            byteorder = other.byteorder;
            binaryExif = other.binaryExif;
            exifData = std::move(other.exifData);

            other.binary_size = 0;
            other.byteorder = Exiv2::littleEndian;
            other.binaryExif = nullptr;
            other.exifData.clear();
        }
        return *this;
    }


    std::shared_ptr<unsigned char[]> IIIFExif::exifBytes(unsigned int &len) {
        Exiv2::Blob blob;
        Exiv2::WriteMethod wm = Exiv2::ExifParser::encode(blob, binaryExif.get(), binary_size, byteorder, exifData);
        if (wm == Exiv2::wmIntrusive) {
            binary_size = blob.size();
            std::shared_ptr<unsigned char[]> tmpbuf(new unsigned char[binary_size]);
            memcpy (tmpbuf.get(), blob.data(), binary_size);
            binaryExif = tmpbuf;
        }
        len = binary_size;
        return binaryExif;
    }
    //============================================================================

    std::vector<unsigned char> IIIFExif::exifBytes() {
        unsigned int len = 0;
        auto buf = exifBytes(len);
        std::vector<unsigned char> data(buf.get(), buf.get() + len);
        return data;
    }
    //============================================================================

    Exiv2::Rational IIIFExif::toRational(double f) {
        int32_t numerator;
        int32_t denominator;
        if (f == 0.0) {
            numerator = 0;
            denominator = 1;
        }
        else if (f == floor(f)) {
            numerator = (int) f;
            denominator = 1;
        }
        else if (f > 0.0) {
            if (f < 1.0) {
                numerator = static_cast<int32_t>(lround(f*static_cast<double>(LONG_MAX)));
                denominator = INT_MAX;
            }
            else {
                numerator = INT_MAX;
                denominator = static_cast<int32_t>(lround(static_cast<double>(INT_MAX ) / f));
            }
        }
        else {
            if (f > -1.0F) {
                numerator = static_cast<int32_t>(lround(f*static_cast<double>(INT_MAX)));
                denominator = INT_MAX;
            }
            else {
                numerator = INT_MAX;
                denominator = static_cast<int32_t>(lround(static_cast<double>(INT_MAX )/ f));
            }
        }
        return std::make_pair(numerator, denominator);
    }
    //============================================================================

    Exiv2::URational IIIFExif::toURational(double f) {
        uint32_t numerator;
        uint32_t denominator;

        if (f < 0.0) throw IIIFError(file_, __LINE__, "Cannot convert negative float to URational!");

        if (f == 0.0) {
            numerator = 0;
            denominator = 1;
        }
        else if (f == floor(f)) {
            numerator = (int) f;
            denominator = 1;
        }
        if (f < 1.0F) {
            numerator = static_cast<uint32_t>(lround(f*static_cast<double>(UINT_MAX)));
            denominator = UINT_MAX;
        }
        else {
            numerator = UINT_MAX;
            denominator = static_cast<uint32_t>(lround(static_cast<double>(UINT_MAX) / f));
        }
        return std::make_pair(numerator, denominator);
    }
    //============================================================================

    std::ostream &operator<< (std::ostream &outstr, IIIFExif &rhs) {
        auto end = rhs.exifData.end();
        for (auto i = rhs.exifData.begin(); i != end; ++i) {
            const char* tn = i->typeName();
            outstr << std::setw(44) << std::setfill(' ') << std::left
                << i->key() << " "
                << "0x" << std::setw(4) << std::setfill('0') << std::right
                << std::hex << i->tag() << " "
                << std::setw(9) << std::setfill(' ') << std::left
                << (tn ? tn : "Unknown") << " "
                << std::dec << std::setw(3)
                << std::setfill(' ') << std::right
                << i->count() << "  "
                << std::dec << i->value()
                << "\n";
        }
        return outstr;
    }
    //============================================================================

}
