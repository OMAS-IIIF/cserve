//
// Created by Lukas Rosenthaler on 06.07.22.
//
#include <sstream>
#include <iomanip>

#include "Error.h"
#include "HttpHelpers.h"


static const char file_[] = __FILE__;

namespace cserve {

    std::pair<std::string, std::string> strsplit(const std::string &str, const char c) {
        size_t pos;
        if ((pos = str.find(c)) != std::string::npos) {
            std::string s1 = str.substr(0, pos);
            std::string s2 = str.substr(pos + 1);
            return make_pair(s1, s2);
        } else {
            std::string emptystr;
            return make_pair(str, emptystr);
        }
    }

    size_t safeGetline(std::istream &is, std::string &t, size_t max_n) {
        t.clear();

        size_t n = 0;
        for (;;) {
            int c;
            c = is.get();
            switch (c) {
                case '\n':
                    n++;
                    return n;
                case '\r':
                    n++;
                    if (is.peek() == '\n') {
                        is.get();
                        n++;
                    }
                    return n;
                case EOF:
                    return n;
                default:
                    n++;
                    t += (char) c;  // ToDo: check t exhausting memory....
            }
            if ((max_n > 0) && (n >= max_n)) {
                throw Error(file_, __LINE__, "Input line too long!");
            }
        }
    }

    std::string urldecode(const std::string &src, bool form_encoded) {

#define HEXTOI(x) (isdigit(x) ? (x) - '0' : (x) - 'W')

        std::stringstream outss;
        size_t start = 0;
        size_t pos;

        while ((pos = src.find('%', start)) != std::string::npos) {
            if ((pos + 2) < src.length()) {
                if (isxdigit(src[pos + 1]) && isxdigit(src[pos + 2])) {
                    std::string tmpstr = src.substr(start, pos - start);

                    if (form_encoded) {
                        for (char & i : tmpstr) {
                            if (i == '+') i = ' ';
                        }
                    }

                    outss << tmpstr;

                    //
                    // we have a valid hex number
                    //
                    char a = (char) tolower(src[pos + 1]);
                    char b = (char) tolower(src[pos + 2]);
                    char c = (char) ((HEXTOI(a) << 4) | HEXTOI(b));
                    outss << c;
                    start = pos + 3;
                } else {
                    pos += 3;
                    std::string tmpstr = src.substr(start, pos - start);
                    if (form_encoded) {
                        for (char & i : tmpstr) {
                            if (i == '+') i = ' ';
                        }
                    }
                    outss << tmpstr;
                    start = pos;
                }
            }
        }

        outss << src.substr(start, src.length() - start);
        return outss.str();
    }//=========================================================================

    [[maybe_unused]] std::string urlencode(const std::string &value) {
        std::ostringstream escaped;
        escaped.fill('0');
        escaped << std::hex;

        for (char c : value) {
            // Keep alphanumeric and other accepted characters intact
            if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
                escaped << c;
                continue;
            }

            // Any other characters are percent-encoded
            escaped << std::uppercase;
            escaped << '%' << std::setw(2) << int((unsigned char) c);
            escaped << std::nouppercase;
        }

        return escaped.str();
    }


    std::unordered_map<std::string, std::string> parse_header_options(const std::string &options, bool form_encoded, char sep) {
        std::vector<std::string> params;
        size_t pos = 0;
        size_t old_pos = 0;
        while ((pos = options.find(sep, pos)) != std::string::npos) {
            pos++;
            if (pos == 1) { // if first char is a token skip it!
                old_pos = pos;
                continue;
            }
            params.push_back(options.substr(old_pos, pos - old_pos - 1));
            old_pos = pos;
        }
        if (old_pos != options.length()) {
            params.push_back(options.substr(old_pos, std::string::npos));
        }
        std::unordered_map<std::string, std::string> q;
        std::string name;
        std::string value;
        for (auto & param : params) {
            if ((pos = param.find('=')) != std::string::npos) {
                name = urldecode(param.substr(0, pos), form_encoded);
                value = urldecode(param.substr(pos + 1, std::string::npos), form_encoded);
            } else {
                name = urldecode(param, form_encoded);
            }
            trim(name);
            trim(value);
            asciitolower(name);
            q[name] = value;
        }
        return q;
    }

    std::unordered_map<std::string, std::string> parse_query_string(const std::string &query, bool form_encoded) {
        std::vector<std::string> params;
        size_t pos = 0;
        size_t old_pos = 0;

        while ((pos = query.find('&', pos)) != std::string::npos) {
            pos++;

            if (pos == 1) { // if first char is a token skip it!
                old_pos = pos;
                continue;
            }

            params.push_back(query.substr(old_pos, pos - old_pos - 1));

            old_pos = pos;
        }

        if (old_pos != query.length()) {
            params.push_back(query.substr(old_pos, std::string::npos));
        }

        std::unordered_map<std::string, std::string> q;

        for (auto & param : params) {
            std::string name;
            std::string value;

            if ((pos = param.find('=')) != std::string::npos) {
                name = urldecode(param.substr(0, pos), form_encoded);
                value = urldecode(param.substr(pos + 1, std::string::npos), form_encoded);
            } else {
                name = urldecode(param, form_encoded);
            }

            //asciitolower(name);
            q[name] = value;
        }

        return q;
    }

}
