//
// Created by Lukas Rosenthaler on 06.07.22.
//

#ifndef CSERVER_HTTPSENDERROR_H
#define CSERVER_HTTPSENDERROR_H

#include "Connection.h"
#include "../handlers/iiifhandler/IIIFError.h"

namespace cserve {

    extern void send_error(Connection &conn_obj, Connection::StatusCodes code, const std::string &errmsg);

    [[maybe_unused]] extern void send_error(Connection &conn_obj, Connection::StatusCodes code, const IIIFError &err);

    [[maybe_unused]] void send_error(Connection &conn_obj, Connection::StatusCodes code, const Error &err);

    [[maybe_unused]] void send_error(Connection &conn_obj, Connection::StatusCodes code);
}

#endif //CSERVER_HTTPSENDERROR_H
