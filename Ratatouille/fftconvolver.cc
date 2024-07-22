/*
 * fftconvolver.cc
 *
 * SPDX-License-Identifier:  BSD-3-Clause
 *
 * Copyright (C) 2024 brummer <brummer@web.de>
 */


#include "fftconvolver.h"
#include <string.h>


/****************************************************************
 ** Audiofile
 */

Audiofile::Audiofile(void) {
    reset();
}


Audiofile::~Audiofile(void) {
    close();
}


void Audiofile::reset(void) {
    _sndfile = 0;
    _type = TYPE_OTHER;
    _form = FORM_OTHER;
    _rate = 0;
    _chan = 0;
    _size = 0;
}


int Audiofile::open_read(std::string name) {
    SF_INFO I;

    reset();

    if ((_sndfile = sf_open(name.c_str(), SFM_READ, &I)) == 0) return ERR_OPEN;

    switch (I.format & SF_FORMAT_TYPEMASK) {
    case SF_FORMAT_CAF:
        _type = TYPE_CAF;
        break;
    case SF_FORMAT_WAV:
        _type = TYPE_WAV;
        break;
    case SF_FORMAT_AIFF:
        _type = TYPE_AIFF;
        break;
    case SF_FORMAT_WAVEX:
#ifdef     SFC_WAVEX_GET_AMBISONIC
        if (sf_command(_sndfile, SFC_WAVEX_GET_AMBISONIC, 0, 0) == SF_AMBISONIC_B_FORMAT)
            _type = TYPE_AMB;
        else
#endif
            _type = TYPE_WAV;
    }

    switch (I.format & SF_FORMAT_SUBMASK) {
    case SF_FORMAT_PCM_16:
        _form = FORM_16BIT;
        break;
    case SF_FORMAT_PCM_24:
        _form = FORM_24BIT;
        break;
    case SF_FORMAT_PCM_32:
        _form = FORM_32BIT;
        break;
    case SF_FORMAT_FLOAT:
        _form = FORM_FLOAT;
        break;
    }

    _rate = I.samplerate;
    _chan = I.channels;
    _size = I.frames;

    return 0;
}


int Audiofile::close(void) {
    if (_sndfile) sf_close(_sndfile);
    reset();
    return 0;
}


int Audiofile::seek(uint32_t posit) {
    if (!_sndfile) return ERR_MODE;
    if (sf_seek(_sndfile, posit, SEEK_SET) != posit) return ERR_SEEK;
    return 0;
}


int Audiofile::read(float *data, uint32_t frames) {
    return sf_readf_float(_sndfile, data, frames);
}

/****************************************************************
 ** DoubleThreadConvolver
 */

///////////////////////// INTERNAL WORKER CLASS   //////////////////////

ConvolverWorker::ConvolverWorker(DoubleThreadConvolver &xr)
    : _execute(false),
    _xr(xr) {
}

ConvolverWorker::~ConvolverWorker() {
    if( _execute.load(std::memory_order_acquire) ) {
        stop();
    };
}

void ConvolverWorker::set_priority() {
#if defined(__linux__) || defined(_UNIX) || defined(__APPLE__)
    sched_param sch_params;
    sch_params.sched_priority = 5;
    if (pthread_setschedparam(_thd.native_handle(), SCHED_FIFO, &sch_params)) {
        fprintf(stderr, "ConvolverWorker: fail to set priority\n");
    }
#elif defined(_WIN32)
    // HIGH_PRIORITY_CLASS, THREAD_PRIORITY_TIME_CRITICAL
    if (SetThreadPriority(_thd.native_handle(), 15)) {
        fprintf(stderr, "ConvolverWorker: fail to set priority\n");
    }
#else
    //system does not supports thread priority!
#endif
}

void ConvolverWorker::stop() {
    if (is_running()) {
        _execute.store(false, std::memory_order_release);
        if (_thd.joinable()) {
            cv.notify_one();
            _thd.join();
        }
    }
}

void ConvolverWorker::run() {
    if( _execute.load(std::memory_order_acquire) ) {
        stop();
    };
    _execute.store(true, std::memory_order_release);
    _thd = std::thread([this]() {
        set_priority();
        while (_execute.load(std::memory_order_acquire)) {
            std::unique_lock<std::mutex> lk(m);
            // wait for signal from dsp that work is to do
            cv.wait(lk);
            //do work
            if (_execute.load(std::memory_order_acquire)) {
                _xr.doBackgroundProcessing();
                _xr.co.notify_one();
                _xr.setWait.store(false, std::memory_order_release);
            }
        }
        // when done
    });    
}

void ConvolverWorker::start() {
    if (!is_running()) run();
}

bool ConvolverWorker::is_running() const noexcept {
    return ( _execute.load(std::memory_order_acquire) && 
             _thd.joinable() );
}

void DoubleThreadConvolver::startBackgroundProcessing()
{
    if (work.is_running()) {
        setWait.store(true, std::memory_order_release);
        work.cv.notify_one();
    } else {
        doBackgroundProcessing();
    }
}


void DoubleThreadConvolver::waitForBackgroundProcessing()
{
    if (work.is_running() && setWait.load(std::memory_order_acquire)) {
        std::unique_lock<std::mutex> lk(mo);
        while (setWait.load(std::memory_order_acquire))
            co.wait_for(lk, timeoutPeriod);
    }
}

bool DoubleThreadConvolver::get_buffer(std::string fname, float **buffer, uint32_t *rate, int *asize)
{
    Audiofile audio;
    if (audio.open_read(fname)) {
        fprintf(stderr, "Unable to open %s\n", fname.c_str() );
        *buffer = 0;
        return false;
    }
    *rate = audio.rate();
    *asize = audio.size();
    const int limit = 2000000; // arbitrary size limit
    if (*asize > limit) {
        fprintf(stderr, "too many samples (%i), truncated to %i\n"
                           , audio.size(), limit);
        *asize = limit;
    }
    if (*asize * audio.chan() == 0) {
        fprintf(stderr, "No samples found\n");
        *buffer = 0;
        audio.close();
        return false;
    }
    float* cbuffer = new float[*asize * audio.chan()];
    if (audio.read(cbuffer, *asize) != static_cast<int>(*asize)) {
        delete[] cbuffer;
        fprintf(stderr, "Error reading file\n");
        *buffer = 0;
        audio.close();
        return false;
    }
    if (audio.chan() > 1) {
        //fprintf(stderr,"only taking first channel of %i channels in impulse response\n", audio.chan());
        float *abuffer = new float[*asize];
        for (int i = 0; i < *asize; i++) {
            abuffer[i] = cbuffer[i * audio.chan()];
        }
        delete[] cbuffer;
        cbuffer = NULL;
        cbuffer = abuffer;
    }
    *buffer = cbuffer;
    audio.close();
    if (*rate != samplerate) {
        *buffer = resamp.process(*rate, *asize, *buffer, samplerate, asize);
        if (!*buffer) {
            printf("no buffer\n");
            return false;
        }
        //fprintf(stderr, "FFTConvolver: resampled from %i to %i\n", *rate, samplerate);
    }
    return true;
}

void DoubleThreadConvolver::normalize(float* buffer, int asize) {
    if (!norm) return;
    double gain = 0.0;
    // get gain factor from file buffer
    for (int i = 0; i < asize; i++) {
        double v = buffer[i] ;
        gain += v*v;
    }
    // apply gain factor when needed
    if (gain != 0.0) {
        gain = 1.0 / gain;

        for (int i = 0; i < asize; i++) {
            buffer[i] *= gain;
        }
    }
}

void DoubleThreadConvolver::set_normalisation(uint32_t norm_) {
    norm = norm_;
}

bool DoubleThreadConvolver::configure(std::string fname, float gain, unsigned int delay, unsigned int offset,
            unsigned int length, unsigned int size, unsigned int bufsize)
{
    filename = fname;
    float* abuf = NULL;
    uint32_t arate = 0;
    int asize = 0;
    if (!get_buffer(fname, &abuf, &arate, &asize)) {
        return false;
    }
    normalize(abuf, asize);
    int timeout = std::max(100,static_cast<int>((buffersize/(samplerate*0.000001))*0.1));
    timeoutPeriod = std::chrono::microseconds(timeout);
    uint32_t _head = 1;
    while (_head < buffersize) {
        _head *= 2;
    }
    uint32_t _tail = _head > 8192 ? _head : 8192;
    //fprintf(stderr, "head %i tail %i irlen %i timeout %i microsec\n", _head, _tail, asize, timeout);
    if (init(_head, _tail, abuf, asize)) {
        ready = true;
        delete[] abuf;
        return true;
    }
    delete[] abuf;
    return false;
}

bool DoubleThreadConvolver::compute(int32_t count, float* input, float* output)
{
    process(input, output, count);
    return true;
}
