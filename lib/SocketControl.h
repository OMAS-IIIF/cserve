/**
 *
 */
#ifndef __defined_cserve_socketcontrol_h
#define __defined_cserve_socketcontrol_h

#include <map>
#include <vector>
#include <queue>
#include <unordered_set>
#include <mutex>
#include <csignal>
#include <iostream>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h> //inet_addr
#include <unistd.h>
#include <poll.h>
#include <string.h>

#ifdef CSERVE_ENABLE_SSL

#include "openssl/bio.h"
#include "openssl/ssl.h"
#include "openssl/err.h"

#endif

#include "Error.h"
#include "ThreadControl.h"



namespace cserve {


    class SocketControl {
    public:
        enum ControlMessageType {
            NOOP, PROCESS_REQUEST, FINISHED_AND_CONTINUE, FINISHED_AND_CLOSE, SOCKET_CLOSED, EXIT, ERROR
        };
        enum SocketType {
            CONTROL_SOCKET, STOP_SOCKET, HTTP_SOCKET, SSL_SOCKET, DYN_SOCKET
        };

        class SocketInfo {
        public:
            ControlMessageType type;
            SocketType socket_type;
            int sid;
#ifdef CSERVE_ENABLE_SSL
            SSL *ssl_sid;
#endif
            char peer_ip[INET6_ADDRSTRLEN];
            int peer_port;

            SocketInfo(ControlMessageType type = NOOP,
                       SocketType socket_type = CONTROL_SOCKET,
                       int sid = -1,
                       SSL * ssl_sid = nullptr,
                       char *_peer_ip = nullptr,
                       int peer_port = -1) : type(type), socket_type(socket_type), sid(sid), ssl_sid(ssl_sid), peer_port(peer_port)
            {
                if (_peer_ip == nullptr) {
                    for (int i = 0; i < INET6_ADDRSTRLEN; i++) peer_ip[i] = '\0';
                } else {
                    strncpy(peer_ip, _peer_ip, INET6_ADDRSTRLEN - 1);
                    peer_ip[INET6_ADDRSTRLEN - 1] = '\0';
                }
            }

            SocketInfo(const SocketInfo &si) {
                sid = si.sid;
#ifdef CSERVE_ENABLE_SSL
                ssl_sid = si.ssl_sid;
#endif
                for (int i = 0; i < INET6_ADDRSTRLEN; i++) peer_ip[i] = si.peer_ip[i];
                peer_port = si.peer_port;
            }


            SocketInfo operator=(const SocketInfo &si) {
                sid = si.sid;
#ifdef CSERVE_ENABLE_SSL
                ssl_sid = si.ssl_sid;
#endif
                for (int i = 0; i < INET6_ADDRSTRLEN; i++) peer_ip[i] = si.peer_ip[i];
                peer_port = si.peer_port;
                return *this;
            }

        };

        struct socket_info_hash {
            size_t operator()(SocketInfo const &sockid) const noexcept {
                return (sockid.sid);
            }
        };


    private:
        std::mutex sockets_mutex; //!> protecting mutex
        std::vector<pollfd> open_sockets; //!> open sockets waiting for reading
        std::vector<SocketInfo> generic_open_sockets;//!> open socket-info's waiting for reading
        std::queue<SocketInfo> waiting_sockets; //!> Sockets that have input and are waiting for the thread
        //std::unordered_set<SocketInfo, socket_info_hash> working_sockets; //!> Socket's that are currently working
        int n_msg_sockets; //!> Number of sockets communicating with the threads
        int stop_sock_id; //!> Index of the stopsocket (the thread that catches signals sens to this socket)
        int http_sock_id; //!> Index of the HTTP socckel
        int ssl_sock_id; //!> Index of the SSL socket
        int dyn_socket_base; //!> base index of the dynanic sockets created by accept

    public:
        /*!
         * Initialize the socket control.
         *
         * The SocketControl instance manages the sockets of the server. These consist of
         * internal sockets for the communication with the threads, and the HTTP sockets
         * from the requests from the outside.
         *
         * @param thread_control ThreadControl instance which is used to initialize the
         * sockets for the internal communication with the threads. The Initial sockets
         * are added to the idling sockets pool
         */
        SocketControl(ThreadControl &thread_control);

        pollfd *get_sockets_arr();

        int get_sockets_size() { return generic_open_sockets.size(); }

        int get_n_msg_sockets() { return n_msg_sockets; }

        void add_stop_socket(int sid);

        int get_stop_socket_id() { return stop_sock_id; }

        void add_http_socket(int sid);

        int get_http_socket_id() { return http_sock_id; }

        void add_ssl_socket(int sid);

        int get_ssl_socket_id() { return ssl_sock_id; }

        void add_dyn_socket(SocketInfo sockid);

        int get_dyn_socket_base() { return dyn_socket_base; }

        int size() { return generic_open_sockets.size(); }

        const pollfd &operator[](int index);

        void remove(int pos, SocketInfo &sockid);


        /*!
         *
         * @param offset
         * @param sockid
         * @return
         */
        bool open_pop(int offset, SocketInfo &sockid);

        void move_to_waiting(int pos);

        bool get_waiting(SocketInfo &sockid);

        static int send_control_message(int pipe_id, const SocketInfo &msg);

        static SocketInfo receive_control_message(int pipe_id);

        void broadcast_exit();

        void close_all_dynsocks(int (*closefunc)(const SocketInfo&));

    };

}


#endif //SIPI_SOCKETCONTROL_H
