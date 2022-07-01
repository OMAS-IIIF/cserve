//
// Created by Lukas Rosenthaler on 08.07.20.
//
#include "Global.h"
#include "SocketControl.h"
#include "spdlog/fmt/bundled/format.h"

static const char thisSourceFile[] = __FILE__;

namespace cserve {

    SocketControl::SocketControl(ThreadControl &thread_control) {
        for (int i = 0; i < thread_control.nthreads(); i++) {
            generic_open_sockets.emplace_back(NOOP, CONTROL_SOCKET, thread_control[i].control_pipe);
        }
        n_msg_sockets = thread_control.nthreads();
        stop_sock_id = -1;
        http_sock_id = -1;
        ssl_sock_id = -1;
        dyn_socket_base = -1;
    }
    //=========================================================================

    pollfd *SocketControl::get_sockets_arr() {
        open_sockets.clear();
         for (auto const &tmp: generic_open_sockets) {
             open_sockets.push_back({tmp.sid, POLLIN, 0});
        }
        return open_sockets.data();
    }

    void SocketControl::add_stop_socket(int sid) { // only called once
        std::unique_lock<std::mutex> mutex_guard(sockets_mutex);
        if (dyn_socket_base != -1) {
            throw Error(thisSourceFile, __LINE__, "Adding stop socket not allowed after adding dynamic sockets!");
        }
        generic_open_sockets.emplace_back(NOOP, STOP_SOCKET, sid);
        stop_sock_id = static_cast<int>(generic_open_sockets.size() - 1);
    }
    //=========================================================================

    void SocketControl::add_http_socket(int sid) { // only called once
        std::unique_lock<std::mutex> mutex_guard(sockets_mutex);
        if (dyn_socket_base != -1) {
            throw Error(thisSourceFile, __LINE__, "Adding HTTP socket not allowed after adding dynamic sockets!");
        }
        generic_open_sockets.emplace_back(NOOP, HTTP_SOCKET, sid);
        http_sock_id = static_cast<int>(generic_open_sockets.size() - 1);
    }
    //=========================================================================

    void SocketControl::add_ssl_socket(int sid) { // only called once
        std::unique_lock<std::mutex> mutex_guard(sockets_mutex);
        if (dyn_socket_base != -1) {
            throw Error(thisSourceFile, __LINE__, "Adding SSL socket not allowed after adding dynamic sockets!");
        }
        generic_open_sockets.emplace_back(NOOP, SSL_SOCKET, sid, nullptr);
        ssl_sock_id = static_cast<int>(generic_open_sockets.size() - 1);
    }
    //=========================================================================

    void SocketControl::add_dyn_socket(SocketInfo sockid) { // called multiple times, changes open_sockets vector!!!
        std::unique_lock<std::mutex> mutex_guard(sockets_mutex);
        sockid.type = NOOP;
        sockid.socket_type = DYN_SOCKET;
        generic_open_sockets.push_back(sockid);
        if (dyn_socket_base == -1) {
            dyn_socket_base = static_cast<int>(generic_open_sockets.size() - 1);
        }
    }
    //=========================================================================

    SocketControl::SocketInfo SocketControl::remove(int pos) { // called multiple times, changes open sockets vector!!!
        std::unique_lock<std::mutex> mutex_guard(sockets_mutex);
        SocketControl::SocketInfo sockid;
        if ((pos >= 0) && (pos < generic_open_sockets.size())) {
            sockid = generic_open_sockets[pos];
            generic_open_sockets.erase(generic_open_sockets.begin() + pos);
        } else {
            throw Error(thisSourceFile, __LINE__, "Socket index out of range!");
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
        return sockid;
    }

    void SocketControl::move_to_waiting(int pos) { // called multiple times, changes open sockets vector!!!
        std::unique_lock<std::mutex> mutex_guard(sockets_mutex);
        if (((pos - dyn_socket_base) >= 0) && (pos < generic_open_sockets.size())) {
            SocketControl::SocketInfo sockid = generic_open_sockets[pos];
            generic_open_sockets.erase(generic_open_sockets.begin() + pos);
            waiting_sockets.push(sockid);
        } else {
            throw Error(thisSourceFile, __LINE__, "Socket index out of range!");
        }
    }
    //=========================================================================

    std::optional<SocketControl::SocketInfo> SocketControl::get_waiting() {
        std::unique_lock<std::mutex> mutex_guard(sockets_mutex);
        if (!waiting_sockets.empty()) {
            SocketInfo sock_info = waiting_sockets.front();
            waiting_sockets.pop();
            return sock_info;
        } else {
            return {};
        }
    }
    //=========================================================================§

    ssize_t SocketControl::send_control_message(int pipe_id, const SocketInfo &msg) {
        SIData data = {msg.type, msg.socket_type, msg.sid, msg.ssl_sid, msg.sslctx, "", msg.peer_port};
        for (int i = 0; i < INET6_ADDRSTRLEN; ++i) {
            data.peer_ip[i] = msg.peer_ip[i];
        }
        return ::send(pipe_id, &data, sizeof(SIData), 0);
    }
    //=========================================================================§

    SocketControl::SocketInfo SocketControl::receive_control_message(int pipe_id) {
        SIData data{};
        ssize_t n;
        if ((n = ::read(pipe_id, &data, sizeof(SIData))) != sizeof(SIData)) {
            data.type = ERROR;
            std::cerr << "==> receive_control_message: received only " << n << " bytes!!" << std::endl;
        }
        return SocketInfo(data);
    }
    //=========================================================================§

    void SocketControl::broadcast_exit(int (*closefunc)(const SocketInfo&)) {
        SIData data = {EXIT, CONTROL_SOCKET, -1, nullptr, nullptr, "", -1};
        for (auto &ss: working_sockets) {
            (void) closefunc(ss);
        }
        for (char &c: data.peer_ip) { c = '\0'; }
        for (int i = 0; i < n_msg_sockets; i++) {
            data.sid = generic_open_sockets[i].sid;
            ::send(generic_open_sockets[i].sid, &data, sizeof(SIData), 0);
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