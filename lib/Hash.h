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
#ifndef cserve_hash_h
#define cserve_hash_h

#include <iostream>

#include <openssl/evp.h>


namespace cserve {

    typedef enum {
        none = 0, md5 = 1, sha1 = 2, sha256 = 3, sha384 = 4, sha512 = 5
    } HashType;

    /*!
     * \brief Hash class which implements a variety of checksum schemes
     * \author Lukas Rosenthaler
     * \version 1.0
     * \date 2016-11-11
     */
    class Hash {
    private:
        EVP_MD_CTX *context;

    public:
        /*!
        * Constructor of a Hash instance
        *
        * \param[in] type Hash/checksum method to use (see HashType)
        */
        [[maybe_unused]] explicit Hash(HashType type);

        /*!
        * Destructor which cleans up everything
        */
        ~Hash();

        /*!
        * Adds data to the hash
        *
        * \param[in] data Pointer to data
        * \param[in] len Length of data in bytes
        *
        * \returns true in case of success, false if the data couln't be processed
        */
        [[maybe_unused]] bool add_data(const void *data, size_t len);

        /*!
        * Calculate the checksum of a fileType_string. THe method uses directly
        * the unix system calls open, read and write and does the buffering
        * internally.
        *
        * \param[in] path Path to the file
        * \param[in] buflen Internal buffer for reading the file
        *
        * \returns true in case of success, false if the data couln't be processed
        */
        [[maybe_unused]] bool hash_of_file(const std::string &path, size_t buflen = 16 * 1024);

        /*!
        * Adds data to the has from a input stream. It reads the stream until
        * an eof is encountered!
        *
        * \code{.cpp}
        * stringstream strstr;
        * ss << "Waseliwas " << a_number << " ist das ?";
        * cserve::Hash h(shttp::HashType:md5);
        * strstr >> h;
        * std::string checksum = h.hash();
        * \endcode
        */
        friend std::istream &operator>>(std::istream &input, Hash &h);

        /*!
        * Calculate and return the has value as string
        *
        * \returns Returns the has value as string
        */
        [[maybe_unused]] std::string hash();
    };

}

#endif
