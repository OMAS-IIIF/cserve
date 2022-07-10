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
#include <iomanip>
#include <sstream>

#include <fcntl.h>
#include <unistd.h>

#include "Error.h"
#include "Hash.h"
#include "makeunique.h"

using namespace std;

static const char file_[] = __FILE__;

namespace cserve {

    [[maybe_unused]] Hash::Hash(HashType type) {
        context = EVP_MD_CTX_create();
        if (context == nullptr) {
            throw Error(file_, __LINE__, "EVP_MD_CTX_create failed!");
        }
        int status;
        switch (type) {
            case none:
            case md5: {
                status = EVP_DigestInit_ex(context, EVP_md5(), nullptr);
                break;
            }
            case sha1: {
                status = EVP_DigestInit_ex(context, EVP_sha1(), nullptr);
                break;
            }
            case sha256: {
                status = EVP_DigestInit_ex(context, EVP_sha256(), nullptr);
                break;
            }
            case sha384: {
                status = EVP_DigestInit_ex(context, EVP_sha384(), nullptr);
                break;
            }
            case sha512: {
                status = EVP_DigestInit_ex(context, EVP_sha512(), nullptr);
                break;
            }
        }
        if (status != 1) {
            EVP_MD_CTX_destroy(context);
            throw Error(file_, __LINE__, "EVP_DigestInit_ex failed!");
        }
    }
    //==========================================================================

    Hash::~Hash() {
        EVP_MD_CTX_destroy(context);
    }
    //==========================================================================

    [[maybe_unused]] bool Hash::add_data(const void *data, size_t len) {
        return EVP_DigestUpdate(context, data, len);
    }
    //==========================================================================

    [[maybe_unused]] bool Hash::hash_of_file(const string &path, size_t buflen) {
        auto buf = make_unique<char[]>(buflen);

        int fptr = ::open(path.c_str(), O_RDONLY);
        if (fptr == -1) {
            return false;
        }
        size_t n;
        while ((n = ::read(fptr, buf.get(), buflen)) > 0) {
            if (n == -1) {
                ::close(fptr);
                return false;
            }
            if (!EVP_DigestUpdate(context, buf.get(), n)) {
                ::close(fptr);
                return false;
            }
        }
        ::close(fptr);
        return true;
    }
    //==========================================================================

    istream &operator>>(istream &input, Hash &h) {
        char buffer[4096];
        int i = 0;
        while (input.good() && (i < 4096)) {
            int c = input.get();
            if (c == EOF) break;
            buffer[i++] = static_cast<char>(c);
        }
        EVP_DigestUpdate(h.context, buffer, i);
        return input;
    }
    //==========================================================================

    [[maybe_unused]] string Hash::hash() {
        unsigned char hash[EVP_MAX_MD_SIZE];
        unsigned int lengthOfHash = 0;
        string hashstr;
        if (EVP_DigestFinal_ex(context, hash, &lengthOfHash)) {
            std::stringstream ss;
            for (unsigned int i = 0; i < lengthOfHash; ++i) {
                ss << std::hex << std::setw(2) << std::setfill('0') << (int) hash[i];
            }
            hashstr = ss.str();
        }
        return hashstr;
    }
    //==========================================================================
}
