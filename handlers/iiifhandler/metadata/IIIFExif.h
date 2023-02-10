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
#ifndef defined_exif_h
#define defined_exif_h

/*
namespace std {
    template<typename T>
    using auto_ptr = unique_ptr<T>;
}
*/

#include <memory>
#include <string>
#include <vector>
#include <exiv2/exiv2.hpp>

#include "../IIIFError.h"

namespace cserve {

    /**
     * @class SipiXmp
     * @author Lukas Rosenthaler
     * @version 0.1
     *
     * This class handles Exif metadata. It uses the Exiv2 library
     *
     * There is a problem that the libtiff libraray accesses the EXIF data tag by tag and is not able
     * to pass or get the the EXIF data as a blob. All other libraries pass EXIF data as a blob that
     * can be handled by exiv2. Therefore there are methods to get/add EXIF-data tagwise.
     * A list of all valid EXIF tags can be found at http://www.exiv2.org/tags.html .
     */
    class IIIFExif {
    private:
        std::shared_ptr<unsigned char[]> binaryExif;
        uint32_t binary_size;
        Exiv2::ExifData exifData;   //!< Private member variable holding the exiv2 EXIF data
        Exiv2::ByteOrder byteorder; //!< Private member holding the byteorder of the EXIF data

        static inline bool assign_val(std::unique_ptr<Exiv2::Value> v, std::string &val) {
            val = v->toString();
            return v->ok();
        }

        static inline bool assign_val(std::unique_ptr<Exiv2::Value> v, std::vector<std::string> &val) {
            for (int i = 0; i < v->count(); i++) {
                val.push_back(v->toString(i));
            }
            return v->ok();
        }

        static inline bool assign_val(std::unique_ptr<Exiv2::Value> v, char &val) {
            val = static_cast<char>(v->toInt64());
            return v->ok();
        }

        static inline bool assign_val(std::unique_ptr<Exiv2::Value> v, std::vector<char> &val) {
            for (int i = 0; i < v->count(); i++) {
                val.push_back(static_cast<char>(v->toInt64(i)));
            }
            return v->ok();
        }

        static inline bool assign_val(std::unique_ptr<Exiv2::Value> v, unsigned char &val) {
            val = static_cast<unsigned char>(v->toUint32());
            return v->ok();
        }

        static inline bool assign_val(std::unique_ptr<Exiv2::Value> v, std::vector<unsigned char> &val) {
            for (int i = 0; i < v->count(); i++) {
                val.push_back(static_cast<unsigned char>(v->toUint32(i)));
            }
            return v->ok();
        }

        static inline bool assign_val(std::unique_ptr<Exiv2::Value> v, short &val) {
            val = static_cast<short>(v->toInt64());
            return v->ok();
        }

        static inline bool assign_val(std::unique_ptr<Exiv2::Value> v, std::vector<short> &val) {
            for (int i = 0; i < v->count(); i++) {
                val.push_back(static_cast<short>(v->toInt64(i)));
            }
            return v->ok();
        }

        static inline bool assign_val(std::unique_ptr<Exiv2::Value> v, unsigned short &val) {
            val = static_cast<unsigned short>(v->toUint32());
            return v->ok();
        }

        static inline bool assign_val(std::unique_ptr<Exiv2::Value> v, std::vector<unsigned short> &val) {
            for (int i = 0; i < v->count(); i++) {
                val.push_back(static_cast<unsigned short>(v->toUint32(i)));
            }
            return v->ok();
        }

        static inline bool assign_val(std::unique_ptr<Exiv2::Value> v, int &val) {
            val = static_cast<int>(v->toInt64());
            return v->ok();
        }

        static inline bool assign_val(std::unique_ptr<Exiv2::Value> v, std::vector<int> &val) {
            for (int i = 0; i < v->count(); i++) {
                val.push_back(static_cast<int>(v->toInt64(i)));
            }
            return v->ok();
        }

        static inline bool assign_val(std::unique_ptr<Exiv2::Value> v, unsigned int &val) {
            val = static_cast<unsigned int>(v->toUint32());
            return v->ok();
        }

        static inline bool assign_val(std::unique_ptr<Exiv2::Value> v, std::vector<unsigned int> &val) {
            for (int i = 0; i < v->count(); i++) {
                val.push_back(static_cast<unsigned int>(v->toUint32(i)));
            }
            return v->ok();
        }

        static inline bool assign_val(std::unique_ptr<Exiv2::Value> v, float &val) {
            val = static_cast<float>(v->toFloat());
            return v->ok();
        }

        static inline bool assign_val(std::unique_ptr<Exiv2::Value> v, std::vector<float> &val) {
            for (int i = 0; i < v->count(); i++) {
                val.push_back(static_cast<float>(v->toFloat(i)));
            }
            return v->ok();
        }

        static inline bool assign_val(std::unique_ptr<Exiv2::Value> v, double &val) {
            val = static_cast<double>(v->toFloat());
            return v->ok();
        }

        static inline bool assign_val(std::unique_ptr<Exiv2::Value> v, std::vector<double> &val) {
            for (int i = 0; i < v->count(); i++) {
                val.push_back(static_cast<double>(v->toFloat(i)));
            }
            return v->ok();
        }

        static inline bool assign_val(std::unique_ptr<Exiv2::Value> v, Exiv2::Rational &val) {
            val = v->toRational();
            return v->ok();
        }

        static inline bool assign_val(std::unique_ptr<Exiv2::Value> v, std::vector<Exiv2::Rational> &val) {
            for (int i = 0; i < v->count(); i++) {
                val.push_back(v->toRational(i));
            }
            return v->ok();
        }

    public:
        /*!
         * Constructor (default)
         */
        IIIFExif();

        /*!
         * Copy Constructor with deep copy
         *
         * @param other
         */
        IIIFExif(const IIIFExif &other);

        /*!
         * Move constructor
         *
         * @param other
         */
        IIIFExif(IIIFExif &&other) noexcept;


        /*!
         * Constructor using an EXIF blob
         *
         * \param[in] exif Buffer containing the EXIF data
         * \Param[in] len Length of the EXIF buffer
         */
        IIIFExif(const unsigned char *exif, unsigned int len);


        ~IIIFExif();

        /*!
         * Copy assignment
         *
         * @param other
         * @return
         */
        IIIFExif &operator=(const IIIFExif &other);

        /*!
         * Move assignment
         *
         * @param other
         * @return
         */
        IIIFExif &operator=(IIIFExif &&other) noexcept;

        /*!
         * Returns the bytes of the EXIF data
         *
         * \param[out] len Length of buffer returned
         * \returns Buffer with EXIF data
         */
        std::shared_ptr<unsigned char[]> exifBytes(unsigned int &len);

        /*!
         * Returns the bytes of the EXIF data as vector
         *
         * @return Vector with EXIF data
         */
        std::vector<unsigned char> exifBytes();

        /*!
         * Helper function to convert a signed float to a signed rational as used by EXIF
         *
         * \param[in] f Input signed float
         * \returns Exiv2::Rational
         */
        static Exiv2::Rational toRational(double f);

        /*!
         * Helper function to convert a unsigned float to a unsigned rational as used by EXIF
         *
         * \param[in] f Input unsigned float
         * \returns Exiv2::URational
         */
        static Exiv2::URational toURational(double f);


        template<class T>
        void addKeyVal(const std::string &key_p, const T &val) {
            exifData[key_p] = val;
        }

        template<class T>
        void addKeyVal(uint16_t tag, const std::string &groupName, const T &val) {
            Exiv2::ExifKey key = Exiv2::ExifKey(tag, groupName);
            std::unique_ptr<Exiv2::Value> v;
            if (typeid(T) == typeid(std::string)) {
                v = Exiv2::Value::create(Exiv2::asciiString);
            }
            else if (typeid(T) == typeid(int8_t)) {
                v = Exiv2::Value::create(Exiv2::signedByte);
            }
            else if (typeid(T) == typeid(uint8_t)) {
                v = Exiv2::Value::create(Exiv2::unsignedByte);
            }
            else if (typeid(T) == typeid(int16_t)) {
                v = Exiv2::Value::create(Exiv2::signedShort);
            }
            else if (typeid(T) == typeid(uint16_t)) {
                v = Exiv2::Value::create(Exiv2::unsignedShort);
            }
            else if (typeid(T) == typeid(int32_t)) {
                v = Exiv2::Value::create(Exiv2::signedLong);
            }
            else if (typeid(T) == typeid(uint32_t)) {
                v = Exiv2::Value::create(Exiv2::unsignedLong);
            }
            else if (typeid(T) == typeid(float)) {
                v = Exiv2::Value::create(Exiv2::tiffFloat);
            }
            else if (typeid(T) == typeid(double)) {
                v = Exiv2::Value::create(Exiv2::tiffDouble);
            }
            else if (typeid(T) == typeid(Exiv2::Rational)) {
                v = Exiv2::Value::create(Exiv2::signedRational);
            }
            else if (typeid(T) == typeid(Exiv2::URational)) {
                v = Exiv2::Value::create(Exiv2::unsignedRational);
            }
            else {
                throw IIIFError(__FILE__, __LINE__, "Unsupported type of addKeyVal(2)");
            }

            v->read((unsigned char *) &val, sizeof(T), Exiv2::littleEndian);
            exifData.add(key, v.get());
        }

        template<class T>
        void addKeyVal(uint16_t tag, const std::string &groupName, const T *valptr, size_t len) {
            Exiv2::ExifKey key = Exiv2::ExifKey(tag, groupName);
            std::unique_ptr<Exiv2::Value> v;
            if (typeid(T) == typeid(int8_t)) {
                v = Exiv2::Value::create(Exiv2::signedByte);
            }
            else if (typeid(T) == typeid(uint8_t)) {
                v = Exiv2::Value::create(Exiv2::unsignedByte);
            }
            else if (typeid(T) == typeid(int16_t)) {
                v = Exiv2::Value::create(Exiv2::signedShort);
            }
            else if (typeid(T) == typeid(uint16_t)) {
                v = Exiv2::Value::create(Exiv2::unsignedShort);
            }
            else if (typeid(T) == typeid(int32_t)) {
                v = Exiv2::Value::create(Exiv2::signedLong);
            }
            else if (typeid(T) == typeid(int32_t)) {
                v = Exiv2::Value::create(Exiv2::unsignedLong);
            }
            else if (typeid(T) == typeid(float)) {
                v = Exiv2::Value::create(Exiv2::tiffFloat);
            }
            else if (typeid(T) == typeid(double)) {
                v = Exiv2::Value::create(Exiv2::tiffDouble);
            }
            else if (typeid(T) == typeid(Exiv2::Rational)) {
                v = Exiv2::Value::create(Exiv2::signedRational);
            }
            else if (typeid(T) == typeid(Exiv2::URational)) {
                v = Exiv2::Value::create(Exiv2::unsignedRational);
            }
            else {
                throw IIIFError(__FILE__, __LINE__, "Unsupported type of addKeyVal(2)");
            }
            v->read((unsigned char *) valptr, static_cast<long>(len*sizeof(unsigned short)), Exiv2::littleEndian);
            exifData.add(key, v.get());
        }

        template<class T>
        bool getValByKey(const std::string &key_p, T &val)  {
            try {
                Exiv2::ExifKey key = Exiv2::ExifKey(key_p);
                auto pos = exifData.findKey(key);
                if (pos == exifData.end()) {
                    return false;
                }
                auto v = pos->getValue();
                return assign_val(std::move(v), val);
            } catch (const Exiv2::Error &err) {
                return false;
            }
        }

        template<class T>
        bool getValByKey(uint16_t tag, const std::string &groupName, T &val) {
            try {
                Exiv2::ExifKey key = Exiv2::ExifKey(tag, groupName);
                auto pos = exifData.findKey(key);
                if (pos == exifData.end()) {
                    return false;
                }
                auto v = pos->getValue();
                return assign_val(std::move(v), val);

            } catch (const Exiv2::Error &err) {
                return false;
            }
        }


        friend std::ostream &operator<<(std::ostream &lhs, IIIFExif &rhs);

    };

}

#endif
