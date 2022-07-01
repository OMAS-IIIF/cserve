//
// Created by Lukas Rosenthaler on 08.07.20.
//
#include <functional>

#include "ThreadControl.h"
#include "Cserve.h"

static const char file_[] = __FILE__;

namespace cserve {


    ThreadControl::ThreadControl(unsigned n_threads, const ThreadFunction& start_routine, Server *serv) {
        //
        // first we have to create the vector for the child data. We misuse the "result"-field
        // to store the master's socket endpoint.
        //
        for (int n = 0; n < n_threads; n++) {
            int control_pipe[2];
            if (socketpair(PF_LOCAL, SOCK_STREAM, 0, control_pipe) != 0) {
                Server::logger()->error("Creating pipe failed at [{}: {}]: {}", file_, __LINE__, strerror(errno));
                return;
            }
            child_data.push_back({control_pipe[1], control_pipe[0], serv});
        }
        //
        // now we can create the threads
        //
        for (int n = 0; n < n_threads; n++) {
            ThreadMasterData thread_data;
            thread_data.control_pipe = child_data[n].result;
            thread_data.thread_ptr = std::make_shared<std::thread>(start_routine, std::ref(child_data[n]));
            thread_list.push_back(thread_data);
            thread_push(thread_data);
        }
    }
    //=========================================================================

    ThreadControl::~ThreadControl() {
        for (auto const &thread_data : thread_list) {
            thread_data.thread_ptr->join();
        }
    }
    //=========================================================================

    void ThreadControl::thread_push(const ThreadMasterData &tinfo) {
        std::unique_lock<std::mutex> thread_queue_guard(thread_queue_mutex);
        thread_queue.push(tinfo);
    }
    //=========================================================================

    bool ThreadControl::thread_pop(ThreadMasterData &tinfo) {
        std::unique_lock<std::mutex> thread_queue_guard(thread_queue_mutex);
        tinfo = {0, 0};
        if (!thread_queue.empty()) {
            tinfo = thread_queue.front();
            thread_queue.pop();
            return true;
        }
        return false;
    }
    //=========================================================================

    ThreadControl::ThreadMasterData &ThreadControl::operator[](int index) {
        if (index >= 0 && index < thread_list.size()) {
            return thread_list[index];
        } else {
            throw ThreadControlError("INVALID_INDEX");
        }
    }
    //=========================================================================

    int ThreadControl::thread_delete(int pos) {
        thread_list.erase(thread_list.begin() + pos);
        return static_cast<int>(thread_list.size());
    }
    //=========================================================================

}
