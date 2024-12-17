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

void DoubleThreadConvolver::startBackgroundProcessing()
{
    if (pro.getProcess()) {
        pro.runProcess();
    } else {
        doBackgroundProcessing();
    }
}


void DoubleThreadConvolver::waitForBackgroundProcessing()
{
    pro.processWait();
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
    float gain = 0.0;
    // get gain factor from file buffer
    for (int i = 0; i < asize; i++) {
        current = std::max(gain, std::abs( buffer[i])) ;
        if current > gain {
            gain = current;
        }
    }
    // apply gain factor
    for (int i = 0; i < asize; i++) {
        buffer[i] /= gain;
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

    pro.setTimeOut(std::max(100,static_cast<int>((buffersize/(samplerate*0.000001))*0.1)));

    uint32_t _head = 1;
    while (_head < buffersize) {
        _head *= 2;
    }
    /* FIXME
    int tail = 1;
    while (tail < asize) {
        tail *= 2;
    }
    asize = tail * 0.5;
    */
    uint32_t _tail = _head > 8192 ? _head : 8192;
    //fprintf(stderr, "head %i tail %i irlen %i \n", _head, _tail, asize);
    if (init(_head, _tail, abuf, asize)) {
        ready = true;
        delete[] abuf;
        return true;
    }
    delete[] abuf;
    return false;
}

void DoubleThreadConvolver::compute(int32_t count, float* input, float* output)
{
    if (ready) process(input, output, count);
}

/****************************************************************
 ** SingleThreadConvolver
 */

bool SingleThreadConvolver::get_buffer(std::string fname, float **buffer, uint32_t *rate, int *asize)
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

void SingleThreadConvolver::normalize(float* buffer, int asize) {
    if (!norm) return;
    float gain = 0.0;
    // get gain factor from file buffer
    for (int i = 0; i < asize; i++) {
        current = std::max(gain, std::abs( buffer[i])) ;
        if current > gain {
            gain = current;
        }
    }
    // apply gain factor
    for (int i = 0; i < asize; i++) {
        buffer[i] /= gain;
    }
}

void SingleThreadConvolver::set_normalisation(uint32_t norm_) {
    norm = norm_;
}

bool SingleThreadConvolver::configure(std::string fname, float gain, unsigned int delay, unsigned int offset,
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

    if (init(1024, abuf, asize)) {
        ready = true;
        delete[] abuf;
        return true;
    }
    delete[] abuf;
    return false;
}

void SingleThreadConvolver::compute(int32_t count, float* input, float* output)
{
    if (ready) process(input, output, count);
}
