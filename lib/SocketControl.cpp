//
// Created by Lukas Rosenthaler on 08.07.20.
//
#include "SocketControl.h"

static const char __file__[] = __FILE__;

namespace cserve {

    SocketControl::SocketControl(ThreadControl &thread_control) {
        for (int i = 0; i < thread_control.nthreads(); i++) {
            generic_open_sockets.push_back(SocketInfo(NOOP, CONTROL_SOCKET, thread_control[i].control_pipe));
        }
        n_msg_sockets = thread_control.nthreads();
        stop_sock_id = -1;
        http_sock_id = -1;
        ssl_sock_id = -1;
        dyn_socket_base = -1;
    }
    //=========================================================================

    pollfd *SocketControl::get_sockets_arr() {
        open_sockets = std::vector<pollfd>();
        for (auto const &tmp: generic_open_sockets) {
            pollfd pfd;
            pfd.fd = tmp.sid;
            pfd.events = POLLIN;
            pfd.revents = 0;
            open_sockets.push_back(pfd);
        }
        return open_sockets.data();
    }

    void SocketControl::add_stop_socket(int sid) { // only called once
        std::unique_lock<std::mutex> mutex_guard(sockets_mutex);
        if (dyn_socket_base != -1) {
            throw Error(__file__, __LINE__, "Adding stop socket not allowed after adding dynamic sockets!");
        }
        generic_open_sockets.push_back(SocketInfo(NOOP, STOP_SOCKET, sid));
        stop_sock_id = generic_open_sockets.size() - 1;
    }
    //=========================================================================

    void SocketControl::add_http_socket(int sid) { // only called once
        std::unique_lock<std::mutex> mutex_guard(sockets_mutex);
        if (dyn_socket_base != -1) {
            throw Error(__file__, __LINE__, "Adding HTTP socket not allowed after adding dynamic sockets!");
        }
        generic_open_sockets.push_back(SocketInfo(NOOP, HTTP_SOCKET, sid));
        http_sock_id = generic_open_sockets.size() - 1;
    }
    //=========================================================================

    void SocketControl::add_ssl_socket(int sid) { // only called once
        std::unique_lock<std::mutex> mutex_guard(sockets_mutex);
        if (dyn_socket_base != -1) {
            throw Error(__file__, __LINE__, "Adding SSL socket not allowed after adding dynamic sockets!");
        }
        generic_open_sockets.push_back(SocketInfo(NOOP, SSL_SOCKET, sid, nullptr));
        ssl_sock_id = generic_open_sockets.size() - 1;
    }
    //=========================================================================

    void SocketControl::add_dyn_socket(SocketInfo sockid) { // called multiple times, changes open_sockets vector!!!
        std::unique_lock<std::mutex> mutex_guard(sockets_mutex);
        sockid.type = NOOP;
        sockid.socket_type = DYN_SOCKET;
        generic_open_sockets.push_back(sockid);
        if (dyn_socket_base == -1) {
            dyn_socket_base = generic_open_sockets.size() - 1;
        }
    }
    //=========================================================================

    void SocketControl::remove(int pos, SocketInfo &sockid) { // called multiple times, changes open sockets vector!!!
        std::unique_lock<std::mutex> mutex_guard(sockets_mutex);
        if ((pos >= 0) && (pos < generic_open_sockets.size())) {
            sockid = generic_open_sockets[pos];
            generic_open_sockets.erase(generic_open_sockets.begin() + pos);
        } else {
            throw Error(__file__, __LINE__, "Socket index out of range!");
        }
        if (pos < n_msg_sockets) { // we removed a thread socket, therefore we have to decrement all position ids
            n_msg_sockets--;
            stop_sock_id--;
            http_sock_id--;
            ssl_sock_id--;
            dyn_socket_base--;
        } else if (pos == http_sock_id) {
            http_sock_id = -1;
            ssl_sock_id--;
            dyn_socket_base--;
        } else if (pos == ssl_sock_id) {
            ssl_sock_id = -1;
            dyn_socket_base--;
        }
    }

    void SocketControl::move_to_waiting(int pos) { // called multiple times, changes open sockets vector!!!
        std::unique_lock<std::mutex> mutex_guard(sockets_mutex);
        if (((pos - dyn_socket_base) >= 0) && (pos < generic_open_sockets.size())) {
            SocketControl::SocketInfo sockid = generic_open_sockets[pos];
            generic_open_sockets.erase(generic_open_sockets.begin() + pos);
            waiting_sockets.push(sockid);
        } else {
            throw Error(__file__, __LINE__, "Socket index out of range!");
        }
    }
    //=========================================================================

    bool SocketControl::get_waiting(SocketInfo &sockid) {
        std::unique_lock<std::mutex> mutex_guard(sockets_mutex);
        if (waiting_sockets.size() > 0) {
            sockid = waiting_sockets.front();
            waiting_sockets.pop();
            return true;
        } else {
            return false;
        }
    }
    //=========================================================================§

    int SocketControl::send_control_message(int pipe_id, const SocketInfo &msg) {
        return ::send(pipe_id, &msg, sizeof(SocketInfo), 0);
    }
    //=========================================================================§

    SocketControl::SocketInfo SocketControl::receive_control_message(int pipe_id) {
        SocketInfo msg;
        int n;
        if ((n = ::read(pipe_id, &msg, sizeof(SocketInfo))) != sizeof(SocketInfo)) {
            msg.type = ERROR;
            std::cerr << "==> receive_control_message: received only " << n << " bytes!!" << std::endl;
        }
        return msg;
    }
    //=========================================================================§

    void SocketControl::broadcast_exit() {
        SocketInfo msg;
        msg.type = EXIT;
        for (int i = 0; i < n_msg_sockets; i++) {
            ::send(generic_open_sockets[i].sid, &msg, sizeof(SocketInfo), 0);
        }
    }

    void SocketControl::close_all_dynsocks(int (*closefunc)(const SocketInfo&)) {
        std::unique_lock<std::mutex> mutex_guard(sockets_mutex);
        for (int i = dyn_socket_base; i < generic_open_sockets.size(); i++) {
            (void) closefunc(generic_open_sockets[i]);
            generic_open_sockets.erase(generic_open_sockets.begin() + i);
        }
    }
}