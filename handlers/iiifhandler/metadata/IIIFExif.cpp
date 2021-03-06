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
    };
    //============================================================================

    IIIFExif::IIIFExif(const IIIFExif &other) {
        binary_size = other.binary_size;
        byteorder = other.byteorder;
        binaryExif = new unsigned char[binary_size];
        memcpy (binaryExif, other.binaryExif, binary_size);
        exifData = other.exifData;
    }

    IIIFExif::IIIFExif(IIIFExif &&other) {
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
        binaryExif = new unsigned char[len];
        memcpy (binaryExif, exif, len);
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

    IIIFExif::~IIIFExif() {
        delete [] binaryExif;
    }

    IIIFExif &IIIFExif::operator=(const IIIFExif &other) {
        if (this != &other) {
            binary_size = other.binary_size;
            byteorder = other.byteorder;
            binaryExif = new unsigned char[binary_size];
            memcpy (binaryExif, other.binaryExif, binary_size);
            exifData = other.exifData;
        }
        return *this;
    }

    IIIFExif &IIIFExif::operator=(IIIFExif &&other) {
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


    unsigned char * IIIFExif::exifBytes(unsigned int &len) {
        unsigned char *tmpbuf = new unsigned char[binary_size];
        memcpy (tmpbuf, binaryExif, binary_size);

        Exiv2::Blob blob;
        Exiv2::WriteMethod wm = Exiv2::ExifParser::encode(blob, binaryExif, binary_size, byteorder, exifData);
        unsigned char *buf;
        if (wm == Exiv2::wmIntrusive) {
            // we use blob
            len = blob.size();
            buf = new unsigned char[len];
            memcpy (buf, blob.data(), len);
            delete [] tmpbuf; // cleanup tmpbuf!
            /*
            for (int i = 0; i < len; i++) {
                cerr << i << " => " << hex << blob[i];
                if (isprint(blob[i])) cerr << " \"" << blob[i] << "\"";
                cerr << endl;
            }
            */
        }
        else {
            buf = tmpbuf;
            len = binary_size;
        }
        return buf;
    }
    //============================================================================

    std::vector<unsigned char> IIIFExif::exifBytes(void) {
        unsigned int len = 0;
        unsigned char *buf = exifBytes(len);
        std::vector<unsigned char> data;
        if (buf != nullptr) {
            data.reserve(len);
            for (int i = 0; i < len; i++) data.push_back(buf[i]);
            delete[] buf;
        }
        return data;
    }
    //============================================================================

    Exiv2::Rational IIIFExif::toRational(float f) {
        int numerator;
        int denominator;
        if (f == 0.0F) {
            numerator = 0;
            denominator = 1;
        }
        else if (f == floorf(f)) {
            numerator = (int) f;
            denominator = 1;
        }
        else if (f > 0.0F) {
            if (f < 1.0F) {
                numerator = (int) (f*LONG_MAX);
                denominator = INT_MAX;
            }
            else {
                numerator = INT_MAX;
                denominator = (int) (INT_MAX / f);
            }
        }
        else {
            if (f > -1.0F) {
                numerator = (int) (f*INT_MAX);
                denominator = INT_MAX;
            }
            else {
                numerator = INT_MAX;
                denominator = (int) (INT_MAX / f);
            }
        }
        return std::make_pair(numerator, denominator);
    }
    //============================================================================

    Exiv2::URational IIIFExif::toURational(float f) {
        unsigned int numerator;
        unsigned int denominator;

        if (f < 0.0F) throw IIIFError(file_, __LINE__, "Cannot convert negative float to URational!");

        if (f == 0.0F) {
            numerator = 0;
            denominator = 1;
        }
        else if (f == (float)((int) f)) {
            numerator = (int) f;
            denominator = 1;
        }
        if (f < 1.0F) {
            numerator = (int) (f*UINT_MAX);
            denominator = UINT_MAX;
        }
        else {
            numerator = UINT_MAX;
            denominator = (int) (UINT_MAX / f);
        }
        return std::make_pair(numerator, denominator);
    }
    //============================================================================

    void IIIFExif::addKeyVal(const std::string &key_p, const std::string &val_p) {
        exifData[key_p.c_str()] = val_p.c_str();
    }
    //============================================================================

    void IIIFExif::addKeyVal(uint16_t tag, const std::string &groupName, const std::string &val_p) {
        Exiv2::ExifKey key = Exiv2::ExifKey(tag, groupName);
        Exiv2::Value::UniquePtr v = Exiv2::Value::create(Exiv2::asciiString);
        v->read((unsigned char *) val_p.c_str(), val_p.size(), Exiv2::littleEndian);
        exifData.add(key, v.get());
    }
    //============================================================================

    //............................................................................
    // float values and float arrays
    //
    void IIIFExif::addKeyVal(const std::string &key_p, const float &val_p) {
        exifData[key_p] = toRational(val_p);
    }
    //============================================================================

    void IIIFExif::addKeyVal(const std::string &key_p, const float *vals_p, unsigned int len) {
        Exiv2::Value::UniquePtr v = Exiv2::Value::create(Exiv2::tiffFloat);
        v->read((unsigned char *) vals_p, len*sizeof(float), Exiv2::littleEndian);
        Exiv2::ExifKey key(key_p);
        exifData.add(key, v.get());
    }
    //============================================================================

    void IIIFExif::addKeyVal(uint16_t tag, const std::string &groupName, const float &val_p) {
        Exiv2::ExifKey key = Exiv2::ExifKey(tag, groupName);
        Exiv2::Value::UniquePtr v = Exiv2::Value::create(Exiv2::tiffFloat);
        v->read((unsigned char *) &val_p, sizeof(float), Exiv2::littleEndian);
        exifData.add(key, v.get());
    }
    //============================================================================

    void IIIFExif::addKeyVal(uint16_t tag, const std::string &groupName, const float *vals_p, unsigned int len) {
        Exiv2::Value::UniquePtr v = Exiv2::Value::create(Exiv2::tiffFloat);
        v->read((unsigned char *) vals_p, len*sizeof(float), Exiv2::littleEndian);
        Exiv2::ExifKey key = Exiv2::ExifKey(tag, groupName);
        exifData.add(key, v.get());
    }
    //============================================================================

    //............................................................................
    // short values and short arrays
    //
    void IIIFExif::addKeyVal(const std::string &key_p, const short &val_p) {
        exifData[key_p] = val_p;
    }
    //============================================================================

    void IIIFExif::addKeyVal(const std::string &key_p, const short *vals_p, unsigned int len) {
        Exiv2::Value::UniquePtr v = Exiv2::Value::create(Exiv2::signedShort);
        v->read((unsigned char *) vals_p, len*sizeof(short), Exiv2::littleEndian);
        Exiv2::ExifKey key(key_p);
        exifData.add(key, v.get());
    }
    //============================================================================

    void IIIFExif::addKeyVal(uint16_t tag, const std::string &groupName, const short &val_p) {
        Exiv2::ExifKey key = Exiv2::ExifKey(tag, groupName);
        Exiv2::Value::UniquePtr v = Exiv2::Value::create(Exiv2::signedShort );
        v->read((unsigned char *) &val_p, sizeof(short), Exiv2::littleEndian);
        exifData.add(key, v.get());
    }
    //============================================================================

    void IIIFExif::addKeyVal(uint16_t tag, const std::string &groupName, const short *vals_p, unsigned int len) {
        Exiv2::Value::UniquePtr v = Exiv2::Value::create(Exiv2::signedShort);
        v->read((unsigned char *) vals_p, len*sizeof(short), Exiv2::littleEndian);
        Exiv2::ExifKey key = Exiv2::ExifKey(tag, groupName);
        exifData.add(key, v.get());
    }
    //============================================================================

    //............................................................................
    // unsigned short values and unsigned short arrays
    //
    void IIIFExif::addKeyVal(const std::string &key_p, const unsigned short &val_p) {
        exifData[key_p] = val_p;
    }
    //============================================================================

    void IIIFExif::addKeyVal(const std::string &key_p, const unsigned short *vals_p, int unsigned len) {
        Exiv2::Value::UniquePtr v = Exiv2::Value::create(Exiv2::unsignedShort);
        v->read((unsigned char *) vals_p, len*sizeof(unsigned short), Exiv2::littleEndian);
        Exiv2::ExifKey key(key_p);
        exifData.add(key, v.get());
    }
    //============================================================================

    void IIIFExif::addKeyVal(uint16_t tag, const std::string &groupName, const unsigned short &val_p) {
        Exiv2::ExifKey key = Exiv2::ExifKey(tag, groupName);
        Exiv2::Value::UniquePtr v = Exiv2::Value::create(Exiv2::unsignedShort);
        v->read((unsigned char *) &val_p, sizeof(unsigned short), Exiv2::littleEndian);
        exifData.add(key, v.get());
    }
    //============================================================================


    void IIIFExif::addKeyVal(uint16_t tag, const std::string &groupName, const unsigned short *vals_p, unsigned int len) {
        Exiv2::Value::UniquePtr v = Exiv2::Value::create(Exiv2::unsignedShort);
        v->read((unsigned char *) vals_p, len*sizeof(unsigned short), Exiv2::littleEndian);
        Exiv2::ExifKey key = Exiv2::ExifKey(tag, groupName);
        exifData.add(key, v.get());
    }
    //============================================================================


    //............................................................................
    // int values and int arrays
    //
    void IIIFExif::addKeyVal(const std::string &key_p, const int &val_p) {
        exifData[key_p] = val_p;
    }
    //============================================================================

    void IIIFExif::addKeyVal(const std::string &key_p, const int *vals_p, int unsigned len) {
        Exiv2::Value::UniquePtr v = Exiv2::Value::create(Exiv2::signedLong);
        v->read((unsigned char *) vals_p, len*sizeof(int), Exiv2::littleEndian);
        Exiv2::ExifKey key(key_p);
        exifData.add(key, v.get());
    }
    //============================================================================

    void IIIFExif::addKeyVal(uint16_t tag, const std::string &groupName, const int &val_p) {
        Exiv2::ExifKey key = Exiv2::ExifKey(tag, groupName);
        Exiv2::Value::UniquePtr v = Exiv2::Value::create(Exiv2::signedLong);
        v->read((unsigned char *) &val_p, sizeof(int), Exiv2::littleEndian);
        exifData.add(key, v.get());
    }
    //============================================================================

    void IIIFExif::addKeyVal(uint16_t tag, const std::string &groupName, const int *vals_p, unsigned int len) {
        Exiv2::Value::UniquePtr v = Exiv2::Value::create(Exiv2::signedLong);
        v->read((unsigned char *) vals_p, len*sizeof(int), Exiv2::littleEndian);
        Exiv2::ExifKey key = Exiv2::ExifKey(tag, groupName);
        exifData.add(key, v.get());
    }
    //============================================================================

    //............................................................................
    // unsigned int values and unsigned int arrays
    //
    void IIIFExif::addKeyVal(const std::string &key_p, const unsigned int &val_p) {
        exifData[key_p] = val_p;
    }
    //============================================================================

    void IIIFExif::addKeyVal(const std::string &key_p, const unsigned int *vals_p, unsigned int len) {
        Exiv2::Value::UniquePtr v = Exiv2::Value::create(Exiv2::unsignedLong);
        v->read((unsigned char *) vals_p, len*sizeof(unsigned int), Exiv2::littleEndian);
        Exiv2::ExifKey key(key_p);
        exifData.add(key, v.get());
    }
    //============================================================================

    void IIIFExif::addKeyVal(uint16_t tag, const std::string &groupName, const unsigned int &val_p) {
        Exiv2::ExifKey key = Exiv2::ExifKey(tag, groupName);
        Exiv2::Value::UniquePtr v = Exiv2::Value::create(Exiv2::unsignedLong);
        v->read((unsigned char *) &val_p, sizeof(unsigned int), Exiv2::littleEndian);
        exifData.add(key, v.get());
    }
    //============================================================================

    void IIIFExif::addKeyVal(uint16_t tag, const std::string &groupName, const unsigned int *vals_p, unsigned int len) {
        Exiv2::Value::UniquePtr v = Exiv2::Value::create(Exiv2::unsignedLong);
        v->read((unsigned char *) vals_p, len*sizeof(unsigned int), Exiv2::littleEndian);
        Exiv2::ExifKey key = Exiv2::ExifKey(tag, groupName);
        exifData.add(key, v.get());
    }
    //============================================================================

    //............................................................................
    // Rational values and Rational arrays
    //
    void IIIFExif::addKeyVal(const std::string &key_p, const Exiv2::Rational &r) {
        exifData[key_p] = r;
    }
    //============================================================================

    void IIIFExif::addKeyVal(const std::string &key_p, const Exiv2::Rational *vals_p, unsigned int len) {
        Exiv2::Value::UniquePtr v = Exiv2::Value::create(Exiv2::signedRational);
        v->read((unsigned char *) vals_p, len*sizeof(Exiv2::Rational), Exiv2::littleEndian);
        Exiv2::ExifKey key(key_p);
        exifData.add(key, v.get());
    }
    //============================================================================

    void IIIFExif::addKeyVal(const std::string &key_p, const int &val1_p, const int &val2_p) {
        exifData[key_p] = Exiv2::Rational(val1_p, val2_p);
    }
    //============================================================================

    void IIIFExif::addKeyVal(uint16_t tag, const std::string &groupName, const Exiv2::Rational &r) {
        Exiv2::ExifKey key = Exiv2::ExifKey(tag, groupName);
        Exiv2::Value::UniquePtr v = Exiv2::Value::create(Exiv2::signedRational);
        v->read((unsigned char *) &r, sizeof(Exiv2::Rational), Exiv2::littleEndian);
        exifData.add(key, v.get());
    }
    //============================================================================

    void IIIFExif::addKeyVal(uint16_t tag, const std::string &groupName, const Exiv2::Rational *vals_p, unsigned int len) {
        Exiv2::Value::UniquePtr v = Exiv2::Value::create(Exiv2::signedRational);
        v->read((unsigned char *) vals_p, len*sizeof(Exiv2::Rational), Exiv2::littleEndian);
        Exiv2::ExifKey key = Exiv2::ExifKey(tag, groupName);
        exifData.add(key, v.get());
    }
    //============================================================================

    void IIIFExif::addKeyVal(uint16_t tag, const std::string &groupName, const int &val1_p, const int &val2_p) {
        Exiv2::ExifKey key = Exiv2::ExifKey(tag, groupName);
        Exiv2::Value::UniquePtr v = Exiv2::Value::create(Exiv2::signedRational);
        Exiv2::Rational val = Exiv2::Rational(val1_p, val2_p);
        v->read((unsigned char *) &val, sizeof(Exiv2::Rational), Exiv2::littleEndian);
        exifData.add(key, v.get());
    }
    //============================================================================

    //............................................................................
    // unsigned Rational values and unsigned Rational arrays
    //
    void IIIFExif::addKeyVal(const std::string &key_p, const Exiv2::URational &ur) {
        exifData[key_p] = ur;
    }
    //============================================================================

    void IIIFExif::addKeyVal(const std::string &key_p, const Exiv2::URational *vals_p, unsigned int len) {
        Exiv2::Value::UniquePtr v = Exiv2::Value::create(Exiv2::unsignedRational);
        v->read((unsigned char *) vals_p, len*sizeof(Exiv2::URational), Exiv2::littleEndian);
        Exiv2::ExifKey key(key_p);
        exifData.add(key, v.get());
    }
    //============================================================================

    void IIIFExif::addKeyVal(const std::string &key_p, const unsigned int &val1_p, const unsigned int &val2_p) {
        exifData[key_p] = Exiv2::URational(val1_p, val2_p);
    }
    //============================================================================

    void IIIFExif::addKeyVal(uint16_t tag, const std::string &groupName, const Exiv2::URational &ur) {
        Exiv2::ExifKey key = Exiv2::ExifKey(tag, groupName);
        Exiv2::Value::UniquePtr v = Exiv2::Value::create(Exiv2::unsignedRational);
        v->read((unsigned char *) &ur, sizeof(Exiv2::URational), Exiv2::littleEndian);
        exifData.add(key, v.get());
    }
    //============================================================================

    void IIIFExif::addKeyVal(uint16_t tag, const std::string &groupName, const Exiv2::URational *vals_p, unsigned int len) {
        Exiv2::Value::UniquePtr v = Exiv2::Value::create(Exiv2::unsignedRational);
        v->read((unsigned char *) vals_p, len*sizeof(Exiv2::URational), Exiv2::littleEndian);
        Exiv2::ExifKey key = Exiv2::ExifKey(tag, groupName);
        exifData.add(key, v.get());
    }
    //============================================================================

    void IIIFExif::addKeyVal(uint16_t tag, const std::string &groupName, const unsigned int &val1_p, const unsigned int &val2_p) {
        Exiv2::ExifKey key = Exiv2::ExifKey(tag, groupName);
        Exiv2::Value::UniquePtr v = Exiv2::Value::create(Exiv2::unsignedRational);
        Exiv2::URational val = Exiv2::URational(val1_p, val2_p);
        v->read((unsigned char *) &val, sizeof(Exiv2::URational), Exiv2::littleEndian);
        exifData.add(key, v.get());
    }
    //============================================================================


    //............................................................................
    // Undefined data
    //
    void IIIFExif::addKeyVal(const std::string &key_p, const unsigned char *vals_p, unsigned int len) {
        Exiv2::Value::UniquePtr v = Exiv2::Value::create(Exiv2::undefined);
        v->read(vals_p, len*sizeof(unsigned char), Exiv2::littleEndian);
        Exiv2::ExifKey key(key_p);
        exifData.add(key, v.get());
    }
    //============================================================================

    void IIIFExif::addKeyVal(uint16_t tag, const std::string &groupName, const unsigned char *vals_p, unsigned int len) {
        Exiv2::ExifKey key = Exiv2::ExifKey(tag, groupName);
        Exiv2::Value::UniquePtr v = Exiv2::Value::create(Exiv2::undefined);
        v->read(vals_p, len*sizeof(unsigned char), Exiv2::littleEndian);
        exifData.add(key, v.get());
    }
    //============================================================================

    //............................................................................
    // Getting values from the EXIF object
    //

    //____________________________________________________________________________
    // string values
    //
    bool IIIFExif::getValByKey(const std::string key_p, std::string &str_p) {
        Exiv2::ExifKey key = Exiv2::ExifKey(key_p);
        Exiv2::ExifData::iterator pos = exifData.findKey(key);
        if (pos == exifData.end()) {
            return false;
        }
        Exiv2::Value::UniquePtr v = pos->getValue();
        str_p = v->toString();
        return v->ok();
    }
    //============================================================================


    bool IIIFExif::getValByKey(uint16_t tag, const std::string &groupName, std::string &str_p) {
        Exiv2::ExifKey key = Exiv2::ExifKey(tag, groupName);
        Exiv2::ExifData::iterator pos = exifData.findKey(key);
        if (pos == exifData.end()) {
            return false;
        }
        Exiv2::Value::UniquePtr v = pos->getValue();
        str_p = v->toString();
        return v->ok();
    }
    //============================================================================

    bool IIIFExif::getValByKey(uint16_t tag, const std::string &groupName, std::vector<std::string> &str_p) {
        Exiv2::ExifKey key = Exiv2::ExifKey(tag, groupName);
        Exiv2::ExifData::iterator pos = exifData.findKey(key);
        if (pos == exifData.end()) {
            return false;
        }
        Exiv2::Value::UniquePtr v = pos->getValue();
        for (int i = 0; i < v->count(); i++) {
            str_p.push_back(v->toString(i));
        }
        return v->ok();
    }
    //============================================================================


    //____________________________________________________________________________
    // unsigned char values
    //
    bool IIIFExif::getValByKey(const std::string key_p, unsigned char &uc) {
        Exiv2::ExifKey key = Exiv2::ExifKey(key_p);
        Exiv2::ExifData::iterator pos = exifData.findKey(key);
        if (pos == exifData.end()) {
            return false;
        }
        Exiv2::Value::UniquePtr v = pos->getValue();
        uc = (unsigned char) v->toUint32();
        return v->ok();
    }
    //============================================================================


    bool IIIFExif::getValByKey(uint16_t tag, const std::string &groupName, unsigned char &uc) {
        Exiv2::ExifKey key = Exiv2::ExifKey(tag, groupName);
        Exiv2::ExifData::iterator pos = exifData.findKey(key);
        if (pos == exifData.end()) {
            return false;
        }
        Exiv2::Value::UniquePtr v = pos->getValue();
        uc = (unsigned char) v->toUint32();
        return v->ok();
    }
    //============================================================================


    bool IIIFExif::getValByKey(uint16_t tag, const std::string &groupName, std::vector<unsigned char> &vuc) {
        Exiv2::ExifKey key = Exiv2::ExifKey(tag, groupName);
        Exiv2::ExifData::iterator pos = exifData.findKey(key);
        if (pos == exifData.end()) {
            return false;
        }
        Exiv2::Value::UniquePtr v = pos->getValue();
        for (int i = 0; i < v->count(); i++) {
            vuc.push_back((unsigned char) v->toUint32(i));
        }
        return v->ok();
    }
    //============================================================================

    //____________________________________________________________________________
    // float values
    //
    bool IIIFExif::getValByKey(const std::string key_p, float &f) {
        Exiv2::ExifKey key = Exiv2::ExifKey(key_p);
        Exiv2::ExifData::iterator pos = exifData.findKey(key);
        if (pos == exifData.end()) {
            return false;
        }
        Exiv2::Value::UniquePtr v = pos->getValue();
        f = v->toFloat();
        return v->ok();
    }
    //============================================================================


    bool IIIFExif::getValByKey(uint16_t tag, const std::string &groupName, float &f) {
        Exiv2::ExifKey key = Exiv2::ExifKey(tag, groupName);
        Exiv2::ExifData::iterator pos = exifData.findKey(key);
        if (pos == exifData.end()) {
            return false;
        }
        Exiv2::Value::UniquePtr v = pos->getValue();
        f = v->toFloat();
        return v->ok();
    }
    //============================================================================


    bool IIIFExif::getValByKey(uint16_t tag, const std::string &groupName, std::vector<float> &f) {
        Exiv2::ExifKey key = Exiv2::ExifKey(tag, groupName);
        Exiv2::ExifData::iterator pos = exifData.findKey(key);
        if (pos == exifData.end()) {
            return false;
        }
        Exiv2::Value::UniquePtr v = pos->getValue();
        for (int i = 0; i < v->count(); i++) {
            f.push_back(v->toFloat(i));
        }
        return v->ok();
    }
    //============================================================================


    //____________________________________________________________________________
    // Rational values
    //
    bool IIIFExif::getValByKey(const std::string key_p, Exiv2::Rational &r) {
        Exiv2::ExifKey key = Exiv2::ExifKey(key_p);
        Exiv2::ExifData::iterator pos = exifData.findKey(key);
        if (pos == exifData.end()) {
            return false;
        }
        Exiv2::Value::UniquePtr v = pos->getValue();
        r =  v->toRational();
        return v->ok();
    }
    //============================================================================


    bool IIIFExif::getValByKey(uint16_t tag, const std::string &groupName, Exiv2::Rational &r) {
        Exiv2::ExifKey key = Exiv2::ExifKey(tag, groupName);
        Exiv2::ExifData::iterator pos = exifData.findKey(key);
        if (pos == exifData.end()) {
            return false;
        }
        Exiv2::Value::UniquePtr v = pos->getValue();
        r =  v->toRational();
        return v->ok();
    }
    //============================================================================


    bool IIIFExif::getValByKey(uint16_t tag, const std::string &groupName, std::vector<Exiv2::Rational> &r) {
        Exiv2::ExifKey key = Exiv2::ExifKey(tag, groupName);
        Exiv2::ExifData::iterator pos = exifData.findKey(key);
        if (pos == exifData.end()) {
            return false;
        }
        Exiv2::Value::UniquePtr v = pos->getValue();
        for (int i = 0; i < v->count(); i++) {
            r.push_back(v->toRational(i));
        }
        return v->ok();
    }
    //============================================================================


    //____________________________________________________________________________
    // short values
    //
    bool IIIFExif::getValByKey(const std::string key_p, short &s) {
        Exiv2::ExifKey key = Exiv2::ExifKey(key_p);
        Exiv2::ExifData::iterator pos = exifData.findKey(key);
        if (pos == exifData.end()) {
            return false;
        }
        Exiv2::Value::UniquePtr v = pos->getValue();
        s = (short) v->toUint32();
        return v->ok();
    }
    //============================================================================


    bool IIIFExif::getValByKey(uint16_t tag, const std::string &groupName, short &s) {
        Exiv2::ExifKey key = Exiv2::ExifKey(tag, groupName);
        Exiv2::ExifData::iterator pos = exifData.findKey(key);
        if (pos == exifData.end()) {
            return false;
        }
        Exiv2::Value::UniquePtr v = pos->getValue();
        s = (short) v->toUint32();
        return v->ok();
    }
    //============================================================================


    bool IIIFExif::getValByKey(uint16_t tag, const std::string &groupName, std::vector<short> &s) {
        Exiv2::ExifKey key = Exiv2::ExifKey(tag, groupName);
        Exiv2::ExifData::iterator pos = exifData.findKey(key);
        if (pos == exifData.end()) {
            return false;
        }
        Exiv2::Value::UniquePtr v = pos->getValue();
        for (int i = 0; i < v->count(); i++) {
            s.push_back((short) v->toUint32(i));
        }
        return v->ok();
    }
    //============================================================================


    //____________________________________________________________________________
    // unsigned short values
    //
    bool IIIFExif::getValByKey(const std::string key_p, unsigned short &s) {
        Exiv2::ExifKey key = Exiv2::ExifKey(key_p);
        Exiv2::ExifData::iterator pos = exifData.findKey(key);
        if (pos == exifData.end()) {
            return false;
        }
        Exiv2::Value::UniquePtr v = pos->getValue();
        s = (unsigned short) v->toUint32();
        return v->ok();
    }
    //============================================================================


    bool IIIFExif::getValByKey(uint16_t tag, const std::string &groupName, unsigned short &s) {
        Exiv2::ExifKey key = Exiv2::ExifKey(tag, groupName);
        Exiv2::ExifData::iterator pos = exifData.findKey(key);
        if (pos == exifData.end()) {
            return false;
        }
        Exiv2::Value::UniquePtr v = pos->getValue();
        s = (unsigned short) v->toUint32();
        return v->ok();
    }
    //============================================================================


    bool IIIFExif::getValByKey(uint16_t tag, const std::string &groupName, std::vector<unsigned short> &s) {
        Exiv2::ExifKey key = Exiv2::ExifKey(tag, groupName);
        Exiv2::ExifData::iterator pos = exifData.findKey(key);
        if (pos == exifData.end()) {
            return false;
        }
        Exiv2::Value::UniquePtr v = pos->getValue();
        for (int i = 0; i < v->count(); i++) {
            s.push_back((unsigned short) v->toUint32());
        }
        return v->ok();
    }
    //============================================================================


    //____________________________________________________________________________
    // int values
    //
    bool IIIFExif::getValByKey(const std::string key_p, int &s) {
        Exiv2::ExifKey key = Exiv2::ExifKey(key_p);
        Exiv2::ExifData::iterator pos = exifData.findKey(key);
        if (pos == exifData.end()) {
            return false;
        }
        Exiv2::Value::UniquePtr v = pos->getValue();
        s = (int) v->toUint32();
        return v->ok();
    }
    //============================================================================


    bool IIIFExif::getValByKey(uint16_t tag, const std::string &groupName, int &s) {
        Exiv2::ExifKey key = Exiv2::ExifKey(tag, groupName);
        Exiv2::ExifData::iterator pos = exifData.findKey(key);
        if (pos == exifData.end()) {
            return false;
        }
        Exiv2::Value::UniquePtr v = pos->getValue();
        s = (int) v->toUint32();
        return v->ok();
    }
    //============================================================================


    bool IIIFExif::getValByKey(uint16_t tag, const std::string &groupName, std::vector<int> &s) {
        Exiv2::ExifKey key = Exiv2::ExifKey(tag, groupName);
        Exiv2::ExifData::iterator pos = exifData.findKey(key);
        if (pos == exifData.end()) {
            return false;
        }
        Exiv2::Value::UniquePtr v = pos->getValue();
        for (int i = 0; i < v->count(); i++) {
            s.push_back((int) v->toUint32(i));
        }
        return v->ok();
    }
    //============================================================================


    //____________________________________________________________________________
    // unsigned int values
    //
    bool IIIFExif::getValByKey(const std::string key_p, unsigned int &s) {
        Exiv2::ExifKey key = Exiv2::ExifKey(key_p);
        Exiv2::ExifData::iterator pos = exifData.findKey(key);
        if (pos == exifData.end()) {
            return false;
        }
        Exiv2::Value::UniquePtr v = pos->getValue();
        s = (unsigned int) v->toUint32();
        return v->ok();
    }
    //============================================================================


    bool IIIFExif::getValByKey(uint16_t tag, const std::string &groupName, unsigned int &s) {
        Exiv2::ExifKey key = Exiv2::ExifKey(tag, groupName);
        Exiv2::ExifData::iterator pos = exifData.findKey(key);
        if (pos == exifData.end()) {
            return false;
        }
        Exiv2::Value::UniquePtr v = pos->getValue();
        s = (unsigned int) v->toUint32();
        return v->ok();
    }
    //============================================================================


    bool IIIFExif::getValByKey(uint16_t tag, const std::string &groupName, std::vector<unsigned int> &s) {
        Exiv2::ExifKey key = Exiv2::ExifKey(tag, groupName);
        Exiv2::ExifData::iterator pos = exifData.findKey(key);
        if (pos == exifData.end()) {
            return false;
        }
        Exiv2::Value::UniquePtr v = pos->getValue();
        for (int i = 0; i < v->count(); i++) {
            s.push_back((unsigned int) v->toUint32());
        }
        return v->ok();
    }
    //============================================================================


    std::ostream &operator<< (std::ostream &outstr, IIIFExif &rhs) {
        Exiv2::ExifData::const_iterator end = rhs.exifData.end();
        for (Exiv2::ExifData::const_iterator i = rhs.exifData.begin(); i != end; ++i) {
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
