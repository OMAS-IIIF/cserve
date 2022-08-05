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
#include <mutex>

#include "../IIIFError.h"
#include "IIIFXmp.h"

static const char __file__[] = __FILE__;

/*!
 * ToDo: remove provisional code as soon as Exiv2::Xmp is thread safe (expected v.26)
 * ATTENTION!!!!!!!!!
 * Since the Xmp-Part of Exiv2 Version 0.25 is not thread safe, we omit for the moment
 * the use of Exiv2::Xmp for processing XMP. We just transfer the XMP string as is. This
 * is bad, since we are not able to modifiy it. But we'll try again with Exiv2 v0.26!
 */

namespace cserve {

    XmpMutex xmp_mutex;


    void xmplock_func(void *pLockData, bool lockUnlock) {
        XmpMutex *m = (XmpMutex *) pLockData;
        if (lockUnlock) {
            std::cerr << "XMP-LOCK!" << std::endl;
            m->lock.lock();
        }
        else {
            std::cerr << "XMP-UNLOCK!" << std::endl;
            m->lock.unlock();
        }
    }
    //=========================================================================

    IIIFXmp::IIIFXmp(const IIIFXmp &rhs) {
        __xmpstr = rhs.__xmpstr;
    }

    IIIFXmp::IIIFXmp(IIIFXmp &&rhs) {
        __xmpstr = std::move(rhs.__xmpstr);
        rhs.__xmpstr.clear();
    }


    IIIFXmp::IIIFXmp(const std::string &xmp) {
        __xmpstr = xmp; // provisional code until Exiv2::Xmp is threadsafe
        return; // provisional code until Exiv2::Xmp is threadsafe
        // TODO: Testing required if now Exiv2::Xmp is thread save
        /*
        try {
            if (Exiv2::XmpParser::decode(xmpData, xmp) != 0) {
                Exiv2::XmpParser::terminate();
                throw SipiError(file_, __LINE__, "Could not parse XMP!");
            }
        }
        catch(Exiv2::BasicError<char> &err) {
            throw SipiError(file_, __LINE__, err.what());
        }
         */
    }
    //============================================================================

    IIIFXmp::IIIFXmp(const char *xmp) {
        __xmpstr = xmp; // provisional code until Exiv2::Xmp is threadsafe
        return; // provisional code until Exiv2::Xmp is threadsafe
        // TODO: Testing required if now Exiv2::Xmp is thread save
        /*
        try {
            if (Exiv2::XmpParser::decode(xmpData, xmp) != 0) {
                Exiv2::XmpParser::terminate();
                throw SipiError(file_, __LINE__, "Could not parse XMP!");
            }
        }
        catch(Exiv2::BasicError<char> &err) {
            throw SipiError(file_, __LINE__, err.what());
        }
         */
    }
    //============================================================================

    IIIFXmp::IIIFXmp(const char *xmp, int len) {
        std::string buf(xmp, len);
        __xmpstr = buf; // provisional code until Exiv2::Xmp is threadsafe
        return; // provisional code until Exiv2::Xmp is threadsafe
        // TODO: Testing required if now Exiv2::Xmp is thread save
        /*
        try {
            if (Exiv2::XmpParser::decode(xmpData, buf) != 0) {
                Exiv2::XmpParser::terminate();
                throw SipiError(file_, __LINE__, "Could not parse XMP!");
            }
        }
        catch(Exiv2::BasicError<char> &err) {
            throw SipiError(file_, __LINE__, err.what());
        }
         */
    }
    //============================================================================


    IIIFXmp::~IIIFXmp() {
        //Exiv2::XmpParser::terminate();
    }
    //============================================================================

    IIIFXmp &IIIFXmp::operator=(const IIIFXmp &rhs) {
        if (this != &rhs) {
            __xmpstr = rhs.__xmpstr;
        }
        return *this;
    }

    IIIFXmp &IIIFXmp::operator=(IIIFXmp &&rhs) {
        if (this != &rhs) {
            __xmpstr = std::move(rhs.__xmpstr);
            rhs.__xmpstr.clear();
        }
        return *this;
    }

    std::unique_ptr<char[]> IIIFXmp::xmpBytes(unsigned int &len) {
        auto buf_ = std::make_unique<char[]>(__xmpstr.length() + 1);
        memcpy (buf_.get(), __xmpstr.c_str(), __xmpstr.length());
        buf_[__xmpstr.length()] = '\0';
        len = __xmpstr.length();
        return std::move(buf_); // provisional code until Exiv2::Xmp is threadsafe
        // TODO: Testing required if now Exiv2::Xmp is thread save

        /*
        std::string xmpPacket;
        try {
            if (0 != Exiv2::XmpParser::encode(xmpPacket, xmpData, Exiv2::XmpParser::useCompactFormat)) {
                throw SipiError(file_, __LINE__, "Failed to serialize XMP data!");
            }
        }
        catch(Exiv2::BasicError<char> &err) {
            throw SipiError(file_, __LINE__, err.what());
        }
        Exiv2::XmpParser::terminate();

        len = xmpPacket.size();
        char *buf = new char[len + 1];
        memcpy (buf, xmpPacket.c_str(), len);
        buf[len] = '\0';
        return buf;
         */
    }
    //============================================================================

    std::string IIIFXmp::xmpBytes(void) {
        unsigned int len = 0;
        auto buf = xmpBytes(len);
        std::string data(buf.get(), buf.get() + len);
        return data;
    }
    //============================================================================

    std::ostream &operator<< (std::ostream &outstr, const IIIFXmp &rhs) {
        /*
        for (Exiv2::XmpData::const_iterator md = rhs.xmpData.begin();
        md != rhs.xmpData.end(); ++md) {
            outstr << std::setfill(' ') << std::left
                << std::setw(44)
                << md->key() << " "
                << std::setw(9) << std::setfill(' ') << std::left
                << md->typeName() << " "
                << std::dec << std::setw(3)
                << std::setfill(' ') << std::right
                << md->count() << "  "
                << std::dec << md->value()
                << std::endl;
        }
         */
        return outstr;
    }
    //============================================================================

}
