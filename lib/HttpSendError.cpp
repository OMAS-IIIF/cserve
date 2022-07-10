//
// Created by Lukas Rosenthaler on 06.07.22.
//
#include <sstream>

#include "Cserve.h"
#include "HttpSendError.h"

namespace cserve {

    /*!
 * Sends an HTTP error response to the client, and logs the error if appropriate.
 *
 * \param conn_obj the server connection.
 * \param code the HTTP status code to be returned.
 * \param errmsg the error message to be returned.
 */
    void send_error(Connection &conn_obj, Connection::StatusCodes code, const std::string &errmsg) {
        std::string http_err_name{};
        try {
            conn_obj.status(code);
            conn_obj.setBuffer();
            conn_obj.header("Content-Type", "text/plain");

            switch (code) {
                case Connection::BAD_REQUEST:
                    http_err_name = "Bad Request";
                    break;

                case Connection::FORBIDDEN:
                    http_err_name = "Forbidden";
                    break;

                case Connection::UNAUTHORIZED:
                    http_err_name = "Unauthorized";
                    break;

                case Connection::NOT_FOUND:
                    http_err_name = "Not Found";
                    break;

                case Connection::INTERNAL_SERVER_ERROR:
                    http_err_name = "Internal Server Error";
                    break;

                case Connection::NOT_IMPLEMENTED:
                    http_err_name = "Not Implemented";
                    break;

                case Connection::SERVICE_UNAVAILABLE:
                    http_err_name = "Service Unavailable";
                    break;

                default:
                    http_err_name = "Unknown error";
                    break;
            }

            // Send an error message to the client.

            conn_obj << http_err_name;

            if (!errmsg.empty()) {
                conn_obj << ": " << errmsg;
            }

            conn_obj.flush();
        }
        catch (InputFailure &err) {}

        cserve::Server::logger()->error("GET {} failed ({})", conn_obj.uri(), http_err_name);
    }

    /*!
     * Sends an HTTP error response to the client, and logs the error if appropriate.
     *
     * \param conn_obj the server connection.
     * \param code the HTTP status code to be returned.
     * \param err an exception describing the error.
     */
     void send_error(Connection &conn_obj, Connection::StatusCodes code, const IIIFError &err) {
        send_error(conn_obj, code, err.to_string());
     }

    /*!
     * Sends an HTTP error response to the client, and logs the error if appropriate.
     *
     * \param conn_obj the server connection.
     * \param code the HTTP status code to be returned.
     * \param err an exception describing the error.
     */
    void send_error(Connection &conn_obj, Connection::StatusCodes code, const Error &err) {
        send_error(conn_obj, code, err.to_string());
    }

    /*!
     * Sends an HTTP error response to the client, and logs the error if appropriate.
     *
     * \param conn_obj the server connection.
     * \param code the HTTP status code to be returned.
     */
    void send_error(Connection &conn_obj, Connection::StatusCodes code) {
        send_error(conn_obj, code, "");
    }
    //=========================================================================

}
