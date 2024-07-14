/*
 * Copyright (C) 2012 Hermann Meyer, Andreas Degert, Pete Shorthose, Steve Poskitt
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 * --------------------------------------------------------------------------
 */


#pragma once

#ifndef SRC_HEADERS_GX_CONVOLVER_H_
#define SRC_HEADERS_GX_CONVOLVER_H_

#include "zita-convolver.h"
#include "TwoStageFFTConvolver.h"
#include <stdint.h>
#include <unistd.h>
#include "gx_resampler.h"

#include <sndfile.hh>

/* GxConvolver */

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

class GxConvolverBase: protected Convproc
{
protected:
  volatile bool ready;
  bool sync;
  void adjust_values(uint32_t audio_size, uint32_t& count, uint32_t& offset,
                     uint32_t& delay, uint32_t& ldelay, uint32_t& length,
                     uint32_t& size, uint32_t& bufsize);
  uint32_t buffersize;
  uint32_t samplerate;
  GxConvolverBase(): ready(false), sync(false), buffersize(), samplerate() {}
  ~GxConvolverBase();
public:
  inline void set_buffersize(uint32_t sz)
  {
    buffersize = sz;
  }
  inline uint32_t get_buffersize()
  {
    return buffersize;
  }
  inline void set_samplerate(uint32_t sr)
  {
    samplerate = sr;
  }
  inline uint32_t get_samplerate()
  {
    return samplerate;
  }
  bool checkstate();
  using Convproc::state;
  inline void set_not_runnable()
  {
    ready = false;
  }
  inline bool is_runnable()
  {
    return ready;
  }
  bool start(int32_t policy, int32_t priority);
  using Convproc::stop_process;
  using Convproc::cleanup;
  inline void set_sync(bool val)
  {
    sync = val;
  }
};


class GxConvolver: public GxConvolverBase {
private:
    gx_resample::StreamingResampler resamp;
    bool read_sndfile(Audiofile& audio, int nchan, int samplerate, const float *gain,
                    unsigned int *delay, unsigned int offset, unsigned int length);
public:
    GxConvolver()
      : GxConvolverBase(), resamp() {}
    bool configure(std::string fname, float gain, unsigned int delay, unsigned int offset,
                    unsigned int length, unsigned int size, unsigned int bufsize);
    bool compute(int count, float* input, float *output);
};


class DoubleThreadConvolver;

class ConvolverWorker {
private:
    std::atomic<bool> _execute;
    std::thread _thd;
    std::mutex m;
    void set_priority();
    void run();
    DoubleThreadConvolver &_xr;

public:
    ConvolverWorker(DoubleThreadConvolver &xr);
    ~ConvolverWorker();
    void stop();
    void start();
    bool is_running() const noexcept;
    std::condition_variable cv;
};

class DoubleThreadConvolver: public fftconvolver::TwoStageFFTConvolver
{
public:
    std::mutex mo;
    std::condition_variable co;
    bool start(int32_t policy, int32_t priority) { 
        work.start(); 
        return ready;}

    bool configure(std::string fname, float gain, unsigned int delay, unsigned int offset,
                    unsigned int length, unsigned int size, unsigned int bufsize);

    bool compute(int32_t count, float* input, float *output);

    bool checkstate() { return true;}

    inline void set_not_runnable() { ready = false;}

    inline bool is_runnable() { return ready;}

    inline void set_buffersize(uint32_t sz) { buffersize = sz;}

    inline void set_samplerate(uint32_t sr) { samplerate = sr;}

    int stop_process() {
            reset();
            ready = false;
            return 0;}

    int cleanup () {
            reset();
            return 0;}

    DoubleThreadConvolver()
        : resamp(), ready(false), samplerate(0), work(*this) {
            timeoutPeriod = std::chrono::microseconds(2000);}

    ~DoubleThreadConvolver() { reset(); work.stop();}

protected:
    virtual void startBackgroundProcessing();
    virtual void waitForBackgroundProcessing();

private:
    friend class ConvolverWorker;
    gx_resample::BufferResampler resamp;
    bool ready;
    uint32_t buffersize;
    uint32_t samplerate;
    ConvolverWorker work;
    std::chrono::time_point<std::chrono::steady_clock> timePoint;
    std::chrono::microseconds timeoutPeriod;
    bool get_buffer(std::string fname, float **buffer, int* rate, int* size);
};


class SelectConvolver
{
public:
    void set_convolver(bool IsPowerOfTwo_) {
            IsPowerOfTwo = IsPowerOfTwo_;}

    bool start(int32_t policy, int32_t priority) {
            return IsPowerOfTwo ?
            conv.start(policy,priority) : 
            sconv.start(policy,priority);}

    bool configure(std::string fname, float gain, unsigned int delay, unsigned int offset,
                            unsigned int length, unsigned int size, unsigned int bufsize) { 
            return IsPowerOfTwo ?
            conv.configure(fname, gain, delay, offset, length, size, bufsize) :
            sconv.configure(fname, gain, delay, offset, length, size, bufsize);}

    bool compute(int32_t count, float* input, float *output) {
            return IsPowerOfTwo ?
            conv.compute(count, input, output) :
            sconv.compute(count, input, output);}

    bool checkstate() {
            return IsPowerOfTwo ?
            conv.checkstate() :
            sconv.checkstate();}

    inline void set_not_runnable() {
            return IsPowerOfTwo ? 
            conv.set_not_runnable() :
            sconv.set_not_runnable();}

    inline bool is_runnable() {
            return IsPowerOfTwo ?
            conv.is_runnable() :
            sconv.is_runnable();}

    inline void set_buffersize(uint32_t sz) {
            return IsPowerOfTwo ?
            conv.set_buffersize(sz) :
            sconv.set_buffersize(sz);}

    inline void set_samplerate(uint32_t sr) {
            return IsPowerOfTwo ?
            conv.set_samplerate(sr) :
            sconv.set_samplerate(sr);}

    int stop_process() {
            return IsPowerOfTwo ?
            conv.stop_process() :
            sconv.stop_process();}

    int cleanup() {
            return IsPowerOfTwo ?
            conv.cleanup() :
            sconv.cleanup();}

    SelectConvolver()
        : conv(), sconv(), IsPowerOfTwo(false) {}

    ~SelectConvolver() {}
private:
    bool IsPowerOfTwo;
    GxConvolver conv;
    DoubleThreadConvolver sconv;
};

#endif  // SRC_HEADERS_GX_CONVOLVER_H_
