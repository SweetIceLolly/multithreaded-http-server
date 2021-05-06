/*
File:   ThreadPool.hpp
Author: Hanson
Desc:   Define types and function prototypes of the thread pool
Note:   Huge thanks to https://nachtimwald.com/2019/04/12/thread-pool-in-c/
*/

#include <thread>
#include <functional>
#include <queue>
#include <mutex>
#include <condition_variable>

class job {
public:
    job(std::function<void(void *)> func, void *args);
    std::function<void (void *)> func;
    void *args;
};

class ThreadPool {
public:
    std::mutex              workMutex;
    std::condition_variable newJobCond;         // Signals when there's a new job to be processed
    std::condition_variable noJobCond;          // Signals when all threads are not working
    std::queue<job>         jobQueue;
    bool                    stop;
    size_t                  workingCount;
    size_t                  threadCount;
    size_t                  maxJobCount;

    /*
    Function:   init
    Desc:       Initialize the thread pool
    Args:       threadCount: Optional. Specify the number of threads. Default is the number
    .                        of concurrent threads supported by the implementation
    .           maxJobCount: Optional. Specify the maximum number of jobs in the queue.
    .                        Default is 0, which means no limitation
    WARNING:    For each ThreadPool class, this function should be called once only!
    */
    void init(size_t threadCount = std::thread::hardware_concurrency(), size_t maxJobCount = 0);

    /*
    Function:   shutdown
    Desc:       Terminate the thread pool
    Args:       finishRemainingJobs: Optional. Determines whether the remaining jobs in the queue
    .                                will be finished before terminating or not
    */
    void shutdown(bool finishRemainingJobs = true);

    /*
    Function:   addJob
    Desc:       Add a new job into the queue
    Args:       newJob: A job element, specifies the handler function and arguments.
    .                   E.g. addJob(job(handler, args));
    Return:     If the job element is added to the queue, true is returned. If the
    .           element is not added because the queue is full, false is returned
    */
    bool addJob(job newJob);

    /*
    Function:   waitForAllJobsDone
    Desc:       Wait until the working queue is empty and no thread is working
    */
    void waitForAllJobsDone();
};
