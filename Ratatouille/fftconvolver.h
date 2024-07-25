/*
 * fftconvolver.h
 *
 * SPDX-License-Identifier:  BSD-3-Clause
 *
 * Copyright (C) 2024 brummer <brummer@web.de>
 */


#pragma once

#ifndef FFTCONVOLVER_H_
#define FFTCONVOLVER_H_

#include "TwoStageFFTConvolver.h"
#include <stdint.h>
#include <unistd.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <sndfile.hh>

#include "ParallelThread.h"
#include "gx_resampler.h"


class Audiofile {
public:

    enum {
        TYPE_OTHER,
        TYPE_CAF,
        TYPE_WAV,
        TYPE_AIFF,
        TYPE_AMB
    };

    enum {
        FORM_OTHER,
        FORM_16BIT,
        FORM_24BIT,
        FORM_32BIT,
        FORM_FLOAT
    };

    enum {
        ERR_NONE    = 0,
        ERR_MODE    = -1,
        ERR_TYPE    = -2,
        ERR_FORM    = -3,
        ERR_OPEN    = -4,
        ERR_SEEK    = -5,
        ERR_DATA    = -6,
        ERR_READ    = -7,
        ERR_WRITE   = -8
    };

    Audiofile(void);
    ~Audiofile(void);

    int type(void) const      { return _type; }
    int form(void) const      { return _form; }
    int rate(void) const      { return _rate; }
    int chan(void) const      { return _chan; }
    unsigned int size(void) const { return _size; }

    int open_read(std::string name);
    int close(void);

    int seek(unsigned int posit);
    int read(float *data, unsigned int frames);

private:

    void reset(void);

    SNDFILE     *_sndfile;
    int          _type;
    int          _form;
    int          _rate;
    int          _chan;
    unsigned int _size;
};


class DoubleThreadConvolver:  public fftconvolver::TwoStageFFTConvolver
{
public:
    std::mutex mo;
    std::condition_variable co;
    bool start(int32_t policy, int32_t priority) {
        if (!pro.is_running()) {
            pro.start(); 
            pro.set_priority(25, 1); //SCHED_FIFO
        }
        return ready;}

    void set_normalisation(uint32_t norm);

    bool configure(std::string fname, float gain, unsigned int delay, unsigned int offset,
                    unsigned int length, unsigned int size, unsigned int bufsize);

    bool compute(int32_t count, float* input, float *output);

    bool checkstate() { return true;}

    inline void set_not_runnable() { ready = false;}

    inline bool is_runnable() { return ready;}

    inline void set_buffersize(uint32_t sz) { buffersize = sz;}

    inline void set_samplerate(uint32_t sr) { samplerate = sr;}

    int stop_process() {
            ready = false;
            return 0;}

    int cleanup () {
            reset();
            return 0;}

    DoubleThreadConvolver()
        : resamp(), ready(false), samplerate(0), pro() {
            pro.setTimeOut(200);
            pro.process = [this] () {this->doBackgroundProcessing();};
            norm = 0;}

    ~DoubleThreadConvolver() { reset(); pro.stop();}

protected:
    virtual void startBackgroundProcessing();
    virtual void waitForBackgroundProcessing();

private:
    friend class ParallelThread;
    gx_resample::BufferResampler resamp;
    volatile bool ready;
    uint32_t buffersize;
    uint32_t samplerate;
    uint32_t norm;
    std::string filename;
    ParallelThread pro;
    std::atomic<bool> setWait;
    std::chrono::microseconds timeoutPeriod;
    bool get_buffer(std::string fname, float **buffer, uint32_t* rate, int* size);
    void normalize(float* buffer, int asize);
};

#endif  // FFTCONVOLVER_H_
