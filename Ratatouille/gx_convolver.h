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

bool read_audio(const std::string& filename, unsigned int *audio_size, int *audio_chan,
		int *audio_type, int *audio_form, int *audio_rate, float **buffer);

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
    GxConvolver(gx_resample::StreamingResampler resamp)
      : GxConvolverBase(), resamp() {}
    bool configure(
        std::string fname, float gain, float lgain,
        unsigned int delay, unsigned int ldelay, unsigned int offset,
        unsigned int length, unsigned int size, unsigned int bufsize);
    bool compute(int count, float* input1, float *input2, float *output1, float *output2);
    bool configure(std::string fname, float gain, unsigned int delay, unsigned int offset,
		   unsigned int length, unsigned int size, unsigned int bufsize);
    bool compute(int count, float* input, float *output);
};

class GxSimpleConvolver: public GxConvolverBase
{
private:
  gx_resample::BufferResampler& resamp;
public:
  int32_t pre_count;
  uint32_t pre_sr;
  float *pre_data;
  float *pre_data_new;
  GxSimpleConvolver(gx_resample::BufferResampler& resamp_)
    : GxConvolverBase(), resamp(resamp_), pre_count(0), pre_sr(0),
    pre_data(NULL), pre_data_new(NULL) {}
  bool configure(int32_t count, float *impresp, uint32_t imprate);
  bool update(int32_t count, float *impresp, uint32_t imprate);
  bool compute(int32_t count, float* input, float *output);
  bool compute(int32_t count, float* buffer)
  {
    return is_runnable() ? compute(count, buffer, buffer) : true;
  }
  
  bool configure_stereo(int32_t count, float *impresp, uint32_t imprate);
  bool update_stereo(int32_t count, float *impresp, uint32_t imprate);
  bool compute_stereo(int32_t count, float* input, float* input1, float *output, float *output1);
  bool compute_stereo(int32_t count, float* buffer, float* buffer1)
  {
    return is_runnable() ? compute_stereo(count, buffer, buffer1, buffer, buffer1) : true;
  }
  static void run_static(uint32_t n_samples, GxSimpleConvolver *p, float *output);
  static void run_static(uint32_t n_samples, GxSimpleConvolver *p, float *input, float *output);
  static void run_static_stereo(uint32_t n_samples, GxSimpleConvolver *p, float *output, float *output1);
};


#endif  // SRC_HEADERS_GX_CONVOLVER_H_
