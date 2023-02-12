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
#ifndef cserve_global_h
#define cserve_global_h

#include <string>
#include <sstream>
#include <istream>
#include <utility>
#include <string>
#include <iostream>
#include <vector>
#include <algorithm>



namespace cserve {

    template<typename Enumeration>
    [[maybe_unused]] inline auto as_integer(Enumeration const value) -> typename std::underlying_type<Enumeration>::type {
        return static_cast<typename std::underlying_type<Enumeration>::type>(value);
    }
    //-------------------------------------------------------------------------


    [[maybe_unused]] inline std::string getFileName(const std::string &s) {
        char sep = '/';
        size_t i = s.rfind(sep, s.length());

        if (i != std::string::npos) {
            return s.substr(i + 1, s.length() - i);
        } else {
            return s;
        }
    }

    inline std::vector<std::string> split(const std::string &str, char delim) {
        std::stringstream test{str};
        std::string segment;
        std::vector<std::string> seglist;
        while(std::getline(test, segment, delim)) {
            seglist.push_back(segment);
        }
        return seglist;
    }

    inline std::string  strtoupper(const std::string &str) {
        std::string  lstr{str};
        std::transform(lstr.begin(), lstr.end(),lstr.begin(), ::toupper);
        return lstr;
    }

    static std::mutex debug_mutex;

    inline void debug_output(int line, const std::string &str) {
        const std::lock_guard<std::mutex> lock(debug_mutex);
        std::cerr << ">>>DEBUG line=" << line << ": " << str << std::endl;
    }

}

#endif
