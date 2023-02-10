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


#include <cmath>
#include <vector>
#include <array>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/evp.h>

#include "Error.h"
#include "IIIFEssentials.h"

static const char file_[] = __FILE__;

static int calcDecodeLength(const std::string &b64input) {
    int padding = 0;

    // Check for trailing '=''s as padding
    if(b64input[b64input.length() - 1] == '=' && b64input[b64input.length() - 2] == '=')
        padding = 2;
    else if (b64input[b64input.length() - 1] == '=')
        padding = 1;

    return (int) (b64input.length()*0.75) - padding;
}

std::string base64Encode(const std::vector<unsigned char> &message) {
    BIO *bio;
    BIO *b64;
    FILE* stream;

    size_t encodedSize = 4*ceil((double) message.size() / 3);

    auto buffer = std::make_unique<char[]>(encodedSize + 1);

    stream = fmemopen(buffer.get(), encodedSize + 1, "w");
    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new_fp(stream, BIO_NOCLOSE);
    bio = BIO_push(b64, bio);
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(bio, message.data(), static_cast<int>(message.size()));
    (void) BIO_flush(bio);
    BIO_free_all(bio);
    fclose(stream);

    std::string res(buffer.get());
    return res;
}

std::vector<unsigned char> base64Decode(const std::string &b64message) {
    BIO *bio;
    BIO *b64;
    size_t decodedLength = calcDecodeLength(b64message);

    auto buffer = std::make_unique<unsigned char[]>(decodedLength + 1);
    //unsigned char *buffer = (unsigned char *) malloc(decodedLength + 1);
    FILE* stream = fmemopen((char *) b64message.c_str(), b64message.size(), "r");

    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new_fp(stream, BIO_NOCLOSE);
    bio = BIO_push(b64, bio);
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
    decodedLength = BIO_read(bio, buffer.get(), static_cast<int>(b64message.size()));
    buffer[decodedLength] = '\0';

    BIO_free_all(bio);
    fclose(stream);

    std::vector<unsigned char> data;
    data.reserve(decodedLength);
    for (int i = 0; i < decodedLength; i++) data.push_back(buffer[i]);

    return data;
}

namespace cserve {

    IIIFEssentials::IIIFEssentials(const std::string &datastr)
    : _is_set(false), _use_icc(false), _hash_type(none) {
        parse(datastr);
    }

    IIIFEssentials::IIIFEssentials(const IIIFEssentials &other) {
        _is_set = other._is_set;
        _origname = other._origname;
        _mimetype = other._mimetype;
        _hash_type = other._hash_type;
        _data_chksum = other._data_chksum;
        _use_icc = other._use_icc;
        _icc_profile = other._icc_profile;
    }

    IIIFEssentials::IIIFEssentials(IIIFEssentials &&other) noexcept : _hash_type(HashType::none), _is_set(false), _use_icc(false) {
        _is_set = other._is_set;
        _origname = other._origname;
        _mimetype = other._mimetype;
        _hash_type = other._hash_type;
        _data_chksum = other._data_chksum;
        _use_icc = other._use_icc;
        _icc_profile = other._icc_profile;

        other._is_set = false;
        other._origname.clear();
        other._mimetype.clear();
        other._hash_type = HashType::none;
        other._data_chksum.clear();
        other._use_icc = false;
        other._icc_profile.clear();
    }

    IIIFEssentials::IIIFEssentials(
            const std::string &origname_p,
            const std::string &mimetype_p,
            HashType hash_type_p,
            const std::string &data_chksum_p,
            const std::vector<unsigned char> &icc_profile_p) :
    _origname(origname_p),
    _mimetype(mimetype_p),
    _hash_type(hash_type_p),
    _data_chksum(data_chksum_p) {
        _is_set = true;
        _use_icc = false;
        if (!icc_profile_p.empty()) {
            _icc_profile = base64Encode(icc_profile_p);
            _use_icc = true;
        }
    }

    IIIFEssentials::IIIFEssentials(
            const std::string &origname_p,
            const std::string &mimetype_p,
            HashType hash_type_p,
            const std::string &data_chksum_p) :
            _origname(origname_p),
            _mimetype(mimetype_p),
            _hash_type(hash_type_p),
            _data_chksum(data_chksum_p) {
        _is_set = true;
        _use_icc = false;
        _icc_profile = {};
    }

    IIIFEssentials &IIIFEssentials::operator=(const IIIFEssentials &other) {
        if (this != &other) {
            _is_set = other._is_set;
            _origname = other._origname;
            _mimetype = other._mimetype;
            _hash_type = other._hash_type;
            _data_chksum = other._data_chksum;
            _use_icc = other._use_icc;
            _icc_profile = other._icc_profile;
        }
        return *this;
    }

    IIIFEssentials &IIIFEssentials::operator=(IIIFEssentials &&other) noexcept {
        if (this != &other) {
            _is_set = other._is_set;
            _origname = other._origname;
            _mimetype = other._mimetype;
            _hash_type = other._hash_type;
            _data_chksum = other._data_chksum;
            _use_icc = other._use_icc;
            _icc_profile = other._icc_profile;

            other._is_set = false;
            other._origname.clear();
            other._mimetype.clear();
            other._hash_type = HashType::none;
            other._data_chksum.clear();
            other._use_icc = false;
            other._icc_profile.clear();
        }
        return *this;
    }

    IIIFEssentials &IIIFEssentials::operator=(const std::string &str) {
        IIIFEssentials es{str};
        _is_set = es._is_set;
        _origname = es._origname;
        _mimetype = es._mimetype;
        _hash_type = es._hash_type;
        _data_chksum = es._data_chksum;
        _use_icc = es._use_icc;
        _icc_profile = es._icc_profile;

        return *this;
    }


    std::string IIIFEssentials::hash_type_string() const {
        std::string hash_type_str;
        switch (_hash_type) {
            case HashType::none:    hash_type_str = "none";   break;
            case HashType::md5:     hash_type_str = "md5";   break;
            case HashType::sha1:    hash_type_str = "sha1";   break;
            case HashType::sha256:   hash_type_str = "sha256";   break;
            case HashType::sha384:  hash_type_str = "sha384";   break;
            case HashType::sha512:  hash_type_str = "sha512";   break;
        }
        return hash_type_str;
    }

    void IIIFEssentials::hash_type(const std::string &hash_type_p) {
        if (hash_type_p == "none")        _hash_type = HashType::none;
        else if (hash_type_p == "md5")    _hash_type = HashType::md5;
        else if (hash_type_p == "sha1")   _hash_type = HashType::sha1;
        else if (hash_type_p == "sha256") _hash_type = HashType::sha256;
        else if (hash_type_p == "sha384") _hash_type = HashType::sha384;
        else if (hash_type_p == "sha512") _hash_type = HashType::sha512;
        else _hash_type = HashType::none;
    }

    std::vector<unsigned char> IIIFEssentials::icc_profile() {
        return base64Decode(_icc_profile);
    }

    void IIIFEssentials::icc_profile(const std::vector<unsigned char> &icc_profile_p) {
        _icc_profile = base64Encode(icc_profile_p);
    }

    // for string delimiter
    std::vector<std::string> split (const std::string& s, const std::string& delimiter) {
        size_t pos_start = 0, pos_end, delim_len = delimiter.length();
        std::string token;
        std::vector<std::string> res;

        while ((pos_end = s.find (delimiter, pos_start)) != std::string::npos) {
            token = s.substr (pos_start, pos_end - pos_start);
            pos_start = pos_end + delim_len;
            res.push_back (token);
        }

        res.push_back (s.substr (pos_start));
        return res;
    }

    void IIIFEssentials::parse(const std::string &str) {
        std::vector<std::string> result = split(str, "|");
        _origname = result[0];
        _mimetype = result[1];
        std::string _hash_type_str = result[2];
        if (_hash_type_str == "none") _hash_type = HashType::none;
        else if (_hash_type_str == "md5") _hash_type = HashType::md5;
        else if (_hash_type_str == "sha1") _hash_type = HashType::sha1;
        else if (_hash_type_str == "sha256") _hash_type = HashType::sha256;
        else if (_hash_type_str == "sha384") _hash_type = HashType::sha384;
        else if (_hash_type_str == "sha512") _hash_type = HashType::sha512;
        else _hash_type = HashType::none;
        _data_chksum = result[3];

        if (result.size() > 5) {
            std::string tmp_use_icc = result[4];
            _use_icc = (tmp_use_icc == "USE_ICC");
            if (_use_icc) {
                _icc_profile = result[5];
            }
            else {
                _icc_profile = std::string();
            }
        }
        _is_set = true;
    }
}
