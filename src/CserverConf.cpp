//
// Created by Lukas Rosenthaler on 28.06.21.
//

#include <thread>

#include "CLI11.hpp"

#include "CserverConf.h"

extern size_t data_volume(const std::string volstr) {
    size_t l = volstr.length();
    size_t data_volume = 0;
    char c = toupper(volstr[l - 1]);

    if (c == 'M') {
        data_volume = stoll(volstr.substr(0, l - 1)) * 1024 * 1024;
    } else if (c == 'G') {
        data_volume = stoll(volstr.substr(0, l - 1)) * 1024 * 1024 * 1024;
    } else if (c == 'T') {
        data_volume = stoll(volstr.substr(0, l - 1)) * 1024 * 1024 * 1024 * 1024;
    } else {
        data_volume = stoll(volstr);
    }
    return data_volume;
}

CserverConf::CserverConf(int argc, char *argv[]) {
    _userid = "";
    _port = 4711;
#ifdef CSERVE_ENABLE_SSL
    _ssl_port = 4712
#else
    _ssl_port = -1;
#endif
    _nthreads = std::thread::hardware_concurrency();
    _tmpdir = "/tmp"
}