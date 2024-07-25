/*
 * ParallelThread.h
 *
 * SPDX-License-Identifier:  BSD-3-Clause
 *
 * Copyright (C) 2024 brummer <brummer@web.de>
 */

#ifdef _WIN32
#define MINGW_STDTHREAD_REDUNDANCY_WARNING
#endif

#include <atomic>
#include <cstdint>
#include <unistd.h>
#include <mutex>
#include <thread>
#include <cstring>
#include <chrono>
#include <condition_variable>

#pragma once

#ifndef PARALLEL_THREAD_H_
#define PARALLEL_THREAD_H_


/****************************************************************
 ** ParallelThread - class to run processes in a separate thread
 * usage:
 *      // create a instance of ParallelThread
 *      ParallelThread proc;
 * 
 *      // start the thread
 *      proc.start();
 * 
 *      // optional: set the time out for the waiting function,
 *      // this is a periodic wake up to check if the data been processed
 *      proc.setTimeOut(microseconds);
 * 
 *      // optional: set the policy class and the priority the thread should use
 *      proc.set_priority(priority, policy):
 * 
 *      // set a (YourClass member) function to be run by the thread
 *      proc.function = [=] () {yourFunction();};
 * 
 *      //optional: prepare the waiting function to wait
 *      proc.setWait();
 * 
 *      // Inform the thread to run the function
 *      proc.cv.notify_one();
 * 
 *      // optional: wait for the processed data
 *      proc.processWait();
 * 
 *      // stop the thread when no longer needed
 *      // at least before exit the program
 *      proc.stop();
 */

class ParallelThread
{
private:
    std::atomic<bool> _execute;
    std::thread _thd;
    std::mutex m;
    std::mutex mo;
    std::condition_variable co;
    std::atomic<bool> pWait;
    std::chrono::microseconds timeoutPeriod;

    void run() {
        if( _execute.load(std::memory_order_acquire) ) {
            stop();
        };
        _execute.store(true, std::memory_order_release);
        _thd = std::thread([this]() {
            while (_execute.load(std::memory_order_acquire)) {
                std::unique_lock<std::mutex> lk(m);
                // wait for signal from dsp that work is to do
                cv.wait(lk);
                //do work
                if (_execute.load(std::memory_order_acquire)) {
                    pWait.store(true, std::memory_order_release);
                    process();
                    pWait.store(false, std::memory_order_release);
                    co.notify_one();
                }
            }
            // when done
        });    
    }

public:
    std::function<void ()>process;
    std::condition_variable cv;

    ParallelThread()
        : _execute(false) {
        timeoutPeriod = std::chrono::microseconds(200);
        pWait.store(false, std::memory_order_release);
    }

    ~ParallelThread() {
        if( _execute.load(std::memory_order_acquire) ) {
            stop();
        };
    }

    void setTimeOut(int timeout) {
        timeoutPeriod = std::chrono::microseconds(timeout);
    }

    void set_priority(int32_t rt_prio, int32_t rt_policy) {
#if defined(__linux__) || defined(_UNIX) || defined(__APPLE__)
        sched_param sch_params;
        if (rt_prio == 0) {
            rt_prio = sched_get_priority_max(rt_policy);
        }
        if ((rt_prio/5) > 0) rt_prio = rt_prio/5;
        sch_params.sched_priority = rt_prio;
        if (pthread_setschedparam(_thd.native_handle(), rt_policy, &sch_params)) {
            fprintf(stderr, "ParallelThread: fail to set priority\n");
        }
#elif defined(_WIN32)
        // HIGH_PRIORITY_CLASS, THREAD_PRIORITY_TIME_CRITICAL
        if (SetThreadPriority(_thd.native_handle(), 15)) {
            fprintf(stderr, "ParallelThread: fail to set priority\n");
        }
#else
        //system does not supports thread priority!
#endif
    }

    void setWait() {
        pWait.store(true, std::memory_order_release);
    }

    void processWait() {
        if (is_running() && pWait.load(std::memory_order_acquire)) {
            std::unique_lock<std::mutex> lk(mo);
            while (pWait.load(std::memory_order_acquire))
                co.wait_for(lk, timeoutPeriod);
        }   
    }

    void stop() {
        if (is_running()) {
            _execute.store(false, std::memory_order_release);
            if (_thd.joinable()) {
                cv.notify_one();
                _thd.join();
            }
        }
    }

    void start() {
        if (!is_running()) run();
    }

    bool is_running() const noexcept {
        return ( _execute.load(std::memory_order_acquire) && 
                 _thd.joinable() );
    }

};

#endif
