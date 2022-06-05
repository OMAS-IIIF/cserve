//
// Created by Lukas Rosenthaler on 08.07.20.
//

#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h> //for threading , link with lpthread
//#include <thread>
#include <utility>
#include <vector>
#include <queue>
#include <mutex>
#include <unistd.h>
#include <poll.h>

#include <atomic>

#ifndef CSERVE_THREADCONTROL_H
#define CSERVE_THREADCONTROL_H

namespace cserve {

    class Server; // Declaration only

    /*!
     * Error thrown by methods of ThreadControl
     */
    class ThreadControlError : public std::exception {
    public:

        /*!
         * Constructor of ThreadControlError
         * @param msg_p Error message as character array
         */
        [[maybe_unused]] inline explicit ThreadControlError(const char *msg_p) { msg = msg_p; };

        /*!
         * Constructor of ThreadControlError
         * @param msg_p Error message as std::string
         */
        [[maybe_unused]] inline explicit ThreadControlError(std::string msg_p) : msg(std::move(msg_p)) {};

        /*!
         * Copy constructor
         * @param err Another ThreadControlError
         */
        inline ThreadControlError(const ThreadControlError &err) { msg = err.msg; };

        /*!
         * Returns the error message
         * @return
         */
        [[nodiscard]] inline const char* what() const noexcept override {
            return msg.c_str();
        }

    private:
        std::string msg;
    };

    /*!
     * Class that controls the worker threads
     */
    class ThreadControl {
    public:
        typedef struct {
            pthread_t tid;
            int control_pipe;
        } ThreadMasterData;

        typedef struct {
            int control_pipe;
            int result;
            Server *serv;
        } ThreadChildData;

    private:
        std::vector<ThreadMasterData> thread_list; //!> List of all threads
        std::vector<ThreadChildData> child_data; //!> Data given to the thread
        std::queue<ThreadMasterData> thread_queue; //!> Queue of available threads for processing
        std::mutex thread_queue_mutex;

    public:
        /*!
         * Constructor which creates the given number of worker threads
         * @param n_threads Number of threads to create
         * @param start_routine Function that the threads should run
         * @param serv Reference to the server
         */
        ThreadControl(unsigned n_threads, void *(*start_routine)(void*), Server *serv);

        /*!
         * Destructor
         */
        ~ThreadControl();

        /*!
         * Push the given thread to the queue of threads waiting for a task
         * @param tinfo Inof about the thread to be pushed to the queue of waiting tasks
         */
        void thread_push(const ThreadMasterData &tinfo);

        /*!
         * Pop a thread from the list of waiting threads and return it in tinfo
         * @param tinfo Thread info of popped thread
         * @return TRUE, if thread was available, FALSE otherwise
         */
        bool thread_pop(ThreadMasterData &tinfo);

        /*!
         * Delete a thread from the list of all threads (may be because it exited...)
         * @param pos Position of hread in the list
         * @return The new number of threads available
         */
        int thread_delete(int pos);

        /*!
         * Access a thread fron the list using the []-operator
         * @param index
         * @return Thread info
         */
        ThreadMasterData &operator[](int index);

        /*!
         * Return the number of threads available
         * @return
         */
        [[nodiscard]] inline int nthreads() const { return static_cast<int>(thread_list.size()); }

    };

}

#endif //CSERVE_THREADCONTROL_H
