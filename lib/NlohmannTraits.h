//
// Created by Lukas Rosenthaler on 07.08.21.
//

#ifndef CSERVER_LIB_NLOHMANNTRAITS_H_
#define CSERVER_LIB_NLOHMANNTRAITS_H_

#include <jwt-cpp/jwt.h>
#include <nlohmann/json.hpp>

struct nlohmann_traits {
    using json = nlohmann::json;
    using value_type = json;
    using object_type = json::object_t;
    using array_type = json::array_t;
    using string_type = std::string;
    using number_type = json::number_float_t;
    using integer_type = json::number_integer_t;
    using boolean_type = json::boolean_t;

    static jwt::json::type get_type(const json &val) {
        using jwt::json::type;

        if (val.type() == json::value_t::boolean)
            return type::boolean;
        else if (val.type() == json::value_t::number_integer)
            return type::integer;
        else if (val.type() == json::value_t::number_unsigned) // nlohmann internally tracks two types of integers
            return type::integer;
        else if (val.type() == json::value_t::number_float)
            return type::number;
        else if (val.type() == json::value_t::string)
            return type::string;
        else if (val.type() == json::value_t::array)
            return type::array;
        else if (val.type() == json::value_t::object)
            return type::object;
        else
            throw std::logic_error("invalid type");
    }

    static json::object_t as_object(const json &val) {
        if (val.type() != json::value_t::object) throw std::bad_cast();
        return val.get<json::object_t>();
    }

    static std::string as_string(const json &val) {
        if (val.type() != json::value_t::string) throw std::bad_cast();
        return val.get<std::string>();
    }

    static json::array_t as_array(const json &val) {
        if (val.type() != json::value_t::array) throw std::bad_cast();
        return val.get<json::array_t>();
    }

    static int64_t as_integer(const json &val) {
        switch (val.type()) {
            case json::value_t::number_integer:
            case json::value_t::number_unsigned:
                return val.get<int64_t>();
            default:
                throw std::bad_cast();
        }
    }

    static bool as_boolean(const json &val) {
        if (val.type() != json::value_t::boolean) throw std::bad_cast();
        return val.get<bool>();
    }

    static double as_number(const json &val) {
        if (val.type() != json::value_t::number_float) throw std::bad_cast();
        return val.get<double>();
    }

    static bool parse(json &val, std::string str) {
        val = json::parse(str.begin(), str.end());
        return true;
    }

    static std::string serialize(const json &val) { return val.dump(); }
};

#endif //CSERVER_LIB_NLOHMANNTRAITS_H_
