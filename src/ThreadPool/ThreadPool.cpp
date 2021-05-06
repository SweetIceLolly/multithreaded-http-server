/*
File:   ThreadPool.cpp
Author: Hanson
Desc:   Implement functionalities of the thread pool
Note:   Huge thanks to https://nachtimwald.com/2019/04/12/thread-pool-in-c/
*/

#include "ThreadPool.hpp"

void worker(ThreadPool *pool);

job::job(std::function<void (void *)> func, void *args) {
    this->func = func;
    this->args = args;
}

void ThreadPool::init(size_t threadCount, size_t maxJobCount) {
    if (threadCount == 0) {
        return;
    }

    this->threadCount = threadCount;
    this->workingCount = 0;
    this->stop = false;
    this->maxJobCount = maxJobCount;
    
    for (size_t i = 0; i < threadCount; i++) {
        std::thread(worker, this).detach();
    }
}

void ThreadPool::shutdown(bool finishRemainingJobs) {
    this->workMutex.lock();
    while (!this->jobQueue.empty()) {
        if (finishRemainingJobs) {
            // Finish all jobs in the queue, one by one
            job currentJob = this->jobQueue.front();
            if (currentJob.func) {
                currentJob.func(currentJob.args);
            }
        }
        this->jobQueue.pop();
    }
    this->stop = true;
    this->newJobCond.notify_all();  // There isn't really a "new job", this just unblocks the workers to allow them to exit
    this->workMutex.unlock();
}

bool ThreadPool::addJob(job newJob) {
    this->workMutex.lock();

    // Check for maximum queue size. 0 means no limitation
    if (maxJobCount == 0 || this->jobQueue.size() < maxJobCount) {
        this->jobQueue.push(newJob);
        this->newJobCond.notify_all();
        this->workMutex.unlock();
        return true;
    }
    else {
        this->workMutex.unlock();
        return false;
    }
}

void ThreadPool::waitForAllJobsDone() {
    std::unique_lock<std::mutex> lock(this->workMutex);
    for (;;) {
        // Wait until no working threads and the queue is empty
        if (this->workingCount != 0 || !this->jobQueue.empty()) {
            this->noJobCond.wait(lock);
        }
        else {
            break;
        }
    }
    lock.unlock();
}

void worker(ThreadPool *pool) {
    for (;;) {
        std::unique_lock<std::mutex> lock(pool->workMutex);

        // Wait for a new job or the stop signal
        while (pool->jobQueue.empty() && !pool->stop) {
            pool->newJobCond.wait(lock);
        }
        
        // Stop signal caught, exit the thread
        if (pool->stop) {
            pool->threadCount--;
            pool->noJobCond.notify_one();
            lock.unlock();
            return;
        }

        // Obtain a job from the queue and start working!
        job currentJob = pool->jobQueue.front();
        pool->jobQueue.pop();
        pool->workingCount++;
        lock.unlock();

        if (currentJob.func) {
            currentJob.func(currentJob.args);
        }

        // Job finished
        lock.lock();
        pool->workingCount--;
        if (pool->jobQueue.empty() && !pool->stop && pool->workingCount == 0) {
            // Notify there's no job working
            pool->noJobCond.notify_one();
        }
        lock.unlock();
    }
}
