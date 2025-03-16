/*
 * engine.h
 *
 * SPDX-License-Identifier:  BSD-3-Clause
 *
 * Copyright (C) 2025 brummer <brummer@web.de>
 */


#include <atomic>
#include <mutex>
#include <condition_variable>
#include <cstdint>
#include <cstdlib>
#include <cmath>
#include <iostream>
#include <cstring>
#include <thread>
#include <unistd.h>

#ifdef __SSE__
 #include <immintrin.h>
 #ifndef _IMMINTRIN_H_INCLUDED
  #include <fxsrintrin.h>
 #endif
 #ifdef __SSE3__
  #ifndef _PMMINTRIN_H_INCLUDED
   #include <pmmintrin.h>
  #endif
 #else
  #ifndef _XMMINTRIN_H_INCLUDED
   #include <xmmintrin.h>
  #endif
 #endif //__SSE3__
#endif //__SSE__

#include "dcblocker.cc"
#include "cdelay.cc"
#include "phasecor.cc"

#include "ModelerSelector.h"
#include "fftconvolver.h"

#pragma once

#ifndef ENGINE_H_
#define ENGINE_H_
namespace ratatouille {

/////////////////////////// DENORMAL PROTECTION   //////////////////////

class DenormalProtection {
private:
#ifdef USE_SSE
    uint32_t  mxcsr_mask;
    uint32_t  mxcsr;
    uint32_t  old_mxcsr;
#endif

public:
    inline void set_() {
#ifdef USE_SSE
        old_mxcsr = _mm_getcsr();
        mxcsr = old_mxcsr;
        _mm_setcsr((mxcsr | _MM_DENORMALS_ZERO_MASK | _MM_FLUSH_ZERO_MASK) & mxcsr_mask);
#endif
    };
    inline void reset_() {
#ifdef USE_SSE
        _mm_setcsr(old_mxcsr);
#endif
    };

    inline DenormalProtection() {
#ifdef USE_SSE
        mxcsr_mask = 0xffbf; // Default MXCSR mask
        mxcsr      = 0;
        uint8_t fxsave[512] __attribute__ ((aligned (16))); // Structure for storing FPU state with FXSAVE command

        memset(fxsave, 0, sizeof(fxsave));
        __builtin_ia32_fxsave(&fxsave);
        uint32_t mask = *(reinterpret_cast<uint32_t *>(&fxsave[0x1c])); // Obtain the MXCSR mask from FXSAVE structure
        if (mask != 0)
            mxcsr_mask = mask;
#endif
    };

    inline ~DenormalProtection() {};
};

class Engine
{
public:
    ParallelThread               xrworker;
    cdeleay::Dsp*                cdelay;
    phasecor::Dsp*               pdelay;
    ModelerSelector              slotA;
    ModelerSelector              slotB;
    ConvolverSelector            conv;
    ConvolverSelector            conv1;

    float                        inputGain;
    float                        inputGain1;
    float                        outputGain;
    float                        blend;
    float                        mix;
    float                        delay;
    float                        phasecor_;
    float                        phase_cor;
    float                        buffered;
    float                        latency;
    float                        XrunCounter;

    int32_t                      normSlotA;
    int32_t                      normSlotB;
    int32_t                      rt_prio;
    int32_t                      rt_policy;
    uint32_t                     bypass;
    uint32_t                     s_rate;
    uint32_t                     bufsize;
    uint32_t                     buffersize;
    int                          phaseOffset;

    std::string                  model_file;
    std::string                  model_file1;

    std::string                  ir_file;
    std::string                  ir_file1;

    std::atomic<bool>            _execute;
    std::atomic<bool>            _notify_ui;
    std::atomic<bool>            _neuralA;
    std::atomic<bool>            _neuralB;
    std::atomic<bool>            bufferIsInit;
    std::atomic<int>             _ab;
    std::atomic<int>             _cd;

    inline Engine();
    inline ~Engine();

    inline void init(uint32_t rate, int32_t rt_prio_, int32_t rt_policy_);
    inline void clean_up();
    inline void do_work_mono();
    inline void process(uint32_t n_samples, float* output);

private:
    ParallelThread               pro;
    ParallelThread               par;
    dcblocker::Dsp*              dcb;
    DenormalProtection           MXCSR;
    std::condition_variable      Sync;
    std::mutex                   WMutex;

    float*                       bufferoutput0;
    float*                       bufferinput0;
    float*                       _bufb;

    double                       fRec0[2];
    double                       fRec3[2];
    double                       fRec2[2];
    double                       fRec1[2];
    double                       fRec4[2];

    inline void processSlotB();
    inline void processConv1();
    inline void processBuffer();
    inline void processDsp(uint32_t n_samples, float* output);

    inline void setModel(ModelerSelector *slot,
                std::string *file, std::atomic<bool> *set);

    inline void setIRFile(ConvolverSelector *co, std::string *file);
};

inline Engine::Engine() :
    xrworker(),
    pro(),
    par(),
    dcb(dcblocker::plugin()),
    cdelay(cdeleay::plugin()),
    pdelay(phasecor::plugin()),
    slotA(&Sync),
    slotB(&Sync),
    conv(),
    conv1(),
    bufferoutput0(NULL),
    bufferinput0(NULL),
    _bufb(0) {
        xrworker.start();
        pro.start();
        par.start();
};

inline Engine::~Engine(){
    xrworker.stop();
    pro.stop();
    par.stop();

    delete[] bufferoutput0;
    delete[] bufferinput0;

    dcb->del_instance(dcb);
    cdelay->del_instance(cdelay);
    pdelay->del_instance(pdelay);
    slotA.cleanUp();
    slotB.cleanUp();
    conv.stop_process();
    conv.cleanup();
    conv1.stop_process();
    conv1.cleanup();
};

inline void Engine::init(uint32_t rate, int32_t rt_prio_, int32_t rt_policy_) {
    s_rate = rate;
    dcb->init(rate);
    cdelay->init(rate);
    pdelay->init(rate);
    slotA.init(rate);
    slotB.init(rate);

    rt_prio = rt_prio_;
    rt_policy = rt_policy_;
    bufsize = 0;
    buffersize = 0;
    phaseOffset = 0;
    bypass = 0;
    normSlotA = 0;
    normSlotB = 0;
    inputGain = 0.0;
    inputGain1 = 0.0;
    outputGain = 0.0;
    blend = 0.0;
    mix = 0.0;
    delay = 0.0;
    phasecor_ = 0.0;
    phase_cor = 0;
    buffered = 0.0;
    latency = 0.0;
    XrunCounter = 0.0;

    model_file = "None";
    model_file1 = "None";

    ir_file = "None";
    ir_file1 = "None";

    _neuralA.store(false, std::memory_order_release);
    _neuralB.store(false, std::memory_order_release);
    _execute.store(false, std::memory_order_release);
    _notify_ui.store(false, std::memory_order_release);
    bufferIsInit.store(false, std::memory_order_release);
    _ab.store(0, std::memory_order_release);
    _cd.store(0, std::memory_order_release);

    xrworker.setThreadName("Worker");
    xrworker.set<Engine, &Engine::do_work_mono>(this);

    pro.setThreadName("RT-Parallel");
    pro.setPriority(rt_prio, rt_policy);
    pro.set<0, Engine, &Engine::processSlotB>(this);
    pro.set<1, Engine, &Engine::processConv1>(this);

    par.setThreadName("RT-BUF");
    par.setPriority(rt_prio, rt_policy);
    par.set<Engine, &Engine::processBuffer>(this);

    for (int l0 = 0; l0 < 2; l0 = l0 + 1) fRec0[l0] = 0.0;
    for (int l0 = 0; l0 < 2; l0 = l0 + 1) fRec3[l0] = 0.0;
    for (int l0 = 0; l0 < 2; l0 = l0 + 1) fRec2[l0] = 0.0;
    for (int l0 = 0; l0 < 2; l0 = l0 + 1) fRec1[l0] = 0.0;
    for (int l0 = 0; l0 < 2; l0 = l0 + 1) fRec4[l0] = 0.0;
};

void Engine::clean_up()
{
    for (int l0 = 0; l0 < 2; l0 = l0 + 1) fRec0[l0] = 0.0;
    for (int l0 = 0; l0 < 2; l0 = l0 + 1) fRec3[l0] = 0.0;
    for (int l0 = 0; l0 < 2; l0 = l0 + 1) fRec2[l0] = 0.0;
    for (int l0 = 0; l0 < 2; l0 = l0 + 1) fRec1[l0] = 0.0;
    for (int l0 = 0; l0 < 2; l0 = l0 + 1) fRec4[l0] = 0.0;
    // delete the internal DSP mem
}

inline void Engine::setModel(ModelerSelector *slot,
                std::string *file, std::atomic<bool> *set) {
    if ((*file).compare(slot->getModelFile()) != 0) {
        slot->setModelFile(*file);
        if (!slot->loadModel()) {
            *file = "None";
            set->store(false, std::memory_order_release);
        } else {
            set->store(true, std::memory_order_release);
        }
    }
}

inline void Engine::setIRFile(ConvolverSelector *co, std::string *file) {
    if (co->is_runnable()) {
        co->set_not_runnable();
        co->stop_process();
        std::unique_lock<std::mutex> lk(WMutex);
        Sync.wait_for(lk, std::chrono::milliseconds(160));
    }

    co->cleanup();
    co->set_samplerate(s_rate);
    co->set_buffersize(bufsize);

    if (*file != "None") {
        co->configure(*file, 1.0, 0, 0, 0, 0, 0);
        while (!co->checkstate());
        if(!co->start(rt_prio, rt_policy)) {
            *file = "None";
           // lv2_log_error(&logger,"impulse convolver update fail\n");
        }
    }
}

void Engine::do_work_mono() {
    // set neural models
    if (_ab.load(std::memory_order_acquire) == 1) {
        setModel(&slotA, &model_file, &_neuralA);
    } else if (_ab.load(std::memory_order_acquire) == 2) {
        setModel(&slotB, &model_file1, &_neuralB);
    } else if (_ab.load(std::memory_order_acquire) > 2) {
        setModel(&slotA, &model_file, &_neuralA);
        setModel(&slotB, &model_file1, &_neuralB);
    }
    // set ir files
    if (_cd.load(std::memory_order_acquire) == 1) {
        setIRFile(&conv, &ir_file);
    } else if (_cd.load(std::memory_order_acquire) == 2) {
        setIRFile(&conv1, &ir_file1);
    } else if (_cd.load(std::memory_order_acquire) > 2) {
        setIRFile(&conv, &ir_file);
        setIRFile(&conv1, &ir_file1);
    }

    // calculate phase offset
    if (_neuralA.load(std::memory_order_acquire) && _neuralB.load(std::memory_order_acquire)) {
        phaseOffset = slotB.getPhaseOffset() - slotA.getPhaseOffset();
        pdelay->set(phaseOffset);
        pdelay->clear_state_f();
        //fprintf(stderr, "phase offset = %i\n", phaseOffset);
    } else {
        phaseOffset = 0;
        pdelay->set(phaseOffset);
        pdelay->clear_state_f();
    }
    
    // init buffer for background processing
    if (buffersize < bufsize) {
        buffersize = bufsize * 2;
        delete[] bufferoutput0;
        bufferoutput0 = NULL;
        bufferoutput0 = new float[buffersize];
        memset(bufferoutput0, 0, buffersize*sizeof(float));
        delete[] bufferinput0;
        bufferinput0 = NULL;
        bufferinput0 = new float[buffersize];
        memset(bufferinput0, 0, buffersize*sizeof(float));
        par.setTimeOut(std::max(100,static_cast<int>((bufsize/(s_rate*0.000001))*0.1)));
        bufferIsInit.store(true, std::memory_order_release);
        // set wait function time out for parallel processor thread
        pro.setTimeOut(std::max(100,static_cast<int>((bufsize/(s_rate*0.000001))*0.1)));
    }
    // set flag that work is done ready
    _execute.store(false, std::memory_order_release);
    // set flag that GUI need information about changed state
    _notify_ui.store(true, std::memory_order_release);
}

// process slotB in parallel thread
inline void Engine::processSlotB() {
    slotB.compute(bufsize, _bufb, _bufb);
    if (normSlotB) slotB.normalize(bufsize, _bufb);
}

// process second convolver in parallel thread
inline void Engine::processConv1() {
    conv1.compute(bufsize, _bufb, _bufb);
}

// process dsp in buffered in a background thread
inline void Engine::processBuffer() {
    processDsp(bufsize, bufferoutput0);
}

inline void Engine::processDsp(uint32_t n_samples, float* output)
{
    if(n_samples<1) return;

    // basic bypass
    if (!bypass) {
        Sync.notify_all();
        return;
    }
    MXCSR.set_();

    // get controller values from host
    double fSlow0 = 0.0010000000000000009 * std::pow(1e+01, 0.05 * double(inputGain));
    double fSlow4 = 0.0010000000000000009 * std::pow(1e+01, 0.05 * double(inputGain1));
    double fSlow3 = 0.0010000000000000009 * std::pow(1e+01, 0.05 * double(outputGain));
    double fSlow2 = 0.0010000000000000009 * double(blend);
    double fSlow1 = 0.0010000000000000009 * double(mix);

    // internal buffer
    float bufa[n_samples];
    memcpy(bufa, output, n_samples*sizeof(float));
    float bufb[n_samples];
    memcpy(bufb, output, n_samples*sizeof(float));
    bufsize = n_samples;

    // process delta delay
    if (delay < 0) cdelay->compute(n_samples, bufa, bufa);
    else cdelay->compute(n_samples, bufb, bufb);

    // clear phase correction when switch on/off
    if (phasecor_ != phase_cor) {
        phase_cor = phasecor_;
        pdelay->clear_state_f();
    }
    // process phase correction
    if (phasecor_ && phaseOffset) {
        if (phaseOffset < 0) pdelay->compute(n_samples, bufa, bufa);
        else pdelay->compute(n_samples, bufb, bufb);
    }

    // process input volume slot A
    if (_neuralA.load(std::memory_order_acquire)) {
        for (uint32_t i0 = 0; i0 < n_samples; i0 = i0 + 1) {
            fRec0[0] = fSlow0 + 0.999 * fRec0[1];
            bufa[i0] = float(double(bufa[i0]) * fRec0[0]);
            fRec0[1] = fRec0[0];
        }
    }

    // process input volume slot B
    if (_neuralB.load(std::memory_order_acquire)) {
        for (uint32_t i0 = 0; i0 < n_samples; i0 = i0 + 1) {
            fRec4[0] = fSlow4 + 0.999 * fRec4[1];
            bufb[i0] = float(double(bufb[i0]) * fRec4[0]);
            fRec4[1] = fRec4[0];
        }
    }

    // process slot B in parallel thread
    _bufb = bufb;
    if (_neuralB.load(std::memory_order_acquire) ) {
        if ( pro.getProcess()) {
            pro.setProcessor(0);
            pro.runProcess();
        } else {
            XrunCounter += 1;
            _notify_ui.store(true, std::memory_order_release);
            //lv2_log_error(&logger,"thread RT missing deadline\n");
            /*if (!_execute.load(std::memory_order_acquire)) {
                _ab.store(2, std::memory_order_release);
                 model_file1 = "None";
                _execute.store(true, std::memory_order_release);
                xrworker.runProcess();
            }*/
            //processSlotB();
        }
    }

    // process slot A
    if (_neuralA.load(std::memory_order_acquire)) {
        slotA.compute(n_samples, bufa, bufa);
        if (normSlotA) slotA.normalize(n_samples, bufa);
    }

    //wait for parallel processed slot B when needed
    if (_neuralB.load(std::memory_order_acquire)) {
        if (!pro.processWait()) {
            XrunCounter += 1;
            _notify_ui.store(true, std::memory_order_release);
            //lv2_log_error(&logger,"thread RT missing wait\n");
            /*if (!_execute.load(std::memory_order_acquire)) {
                _ab.store(2, std::memory_order_release);
                 model_file1 = "None";
                _execute.store(true, std::memory_order_release);
                xrworker.runProcess();
            }*/
        }
    }

    // mix output when needed
    if (_neuralA.load(std::memory_order_acquire) && _neuralB.load(std::memory_order_acquire)) {
        for (uint32_t i0 = 0; i0 < n_samples; i0 = i0 + 1) {
            fRec2[0] = fSlow2 + 0.999 * fRec2[1];
            output[i0] = bufa[i0] * (1.0 - fRec2[0]) + bufb[i0] * fRec2[0];
            fRec2[1] = fRec2[0];
        }
    } else if (_neuralA.load(std::memory_order_acquire)) {
        memcpy(output, bufa, n_samples*sizeof(float));
    } else if (_neuralB.load(std::memory_order_acquire)) {
        memcpy(output, bufb, n_samples*sizeof(float));
    }

    if (_neuralA.load(std::memory_order_acquire) || _neuralB.load(std::memory_order_acquire)) {
        // output volume
        for (uint32_t i0 = 0; i0 < n_samples; i0 = i0 + 1) {
            fRec3[0] = fSlow3 + 0.999 * fRec3[1];
            output[i0] = float(double(output[i0]) * fRec3[0]);
            fRec3[1] = fRec3[0];
        }
    }

    // run dcblocker
    dcb->compute(n_samples, output, output);

    // set buffer for mix control
    memcpy(bufa, output, n_samples*sizeof(float));
    memcpy(bufb, output, n_samples*sizeof(float));

    // process conv1 in parallel thread
    _bufb = bufb;
    if (!_execute.load(std::memory_order_acquire) && conv1.is_runnable()) {
        if (pro.getProcess()) {
            pro.setProcessor(1);
            pro.runProcess();
        } else {
            XrunCounter += 1;
            _notify_ui.store(true, std::memory_order_release);
            //lv2_log_error(&logger,"thread RT (conv) missing deadline\n");
            /*if (!_execute.load(std::memory_order_acquire)) {
                _cd.store(2, std::memory_order_release);
                 ir_file1 = "None";
                _execute.store(true, std::memory_order_release);
                xrworker.runProcess();
            }*/
            //processConv1();
        }
    }

    // process conv
    if (!_execute.load(std::memory_order_acquire) && conv.is_runnable())
        conv.compute(n_samples, bufa, bufa);

    // wait for parallel processed conv1 when needed
    if (!_execute.load(std::memory_order_acquire) && conv1.is_runnable()) {
        if (!pro.processWait()) {
            XrunCounter += 1;
            _notify_ui.store(true, std::memory_order_release);
            //lv2_log_error(&logger,"thread RT (conv) missing wait\n");
            /*if (!_execute.load(std::memory_order_acquire)) {
                _cd.store(2, std::memory_order_release);
                 ir_file1 = "None";
                _execute.store(true, std::memory_order_release);
                xrworker.runProcess();
            }*/
        }
    }

    // mix output when needed
    if ((!_execute.load(std::memory_order_acquire) &&
            conv.is_runnable()) && conv1.is_runnable()) {
        for (uint32_t i0 = 0; i0 < n_samples; i0 = i0 + 1) {
            fRec1[0] = fSlow1 + 0.999 * fRec1[1];
            output[i0] = bufa[i0] * (1.0 - fRec1[0]) + bufb[i0] * fRec1[0];
            fRec1[1] = fRec1[0];
        }
    } else if (!_execute.load(std::memory_order_acquire) && conv.is_runnable()) {
        memcpy(output, bufa, n_samples*sizeof(float));
    } else if (!_execute.load(std::memory_order_acquire) && conv1.is_runnable()) {
        memcpy(output, bufb, n_samples*sizeof(float));
    }
    // notify neural modeller that process cycle is done
    Sync.notify_all();
    MXCSR.reset_();
}

inline void Engine::process(uint32_t n_samples, float* output) {
    // process in buffered mode
    if ((buffered > 0.0) && bufferIsInit.load(std::memory_order_acquire)) {
        // avoid buffer overflow on frame size change
        if (buffersize < n_samples) {
            bufsize = n_samples;
            bufferIsInit.store(false, std::memory_order_release);
            _execute.store(true, std::memory_order_release);
            xrworker.runProcess();
            return;
        }
        // get the buffer from previous process
        if (!par.processWait()) {
            XrunCounter += 1;
            _notify_ui.store(true, std::memory_order_release);
           // lv2_log_error(&logger,"thread RTBUF missing wait\n");
            // if deadline was missing, erase models from processing
            /*if (!_execute.load(std::memory_order_acquire)) {
                _ab.store(3, std::memory_order_release);
                 model_file = "None";
                 model_file1 = "None";
                _execute.store(true, std::memory_order_release);
                xrworker.runProcess();
            }*/
        }
        // copy incoming data to internal input buffer
        memcpy(bufferinput0, output, n_samples*sizeof(float));

        bufsize = n_samples;
        // copy processed data from last circle to output
        memcpy(output, bufferoutput0, bufsize*sizeof(float));
        // copy internal input buffer to process buffer for next circle
        memcpy(bufferoutput0, bufferinput0, bufsize*sizeof(float));

        // process data in background thread
        if (par.getProcess()) par.runProcess();
        else {
            XrunCounter += 1;
            _notify_ui.store(true, std::memory_order_release);
            // lv2_log_error(&logger,"thread RTBUF missing deadline\n");
            // if deadline was missing, erase models from processing
           /* if (!_execute.load(std::memory_order_acquire)) {
                _ab.store(3, std::memory_order_release);
                 model_file = "None";
                 model_file1 = "None";
                _execute.store(true, std::memory_order_release);
                xrworker.runProcess();
            }*/
        }
        latency = n_samples;
    } else {
        // process latency free
        processDsp(n_samples, output);
        latency = 0.0;
    }
}

}; // end namespace ratatouille
#endif
