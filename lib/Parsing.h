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
#ifndef __cserve_parsing_h
#define __cserve_parsing_h

#include <string>
#include <regex>
#include <unordered_map>
#include <vector>

namespace cserve::Parsing {
        //static std::unordered_map<std::string, std::string> mimetypes; //! format (key) to mimetype (value) conversion map

        /*!
         * Parses a string containing a MIME type and optional character set, such as the Content-Type header defined by
         * <https://tools.ietf.org/html/rfc7231#section-3.1.1>.
         * @param mimestr a string containing the MIME type.
         * @return the MIME type and optional character set.
         */
        std::pair<std::string, std::string> parseMimetype(const std::string &mimestr);


        /*!
         * Determine the mimetype of a file using the magic number
         *
         * \param[in] fpath Path to file to check for the mimetype
         * \returns pair<string,string> containing the mimetype as first part
         *          and the charset as second part. Access as val.first and val.second!
         */
        std::pair<std::string, std::string> getFileMimetype(const std::string &fpath);

        /*!
         *
         * \param[in] fpath Path to file to check for the mimetype
         * \returns Best mimetype given magic number and extension
         */
        std::string getBestFileMimetype(const std::string &fpath);

        /*!
         *
         * @param path Path to file (
         * @param filename Original filename with extension
         * @param given_mimetype The mimetype given by the browser
         * @return true, if consistent, else false
         */
        bool checkMimeTypeConsistency(
                const std::string &path,
                const std::string &filename,
                const std::string &given_mimetype = "");


        /*!
         * Parses an integer.
         * @param str the string to be parsed.
         * @return the corresponding integer.
         */
        [[maybe_unused]] size_t parse_int(std::string &str);

        /*!
         * Parses a floating-point number containing only digits and an optional decimal point.
         *
         * @param str the string to be parsed.
         * @return the corresponding floating-point number.
         */
        [[maybe_unused]] float parse_float(std::string &str);
    }

#endif //__cserve_parsing_h
