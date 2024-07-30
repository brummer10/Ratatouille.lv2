/*
 * ParallelThread.h
 *
 * SPDX-License-Identifier:  BSD-3-Clause
 *
 * Copyright (C) 2024 brummer <brummer@web.de>
 */

/****************************************************************
 ** ParallelThread - class to run a processes in a parallel thread
 *                   requires minimum c++17
 *                   works best with c++20 (std::atomic::wait)
 *
 *  ParallelThread using delegate to replace std::function<void> 
 *  see:
 *           https://github.com/rosbacke/delegate
 *
 *  ParallelThread aims to be suitable in real-time processes
 *  to provide a parallel processor.
 *
 *  But ParallelThread could be used in trivial environments,
 *  as worker thread, as well.
 *
 *  usage:
 *      //Create a instance for ParallelThread
 *      ParallelThread proc;
 *      // start the thread
 *      proc.start();
 *      // optional set the scheduling class and the priority (as int32_t)
 *      proc.setPriority(priority, scheduling_class)
 *      // optional set the timeout value for the waiting functions
 *         in microseconds. Default is 400 micro seconds.
 *         This is a safety guard to avoid dead looks.
 *         When overrun this time, the process will break and data may be lost.
 *         A reasonable value for usage in real-time could be calculated by
 *      proc.setTimeOut(std::max(100,static_cast<int>((bufferSize/(sampleRate*0.000001))*0.1)));
 *      // optional set a name for the thread, default name is "anonymous"
 *         that may be helpful for diagnostics.
 *      proc.setThreadName("YourName");
 *      // set the function to run in the parallel thread
 *         function should be defined in YourClass as void YourFunction();
 *      proc.process.set<&YourClass::YourFunction>(*this);
 *      // now anything is setup to run the thread,
 *         so try to get the processing pointer by getProcess()
 *         getProcess() check if the thread is in waiting state, if not,
 *         it waits as max two times the time set by setTimeOut()
 *         so be prepared to run the function in the main process
 *         in case the parallel process is not ready to run.
 *         That is the worst case, and shouldn't happen under normal
 *         circumstances.
 *         if getProcess() return true, runProcess(), otherwise
 *         run the function in the main thread. 
 *      if (proc.getProcess()) proc.runProcess() else functionToRun();
 *      // optional at the point were processed data needs to be merged, 
 *         wait for the data. In case there is no data processed,
 *         or the data is already ready, processWait() returns directly
 *      proc.processWait();
 *         processWait() waits maximal 5 times the timeout. If the
 *         process isn't ready in that time, the data is lost and 
 *         processWait() break to avoid Xruns or dead looks. 
 *         That is the worst case and shouldn't happen 
 *         under normal circumstances.
 *      // Finally stop the thread before exit.
 *      proc.stop(); 
 */

#if defined(_WIN32)
#define MINGW_STDTHREAD_REDUNDANCY_WARNING
#endif

#include <atomic>
#include <cstdint>
#include <unistd.h>
#include <mutex>
#include <thread>
#include <cstring>
#if defined(_WIN32)
#include <chrono>
#else
#include <time.h>
#include <pthread.h>
#endif
#include <condition_variable>

#include "delegate.hpp"

#pragma once

#ifndef PARALLEL_THREAD_H_
#define PARALLEL_THREAD_H_

class ParallelThread
{
private:
    std::atomic<bool> pRun;
    std::atomic<bool> pWait;
    std::atomic<bool> isWaiting;

    #if __cplusplus > 201703L
    std::atomic<bool> pWorkCond;
    #else
    std::mutex pWaitWork;
    std::condition_variable pWorkCond;
    #endif

    std::thread pThd;
    std::string threadName;
    uint32_t timeoutPeriod;

    #if defined(_WIN32)
    std::mutex pWaitProc;
    std::condition_variable pProcCond;
    #else
    pthread_mutex_t pWaitProc;
    pthread_cond_t pProcCond;
    struct timespec timeOut;
    #endif

    inline void run() noexcept {
        if( pRun.load(std::memory_order_acquire) ) {
            stop();
        };
        pRun.store(true, std::memory_order_release);
        pThd = std::thread([this]() {
            #if __cplusplus <= 201703L
            std::unique_lock<std::mutex> lk(pWaitWork);
            #endif
            while (pRun.load(std::memory_order_acquire)) {
                isWaiting.store(true, std::memory_order_release);
                notifyParent();
                // wait for signal from dsp that work is to do
                #if __cplusplus > 201703L
                pWorkCond.wait(false);
                pWorkCond.store(false);
                #else
                pWorkCond.wait(lk);
                #endif
                isWaiting.store(false, std::memory_order_release);
                pWait.store(true, std::memory_order_release);
                process();
                pWait.store(false, std::memory_order_release);
            }
            // when done
        });    
    }

    inline void notifyParent() noexcept {
        #if defined(_WIN32)
        pProcCond.notify_all();
        #else
        pthread_cond_broadcast(&pProcCond);
        #endif
    }

    inline void init() noexcept {
        #if defined(__linux__) || defined(_UNIX) || defined(__APPLE__)
        pthread_condattr_t cond_attr;
        pthread_condattr_init(&cond_attr);
        pthread_condattr_setclock(&cond_attr, CLOCK_MONOTONIC);
        pthread_cond_init(&pProcCond, &cond_attr);
        pthread_condattr_destroy(&cond_attr);
        pWaitProc = PTHREAD_MUTEX_INITIALIZER;
        #endif
    }

    #if defined(__linux__) || defined(_UNIX) || defined(__APPLE__)
    inline struct timespec *getTimeOut() noexcept {
        clock_gettime (CLOCK_MONOTONIC, &timeOut);
        long int at = (timeoutPeriod * 1000);
        if (timeOut.tv_nsec + at > 1000000000) {
            timeOut.tv_sec +=1;
            at -= 1000000000;
        }
        timeOut.tv_nsec += at;
        return &timeOut;
    }
    #elif defined(_WIN32)
    inline std::chrono::microseconds getTimeOut() {
        return std::chrono::microseconds(timeoutPeriod);
    }
    #endif

    void runDummyFunction() {}

public:
    delegate<void ()>process;

    ParallelThread()
        : pRun(false)
         ,pWait(false)
         ,isWaiting(false)
         #if __cplusplus > 201703L
         ,pWorkCond(false)
         #endif
    {
        timeoutPeriod = 400;
        process.set<&ParallelThread::runDummyFunction>(*this);
        threadName = "anonymous";
        init();
    }

    ~ParallelThread() {
        if( pRun.load(std::memory_order_acquire) ) {
            stop();
        };
    }

    inline void runProcess() noexcept {
        #if __cplusplus > 201703L
        pWorkCond.store(true);
        #endif
        pWorkCond.notify_one();
    }

    void setThreadName(std::string name) noexcept {
        threadName = name;
    }

    void setTimeOut(uint32_t timeout) noexcept {
        timeoutPeriod = timeout;
    }

    void setPriority(int32_t rt_prio, int32_t rt_policy) noexcept {
        #if defined(__linux__) || defined(_UNIX) || defined(__APPLE__)
        sched_param sch_params;
        if (rt_prio == 0) {
            rt_prio = sched_get_priority_max(rt_policy);
        }
        if ((rt_prio/5) > 0) rt_prio = rt_prio/5;
        sch_params.sched_priority = rt_prio;
        if (pthread_setschedparam(pThd.native_handle(), rt_policy, &sch_params)) {
            fprintf(stderr, "ParallelThread:%s fail to set priority\n", threadName.c_str());
        }
        #elif defined(_WIN32)
        // HIGH_PRIORITY_CLASS, THREAD_PRIORITY_TIME_CRITICAL
        if (SetThreadPriority(pThd.native_handle(), 15)) {
            fprintf(stderr, "ParallelThread:%s fail to set priority\n", threadName.c_str());
        }
        #else
        //system does not supports thread priority!
        #endif
    }

    inline bool getState() const noexcept {
        return isWaiting.load(std::memory_order_acquire);
    }

    inline bool getProcess() noexcept {
        if (isRunning() && !getState()) {
            int maxDuration = 0;
            #if defined(_WIN32)
            std::unique_lock<std::mutex> lk(pWaitProc);
            #endif
            while (!getState()) {
                #if defined(_WIN32)
                if (pProcCond.wait_for(lk, getTimeOut()) == std::cv_status::timeout) {
                #else
                pthread_mutex_lock(&pWaitProc);
                if (pthread_cond_timedwait(&pProcCond, &pWaitProc, getTimeOut()) == ETIMEDOUT) {
                    pthread_mutex_unlock(&pWaitProc);
                #endif
                    maxDuration +=1;
                    //fprintf(stderr, "%s wait for process %i\n", threadName.c_str(), maxDuration);
                    if (maxDuration > 2) {
                        //fprintf(stderr, "%s break waitForProcess\n", threadName.c_str());
                        break;
                    }
                } else {
                    #if defined(__linux__) || defined(_UNIX) || defined(__APPLE__)
                    pthread_mutex_unlock(&pWaitProc);
                    #endif
                }
            }
        }
        if (getState()) pWait.store(true, std::memory_order_release);
        return getState();
    }

    inline void processWait() noexcept {
        if (isRunning()) {
            int maxDuration = 0;
            #if defined(_WIN32)
            std::unique_lock<std::mutex> lk(pWaitProc);
            #endif
            while (pWait.load(std::memory_order_acquire)) {
                #if defined(_WIN32)
                if (pProcCond.wait_for(lk, getTimeOut()) == std::cv_status::timeout) {
                #else
                pthread_mutex_lock(&pWaitProc);
                if (pthread_cond_timedwait(&pProcCond, &pWaitProc, getTimeOut()) == ETIMEDOUT) {
                    pthread_mutex_unlock(&pWaitProc);
                #endif
                    maxDuration +=1;
                    //fprintf(stderr, "%s wait for data %i\n", threadName.c_str(), maxDuration);
                    if (maxDuration > 5) {
                        pWait.store(false, std::memory_order_release);
                        //fprintf(stderr, "%s break processWait\n", threadName.c_str());
                    }
                } else {
                    #if defined(__linux__) || defined(_UNIX) || defined(__APPLE__)
                    pthread_mutex_unlock(&pWaitProc);
                    #endif
                }
            }
            //fprintf(stderr, "%s processed data %i\n", threadName.c_str(), maxDuration);
        }
    }

    void stop() noexcept {
        if (isRunning()) {
            pRun.store(false, std::memory_order_release);
            if (pThd.joinable()) {
                #if __cplusplus > 201703L
                pWorkCond.store(true);
                #endif
                pWorkCond.notify_one();
                pThd.join();
            }
        }
    }

    void start() noexcept {
        if (!isRunning()) run();
    }

    inline bool isRunning() const noexcept {
        return (pRun.load(std::memory_order_acquire) && 
                 pThd.joinable());
    }

};

#endif
