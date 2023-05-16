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
#include "ConfValue.h"

#include <utility>
#include <algorithm>

namespace cserve {

    extern size_t data_volume(const std::string &volstr) {
        size_t l = volstr.length();
        size_t ll;
        size_t data_volume;
        char c = '\0';

        if (l > 1) {
            c = static_cast<char>(toupper(volstr[l - 1]));
            ll = 1;
        }
        if ((l > 2) && (c == 'B')) {
            c = static_cast<char>(toupper(volstr[l - 2]));
            ll = 2;
        }
        if (c == 'K') {
            data_volume = stoll(volstr.substr(0, l - ll)) * 1024ll;
        } else if (c == 'M') {
            data_volume = stoll(volstr.substr(0, l - ll)) * 1024ll * 1024ll;
        } else if (c == 'G') {
            data_volume = stoll(volstr.substr(0, l - ll)) * 1024ll * 1024ll * 1024ll;
        } else if (c == 'T') {
            data_volume = stoll(volstr.substr(0, l - ll)) * 1024ll * 1024ll * 1024ll * 1024ll;
        } else {
            data_volume = stoll(volstr);
        }
        return data_volume;
    }

    extern std::string data_volume(size_t size) {
        std::string size_str;
        if (size / (1024ll * 1024ll * 1024ll * 1024ll) > 0) {
            size_str = fmt::format("{:.2}TB", (float) size / (float) (1024ll * 1024ll * 1024ll * 1024ll));
        } else if (size / (1024ll * 1024ll * 1024ll) > 0) {
            size_str = fmt::format("{:.2}GB", (float) size / (float) (1024ll * 1024ll * 1024ll));
        } else if (size / (1024ll * 1024ll) > 0) {
            size_str = fmt::format("{:.2}MB", (float) size / (float) (1024ll * 1024ll));
        } else {
            size_str = fmt::format("{}B", size);
        }
        return size_str;
    }

    ConfValue::ConfValue(std::string prefix,
                         std::string optionname,
                         bool bvalue,
                         std::string description,
                         std::string envname,
                         const std::shared_ptr<CLI::App> &app)
            : _prefix(std::move(prefix)),
              _optionname(optionname),
              _bool_value(bvalue),
              _description(std::move(description)),
              _envname(std::move(envname)),
              _value_type(BOOL) {
        app->add_flag(std::move(optionname), _bool_value, _description)
        ->envname(_envname);
    }

    ConfValue::ConfValue(std::string prefix,
                         std::string optionname,
                         int ivalue,
                         std::string description,
                         std::string envname,
                         const std::shared_ptr<CLI::App> &app)
            : _prefix(std::move(prefix)),
              _optionname(optionname),
              _int_value(ivalue),
              _description(std::move(description)),
              _envname(std::move(envname)),
              _value_type(INTEGER) {
        app->add_option(std::move(optionname), _int_value, _description)
                ->envname(_envname)
                ->check(CLI::TypeValidator<int>());
    }

    ConfValue::ConfValue(std::string prefix,
                         std::string optionname,
                         float fvalue, std::string description,
                         std::string envname,
                         const std::shared_ptr<CLI::App> &app)
            : _prefix(std::move(prefix)),
              _optionname(optionname),
              _float_value(fvalue),
              _description(std::move(description)),
              _envname(std::move(envname)),
              _value_type(FLOAT) {
        app->add_option(std::move(optionname), _float_value, _description)
                ->envname(_envname)
                ->check(CLI::Number);
    }

    ConfValue::ConfValue(std::string prefix,
                         std::string optionname,
                         const char *cstr,
                         std::string description,
                         std::string envname,
                         const std::shared_ptr<CLI::App> &app)
            : _prefix(std::move(prefix)),
              _optionname(optionname),
              _string_value(cstr),
              _description(std::move(description)),
              _envname(std::move(envname)),
              _value_type(STRING) {
        app->add_option(std::move(optionname), _string_value, _description)
                ->envname(_envname);
    }

    ConfValue::ConfValue(std::string prefix,
                         std::string optionname,
                         std::string str,
                         std::string description,
                         std::string envname,
                         const std::shared_ptr<CLI::App> &app)
            : _prefix(std::move(prefix)),
              _optionname(optionname),
              _string_value(std::move(str)),
              _description(std::move(description)),
              _envname(std::move(envname)),
              _value_type(STRING) {
        app->add_option(std::move(optionname), _string_value, _description)
                ->envname(_envname);
    }

    ConfValue::ConfValue(std::string prefix,
                         std::string optionname,
                         std::vector<std::string> strvec,
                         std::string description,
                         std::string envname,
                         const std::shared_ptr<CLI::App> &app)
            : _prefix(std::move(prefix)),
              _optionname(optionname),
              _stringvec_value(std::move(strvec)),
              _description(std::move(description)),
              _envname(std::move(envname)),
              _value_type(STRINGVEC) {
        app->add_option(std::move(optionname), _string_value, _description)
                ->envname(_envname);
    }

    ConfValue::ConfValue(std::string prefix,
                         std::string optionname,
                         const DataSize &ds,
                         std::string description,
                         std::string envname,
                         const std::shared_ptr<CLI::App> &app)
            : _prefix(std::move(prefix)),
              _optionname(optionname),
              _datasize_value(ds),
              _description(std::move(description)),
              _envname(std::move(envname)),
              _value_type(DATASIZE) {
        app->add_option(std::move(optionname), _datasize_value.size_ref(), _description)
                ->envname(_envname)->transform(CLI::AsSizeValue(false));
    }

    ConfValue::ConfValue(std::string prefix,
                         std::string optionname,
                         spdlog::level::level_enum loglevel,
                         std::string description,
                         std::string envname,
                         const std::shared_ptr<CLI::App> &app)
            : _prefix(std::move(prefix)),
              _optionname(optionname), _loglevel_value(loglevel),
              _description(std::move(description)),
              _envname(std::move(envname)),
              _value_type(LOGLEVEL) {
        std::vector<std::pair<std::string, spdlog::level::level_enum>> logLevelMap{
                {"TRACE",    spdlog::level::trace},
                {"DEBUG",    spdlog::level::debug},
                {"INFO",     spdlog::level::info},
                {"WARN",     spdlog::level::warn},
                {"ERR",      spdlog::level::err},
                {"CRITICAL", spdlog::level::critical},
                {"OFF",      spdlog::level::off}
        };
        app->add_option(std::move(optionname), _loglevel_value, _description)
                ->envname(_envname)
                ->transform(CLI::CheckedTransformer(logLevelMap, CLI::ignore_case));
    }

    ConfValue::ConfValue(std::string prefix,
                         std::string optionname,
                         const std::vector<RouteInfo> &lua_routes,
                         std::string description,
                         std::string envname,
                         const std::shared_ptr<CLI::App> &app)
            : _prefix(std::move(prefix)),
              _optionname(optionname),
              _description(std::move(description)),
              _envname(std::move(envname)),
              _value_type(LUAROUTES) {
        for (auto &r: lua_routes) { _luaroutes_value.push_back(r.to_string()); }
        app->add_option(std::move(optionname), _luaroutes_value, _description)
                ->envname(_envname);
    }

    std::ostream & operator<<(std::ostream &os, const std::shared_ptr<ConfValue>& p) {
        switch (p->_value_type) {
            case ConfValue::DataType::BOOL: return os << "BOOL: " << p->get_bool().value_or(false);
            case ConfValue::DataType::INTEGER: return os << "INTEGER: " << p->get_int().value_or(9999);
            case ConfValue::DataType::FLOAT: return os << "FLOAT: " << p->get_float().value_or(9999.9999);
            case ConfValue::DataType::STRING: return os << "STRING: " << p->get_string().value_or("--UNDEFINED--");
            case ConfValue::DataType::STRINGVEC: {
                std::string str;
                std::vector<std::string> vv{{"--UNDEFINED--"}};
                for (const auto& sv: p->get_stringvec().value_or(vv)) {
                    str += " " + sv;
                }
                return os << "STRINGVEC:" << str;
            }
            case ConfValue::DataType::DATASIZE: return os << "DATASIZE: " << p->get_datasize().value_or(DataSize("0B")).as_string();
            case ConfValue::DataType::LOGLEVEL: return os << "LOGLEVEL: " << p->get_loglevel_as_string().value_or("--UNDEFINED--");
            case ConfValue::DataType::LUAROUTES: {
                std::string routes;
                for (auto& r: p->get_luaroutes().value_or(std::vector<RouteInfo>{})) {
                    routes += r.method_as_string() + ":" + r.route + ":" + r.additional_data + " ";
                }
                return os << "LUAROUTES: " << routes;
            }
        }
    }

} // cserve