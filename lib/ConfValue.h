//
// Created by Lukas Rosenthaler on 21.06.22.
//

#ifndef CSERVER_CONFVALUE_H
#define CSERVER_CONFVALUE_H

#include <string>
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

        inline std::string as_string() const { return data_volume(size); }

        inline size_t as_size_t() const { return size; }

        inline size_t &size_ref() { return size; }
    };


    class ConfValue {
    public:
        enum DataType {
            INTEGER, FLOAT, STRING, DATASIZE, LOGLEVEL, LUAROUTES
        };
    private:
        DataType _value_type{};
        int _int_value{};
        float _float_value{};
        std::string _string_value{};
        DataSize _datasize_value{};
        spdlog::level::level_enum _loglevel_value{};
        std::vector<LuaRoute> _luaroutes_value{};
        std::string _optionname{};
        std::string _description{};
        std::string _envname{};
    public:
        inline ConfValue() = default;

        ConfValue(std::string optionname, int ivalue, std::string description, std::string envname,
                  const std::shared_ptr<CLI::App> &app);

        ConfValue(std::string optionname, float fvalue, std::string description, std::string envname,
                  const std::shared_ptr<CLI::App> &app);

        ConfValue(std::string optionname, const char *cstr, std::string description, std::string envname,
                  const std::shared_ptr<CLI::App> &app);

        ConfValue(std::string optionname, std::string str, std::string description, std::string envname,
                  const std::shared_ptr<CLI::App> &app);

        ConfValue(std::string optionname, const DataSize &ds, std::string description, std::string envname,
                  const std::shared_ptr<CLI::App> &app);

        ConfValue(std::string optionname, spdlog::level::level_enum loglevel, std::string description,
                  std::string envname, const std::shared_ptr<CLI::App> &app);

        ConfValue(std::string optionname, std::vector<LuaRoute> lua_routes, std::string description,
                  std::string envname, const std::shared_ptr<CLI::App> &app);

        inline ConfValue(const ConfValue &cv)
                : _value_type(cv._value_type), _int_value(cv._int_value), _float_value(cv._float_value),
                  _string_value(cv._string_value),
                  _datasize_value(cv._datasize_value), _loglevel_value(cv._loglevel_value),
                  _luaroutes_value(cv._luaroutes_value), _description(cv._description), _envname(cv._envname) {}

        inline ConfValue &operator=(const ConfValue &cv) {
            _value_type = cv._value_type;
            _int_value = cv._int_value;
            _float_value = cv._float_value;
            _string_value = cv._string_value;
            _datasize_value = cv._datasize_value;
            _loglevel_value = cv._loglevel_value;
            _luaroutes_value = cv._luaroutes_value;
            _description = cv._description;
            _envname = cv._envname;
            return *this;
        }

        inline std::string get_optionname() const { return _optionname; }

        [[nodiscard]] inline DataType get_type() const { return _value_type; }

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


        inline std::optional<std::vector<LuaRoute>> get_luaroutes() const {
            if (_value_type == LUAROUTES) {
                return _luaroutes_value;
            }
            else {
                return {};
            }
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

        inline void set_value(const DataSize &dsval) {
            _datasize_value = dsval;
            _value_type = DATASIZE;
        }

        inline void set_value(spdlog::level::level_enum loglevel_val) {
            _loglevel_value = loglevel_val;
            _value_type = LOGLEVEL;
        }

        inline void set_value(const std::vector<LuaRoute> &luaroutes_val) {
            _luaroutes_value = luaroutes_val;
            _value_type = LUAROUTES;
        }

        inline std::string get_description() { return _description; }

        inline std::string get_envname() { return _envname; }

        inline friend std::ostream & operator<<(std::ostream &os, const std::shared_ptr<ConfValue>& p) {
            switch (p->_value_type) {
                case DataType::INTEGER: return os << "INTEGER: " << p->get_int().value_or(9999);
                case DataType::FLOAT: return os << "FLOAT: " << p->get_float().value_or(9999.9999);
                case DataType::STRING: return os << "STRING: " << p->get_string().value_or("--UNDEFINED--");
                case DataType::DATASIZE: return os << "DATASIZE: " << p->get_datasize().value_or(DataSize("0B")).as_string();
                case DataType::LOGLEVEL: return os << "LOGLEVEL: " << p->get_loglevel_as_string().value_or("--UNDEFINED--");
                case DataType::LUAROUTES: return os << "LUAROUTES: " << "p->get_luaroutes().value_or(std::vector<LuaRoute>{}";
            }
        }
    };


} // cserve

#endif //CSERVER_CONFVALUE_H
