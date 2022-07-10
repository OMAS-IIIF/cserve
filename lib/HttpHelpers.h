//
// Created by Lukas Rosenthaler on 06.07.22.
//

#ifndef CSERVER_HTTPHELPERS_H
#define CSERVER_HTTPHELPERS_H

#include <string>
#include <istream>
#include <unordered_map>
#include <vector>

namespace cserve {
    /*!
     * Function which splits a string at the first occurrence of a given character
     *
     * \params[in] str String to be split
     * \param[in] c The character which indicates the split position in the string
     *
     * \returns A pair of strings
     */
    extern std::pair<std::string, std::string> strsplit(const std::string &str, char c);

    /*!
     *
     * @param[in] is Input stream (HTTP socket stream)
     * @param[out[ t Input stream (HTTP socket stream)
     * @param[in] max_n Maximum length of line accepted. If 0, the length of the line is unlimited. The string t
     *            is expanded as necessary.
     * @return Number of bytes read
     */
    extern size_t safeGetline(std::istream &is, std::string &t, size_t max_n = 0);

    // trim from start (in place)
    inline extern void ltrim(std::string &s) {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
            return !std::isspace(ch);
        }));
    }

    // trim from end (in place)
    inline extern void rtrim(std::string &s) {
        s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
            return !std::isspace(ch);
        }).base(), s.end());
    }

    // trim from both ends (in place)
    inline extern void trim(std::string &s) {
        ltrim(s);
        rtrim(s);
    }

    // trim from start (copying)
    inline extern std::string ltrim_copy(std::string s) {
        ltrim(s);
        return s;
    }

    // trim from end (copying)
    inline extern std::string rtrim_copy(std::string s) {
        rtrim(s);
        return s;
    }

    // trim from both ends (copying)
    inline extern std::string trim_copy(std::string s) {
        trim(s);
        return s;
    }

    /*!
     * convert a string to all lowercase characters. The string should contain
     * only ascii characters. The outcome of non-ascii characters is undefined.
     *
     * \param[in] str Input string with mixed case
     *
     * \returns String converted to all lower case
     */
    [[maybe_unused]] inline extern void asciitolower(std::string &str) { std::transform(str.begin(), str.end(), str.begin(), ::tolower); }

    /*!
     * convert a string to all uppercase characters. The string should contain
     * only ascii characters. The outcome of non-ascii characters is undefined.
     *
     * \param[in] str Input string with mixed case
     *
     * \returns String converted to all upper case
     */
    [[maybe_unused]] inline extern void asciitoupper(std::string &str) { std::transform(str.begin(), str.end(), str.begin(), ::toupper); }

    /*!
     * urldecode is used to decode an according to the HTTP-standard urlencoded string
     *
     * \param[in] src Encoded string
     * \param[in] form_encoded Boolean which must be true if the string has been part
     *            of a form
     *
     *            \returns Decoded string
     */
    std::string extern urldecode(const std::string &src, bool form_encoded = false);

    /*!
     * Encode a string (value) acording the the rules of urlencoded strings
     *
     * \param[in] value String to be encoded
     *
     * \returns Encoded string
     */
    [[maybe_unused]] extern std::string urlencode(const std::string &value);


    /*!
     * Function to parse the options of a HTTP header line
     *
     * \param[in] options Options-part of the string
     * \param[in] form_encoded Has to be set to true, if it is form_encoded data [default: false]
     * \param[in] sep Separator character between options [default: ';']
     *
     * \returns map of options (all names converted to lower case!)
     * */
    extern std::unordered_map<std::string, std::string>
    parse_header_options(const std::string &options, bool form_encoded = false, char sep = ';');

    extern std::unordered_map<std::string, std::string> parse_query_string(const std::string &query, bool form_encoded = false);
}

#endif //CSERVER_HTTPHELPERS_H
