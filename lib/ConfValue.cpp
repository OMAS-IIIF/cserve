//
// Created by Lukas Rosenthaler on 21.06.22.
//

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
            data_volume = stoll(volstr.substr(0, l - ll)) * 1024;
        } else if (c == 'M') {
            data_volume = stoll(volstr.substr(0, l - ll)) * 1024 * 1024;
        } else if (c == 'G') {
            data_volume = stoll(volstr.substr(0, l - ll)) * 1024 * 1024 * 1024;
        } else if (c == 'T') {
            data_volume = stoll(volstr.substr(0, l - ll)) * 1024 * 1024 * 1024 * 1024;
        } else {
            data_volume = stoll(volstr);
        }
        return data_volume;
    }

    extern std::string data_volume(size_t size) {
        std::string size_str;
        if (size / (1024ll * 1024 * 1024ll * 1024ll) > 0) {
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

    ConfValue::ConfValue(std::string optionname, int ivalue, std::string description, std::string envname,
                         const std::shared_ptr<CLI::App> &app)
            : _optionname(optionname), _int_value(ivalue), _description(std::move(description)),
              _envname(std::move(envname)), _value_type(INTEGER) {
        app->add_option(std::move(optionname), _int_value, _description)
                ->envname(_envname)
                ->check(CLI::TypeValidator<int>());
    }

    ConfValue::ConfValue(std::string optionname, float fvalue, std::string description, std::string envname,
                         const std::shared_ptr<CLI::App> &app)
            : _optionname(optionname), _float_value(fvalue), _description(std::move(description)),
              _envname(std::move(envname)), _value_type(FLOAT) {
        app->add_option(std::move(optionname), _float_value, _description)
                ->envname(_envname)
                ->check(CLI::Number);
    }

    ConfValue::ConfValue(std::string optionname, const char *cstr, std::string description, std::string envname,
                         const std::shared_ptr<CLI::App> &app)
            : _optionname(optionname), _string_value(cstr), _description(std::move(description)),
              _envname(std::move(envname)), _value_type(STRING) {
        app->add_option(std::move(optionname), _string_value, _description)
                ->envname(_envname);
    }

    ConfValue::ConfValue(std::string optionname, std::string str, std::string description, std::string envname,
                         const std::shared_ptr<CLI::App> &app)
            : _optionname(optionname), _string_value(std::move(str)), _description(std::move(description)),
              _envname(std::move(envname)), _value_type(STRING) {
        app->add_option(std::move(optionname), _string_value, _description)
                ->envname(_envname);
    }

    ConfValue::ConfValue(std::string optionname, const DataSize &ds, std::string description, std::string envname,
                         const std::shared_ptr<CLI::App> &app)
            : _optionname(optionname), _datasize_value(ds), _description(std::move(description)),
              _envname(std::move(envname)), _value_type(DATASIZE) {
        app->add_option(std::move(optionname), _datasize_value.size_ref(), _description)
                ->envname(_envname)->transform(CLI::AsSizeValue(0));
    }

    ConfValue::ConfValue(std::string optionname, spdlog::level::level_enum loglevel, std::string description,
                         std::string envname, const std::shared_ptr<CLI::App> &app)
            : _optionname(optionname), _loglevel_value(loglevel), _description(std::move(description)),
              _envname(std::move(envname)), _value_type(LOGLEVEL) {
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

    ConfValue::ConfValue(std::string optionname, std::vector<LuaRoute> lua_routes, std::string description,
                         std::string envname, const std::shared_ptr<CLI::App> &app)
            : _optionname(optionname), _luaroutes_value(std::move(lua_routes)), _description(std::move(description)),
              _envname(std::move(envname)), _value_type(LUAROUTES) {
        std::vector<std::string> tmpstr;
        for (auto &r: _luaroutes_value) { tmpstr.push_back(r.to_string()); }
        app->add_option(std::move(optionname), tmpstr, _description)
                ->envname(_envname);
    }

} // cserve