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
#ifndef CSERVER_CONFVALUE_H
#define CSERVER_CONFVALUE_H

#include <string>
#include <vector>
#include <optional>
#include <utility>

#include <spdlog/common.h>

#include "CLI11.hpp"

#include "LuaServer.h"
#include "Global.h"


namespace cserve {

    extern size_t data_volume(const std::string &volstr);

    extern std::string data_volume(size_t size);

    class DataSize {
    private:
        size_t size;

    public:
        inline DataSize() : size(0) {}

        inline explicit DataSize(size_t size) : size(size) {}

        inline explicit DataSize(const char *size_str) { size = data_volume(size_str); }

        inline explicit DataSize(const std::string &size_str) { size = data_volume(size_str); }

        inline bool operator==(const DataSize &other) const { return size == other.size; }

        [[nodiscard]] inline std::string as_string() const { return data_volume(size); }

        [[nodiscard]] inline size_t as_size_t() const { return size; }

        inline size_t &size_ref() { return size; }
    };


    class ConfValue {
    public:
        enum DataType {
            BOOL, INTEGER, FLOAT, STRING, STRINGVEC, DATASIZE, LOGLEVEL, LUAROUTES
        };
    private:
        std::string _prefix;
        DataType _value_type{};
        bool _bool_value{};
        int _int_value{};
        float _float_value{};
        std::string _string_value{};
        std::vector<std::string> _stringvec_value{};
        DataSize _datasize_value{};
        spdlog::level::level_enum _loglevel_value{};
        std::vector<std::string> _luaroutes_value{};
        std::string _optionname{};
        std::string _description{};
        std::string _envname{};
    public:
        inline ConfValue() = default;

        ConfValue(std::string prefix, std::string optionname, bool bvalue, std::string description, std::string envname,
                  const std::shared_ptr<CLI::App> &app);

        ConfValue(std::string prefix, std::string optionname, int ivalue, std::string description, std::string envname,
                  const std::shared_ptr<CLI::App> &app);

        ConfValue(std::string prefix, std::string optionname, float fvalue, std::string description,
                  std::string envname,
                  const std::shared_ptr<CLI::App> &app);

        ConfValue(std::string prefix, std::string optionname, const char *cstr, std::string description,
                  std::string envname,
                  const std::shared_ptr<CLI::App> &app);

        ConfValue(std::string prefix, std::string optionname, std::string str, std::string description,
                  std::string envname,
                  const std::shared_ptr<CLI::App> &app);

        ConfValue(std::string prefix, std::string optionname, std::vector<std::string> strvec, std::string description,
                  std::string envname,
                  const std::shared_ptr<CLI::App> &app);

        ConfValue(std::string prefix, std::string optionname, const DataSize &ds, std::string description,
                  std::string envname,
                  const std::shared_ptr<CLI::App> &app);

        ConfValue(std::string prefix, std::string optionname, spdlog::level::level_enum loglevel,
                  std::string description,
                  std::string envname, const std::shared_ptr<CLI::App> &app);

        ConfValue(std::string prefix, std::string optionname, const std::vector<RouteInfo> &lua_routes,
                  std::string description,
                  std::string envname, const std::shared_ptr<CLI::App> &app);

        inline ConfValue(const ConfValue &cv)
                : _prefix(cv._prefix), _value_type(cv._value_type), _int_value(cv._int_value), _float_value(cv._float_value),
                  _string_value(cv._string_value), _stringvec_value(cv._stringvec_value),
                  _datasize_value(cv._datasize_value), _loglevel_value(cv._loglevel_value),
                  _luaroutes_value(cv._luaroutes_value), _description(cv._description), _envname(cv._envname) {}

        inline ConfValue &operator=(const ConfValue &cv) {
            _prefix = cv._prefix;
            _value_type = cv._value_type;
            _bool_value = cv._bool_value;
            _int_value = cv._int_value;
            _float_value = cv._float_value;
            _string_value = cv._string_value;
            _stringvec_value = cv._stringvec_value;
            _datasize_value = cv._datasize_value;
            _loglevel_value = cv._loglevel_value;
            _luaroutes_value = cv._luaroutes_value;
            _description = cv._description;
            _envname = cv._envname;
            return *this;
        }

        [[nodiscard]] inline std::string get_prefix() const { return _prefix; }

        [[nodiscard]] inline std::string get_optionname() const { return _optionname; }

        [[nodiscard]] inline DataType get_type() const { return _value_type; }

        [[nodiscard]] inline std::optional<bool> get_bool() const {
            if (_value_type == BOOL) {
                return _bool_value;
            }
            else {
                return {};
            }
        }

        [[nodiscard]] inline std::optional<int> get_int() const {
            if (_value_type == INTEGER) {
                return _int_value;
            }
            else {
                return {};
            }
        }

        [[nodiscard]] inline std::optional<float> get_float() const {
            if (_value_type == FLOAT) {
                return _float_value;
            }
            else {
                return {};
            }
        }

        [[nodiscard]] inline std::optional<std::string> get_string() const {
            if (_value_type == STRING) {
                return _string_value;
            }
            else {
                return {};
            }
        }

        [[nodiscard]] inline std::optional<std::vector<std::string>> get_stringvec() const {
            if (_value_type == STRINGVEC) {
                return _stringvec_value;
            }
            else {
                return {};
            }
        }

        [[nodiscard]] inline std::optional<DataSize> get_datasize() const {
            if (_value_type == DATASIZE) {
                return _datasize_value;
            }
            else {
                return {};
            }
        }

        [[nodiscard]] inline std::optional<spdlog::level::level_enum> get_loglevel() const {
            if (_value_type == LOGLEVEL) {
                return _loglevel_value;
            }
            else {
                return {};
            }
        }

        [[nodiscard]] inline std::optional<std::string> get_loglevel_as_string() const {
            if (_value_type == LOGLEVEL) {
                std::unordered_map<spdlog::level::level_enum, std::string> log_level_map{
                        {spdlog::level::trace, "TRACE"},
                        {spdlog::level::debug, "DEBUG"},
                        {spdlog::level::info, "INFO"},
                        {spdlog::level::warn, "WARN"},
                        {spdlog::level::err, "ERR"},
                        {spdlog::level::critical, "CRITICAL"},
                        {spdlog::level::off, "OFF"}
                };
                return log_level_map.at(_loglevel_value);
            }
            else {
                return {};
            }
        }


        [[nodiscard]] inline std::optional<std::vector<RouteInfo>> get_luaroutes() const {
            if (_value_type == LUAROUTES) {
                std::vector<std::string> tmp_str_vec{};
                if ((_luaroutes_value.size() == 1) && (_luaroutes_value[0].find(';'))) {
                    tmp_str_vec = split(_luaroutes_value[0], ';');
                }
                else {
                    tmp_str_vec = _luaroutes_value;
                }
                std::vector<RouteInfo> routes{};
                routes.reserve(tmp_str_vec.size());
                for (auto& rstr: tmp_str_vec) {
                    routes.emplace_back(rstr);
                }
                return routes;
            }
            else {
                return {};
            }
        }

        inline void set_value(bool bval) {
            _bool_value = bval;
            _value_type = BOOL;
        }

        inline void set_value(int ival) {
            _int_value = ival;
            _value_type = INTEGER;
        }

        inline void set_value(float fval) {
            _float_value = fval;
            _value_type = FLOAT;
        }

        inline void set_value(const std::string &strval) {
            _string_value = strval;
            _value_type = STRING;
        }

        inline void set_value(const std::vector<std::string> &strvecval) {
            _stringvec_value = strvecval;
            _value_type = STRINGVEC;
        }

        inline void set_value(const DataSize &dsval) {
            _datasize_value = dsval;
            _value_type = DATASIZE;
        }

        inline void set_value(spdlog::level::level_enum loglevel_val) {
            _loglevel_value = loglevel_val;
            _value_type = LOGLEVEL;
        }

        inline void set_value(const std::vector<RouteInfo> &lua_routes) {
            _luaroutes_value.clear();
            for (auto &r: lua_routes) { _luaroutes_value.push_back(r.to_string()); }
            _value_type = LUAROUTES;
        }

        inline std::string get_description() { return _description; }

        inline std::string get_envname() { return _envname; }

        friend std::ostream & operator<<(std::ostream &os, const std::shared_ptr<ConfValue>& p);
    };

    std::ostream & operator<<(std::ostream &os, const std::shared_ptr<ConfValue>& p);
} // cserve

#endif //CSERVER_CONFVALUE_H
