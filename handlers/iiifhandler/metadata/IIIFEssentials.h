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
#ifndef defined_essentials_h
#define defined_essentials_h

#include <cstdlib>
#include <sstream>
#include <utility>
#include <string>


#include "Hash.h"

namespace cserve {

    /*!
    * Small class to create a small essential metadata packet that can be embedded
    * within an image header. It contains
    * - original file name
    * - original mime type
    * - checksum method
    * - checksum of uncompressed pixel values
    * The class contains methods to set the fields, to serialize and deserialize
    * the data.
    */
    class IIIFEssentials {
    private:
        bool _is_set{};
        std::string _origname; //!< original filename
        std::string _mimetype; //!< original mime type
        HashType _hash_type; //!< type of checksum
        std::string _data_chksum; //!< the checksum of pixel data
        bool _use_icc{}; //!< use this ICC profile when converting from JPEG200 to other format
        std::string _icc_profile; //!< ICC profile if JPEG2000 can not deal with it directly

    public:
        /*!
        * Constructor for empty packet
        */
        inline IIIFEssentials() {
            _hash_type = HashType::none;
            _is_set = false;
            _use_icc = false;
        }

        IIIFEssentials(const IIIFEssentials &other);

        IIIFEssentials(IIIFEssentials &&other) noexcept;

        /*!
        * Constructor where all fields are passed
        *
        * \param[in] origname_p Original filename
        * \param[in] mimetype_p Original mimetype as string
        * \param[in] hash_type_p Checksumtype as defined in Hash.h (shttps::HashType)
        * \param[in] data_chksum_p The actual checksum of the internal image data
        * \param[in] icc_profile_p ICC profile data
        */
        IIIFEssentials(
                const std::string &origname_p,
                const std::string &mimetype_p,
                HashType hash_type_p,
                const std::string &data_chksum_p,
                const std::vector<unsigned char> &icc_profile_p);

        IIIFEssentials(
                const std::string &origname_p,
                const std::string &mimetype_p,
                HashType hash_type_p,
                const std::string &data_chksum_p);

        IIIFEssentials &operator=(const IIIFEssentials &other);

        IIIFEssentials &operator=(IIIFEssentials &&other) noexcept ;

        IIIFEssentials &operator=(const std::string &str);

        /*!
        * Constructor taking a serialized packet (as string)
        *
        * \param[in] datastr Serialzed metadata packet
        */
        explicit IIIFEssentials(const std::string &datastr);

        inline void clear() {
            _is_set = false;
            _use_icc = false;
            _data_chksum.clear();
            _icc_profile.clear();
            _mimetype.clear();
            _origname.clear();
            _hash_type = none;
        }

        /*!
        * Getter for original name
        */
        inline std::string origname() { return _origname; }

        /*!
        * Setter for original name
        */
        inline void origname(const std::string &origname_p) {
            _origname = origname_p;
            _is_set = true;
        }

        /*!
        * Getter for mimetype
        */
        inline std::string mimetype() { return _mimetype; }

        /*!
        * Setter for original name
        */
        inline void mimetype(const std::string &mimetype_p) { _mimetype = mimetype_p; }

        /*!
        * Getter for checksum type as shttps::HashType
        */
        inline HashType hash_type() { return _hash_type; }

        /*!
        * Getter for checksum type as string
        */
        [[nodiscard]] std::string hash_type_string() const;

        /*!
        * Setter for checksum type
        *
        * \param[in] hash_type_p checksum type
        */
        inline void hash_type(HashType hash_type_p) { _hash_type = hash_type_p; }

        /*!
        * Setter for checksum type
        *
        * \param[in] hash_type_p checksum type
        */
        void hash_type(const std::string &hash_type_p);

        /*!
        * Getter for checksum
         *
         * @return Chekcsum as std::string
        */
        inline std::string data_chksum() { return _data_chksum; }

        /*!
        * Setter for checksum
        */
        inline void data_chksum(const std::string &data_chksum_p) { _data_chksum = data_chksum_p; }

        /*!
         * Do I have to use this ICC profile? (only when converting from JPEG2000
         * @return Bool if this ICC profile should be used....
         */
        [[nodiscard]] inline bool use_icc() const { return _use_icc; }

        /*!
         * Setter for boolean flag for the usage of the essential's ICC profile
         * @param use_icc_p
         */
        inline void use_icc(bool use_icc_p) { _use_icc = use_icc_p; }

        std::vector<unsigned char> icc_profile();
         /*!
          * Setter for ICC profile
          *
          * \param[in] icc_profile_p ICC profile binary data as std::vector
          */
          void icc_profile(const std::vector<unsigned char> &icc_profile_p);

        /*!
        * Parse a string containing a serialized metadata packet
        */
        void parse(const std::string &str);

        /*!
        * Check if essential metadata has been set.
        */
        [[nodiscard]] inline bool is_set() const { return _is_set; }

        /*!
        * String conversion operator
        */
        inline explicit operator std::string() const {
            std::stringstream ss;
            ss << *this;
            return ss.str();
        }

        /*!
        * Stream output operator
        */
        inline friend std::ostream &operator<<(std::ostream &ostr, const IIIFEssentials &rhs) {
            ostr << rhs._origname << "|" << rhs._mimetype << "|" << rhs.hash_type_string() << "|" << rhs._data_chksum;
            if (rhs._use_icc) ostr << "|USE_ICC|"; else ostr << "|IGNORE_ICC|";
            if (!rhs._icc_profile.empty()) ostr << rhs._icc_profile; else ostr << "NULL";
            return ostr;
        }
    };
}


#endif
