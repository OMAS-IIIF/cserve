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
#ifndef defined_cserve_error_h
#define defined_cserve_error_h

#include <iostream>
#include <string>
#include <exception>
#include <stdexcept>

namespace cserve {

    /*!
     * \brief Class used to thow errors from the web server implementation
     *
     * This class which inherits from \class std::runtime_error is used to throw catchable
     * errors from the web server. The error contains the cpp-file, line number, a user given
     * description and, if available, the system error message.
     */
    class Error : public std::runtime_error {
    protected:
        std::string cname;
        int line;            //!< Linenumber where the exception has been throwns
        std::string file;    //!< Name of source code file where the exception has been thrown
        std::string message; //!< Description of the problem
        int sysErrno;        //!< If there is a system error number

    public:

        Error(const char *file, int line, const char *msg, int errno_p = 0);

        Error(const char *file, int line, const std::string &msg, int errno_p = 0);

        [[nodiscard]] inline int getLine() const { return line; }

        [[maybe_unused]] [[nodiscard]] inline std::string getFile() const { return file; }

        [[nodiscard]] inline std::string getMessage() const { return message; }

        [[maybe_unused]] [[nodiscard]] inline int getSysErrno() const { return sysErrno; }

        [[nodiscard]] virtual std::string to_string() const;

        inline explicit operator std::string() const {
            return to_string();
        }

        friend std::ostream &operator<<(std::ostream &outStream, const Error &rhs);
    };
}

#endif
