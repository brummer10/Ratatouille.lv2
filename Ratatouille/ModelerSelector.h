/*
 * ModelerSelector.h
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
#include <cstring>
#include <condition_variable>

#include "dsp.h"
#include "get_dsp.h"
#include "activations.h"

#include "RTNeural.h"

#include "gx_resampler.h"

#pragma once

#ifndef MODELER_SELECTOR_H_
#define MODELER_SELECTOR_H_


namespace ratatouille {


/****************************************************************
 ** ModelerBase - virtual base class to handle neural model loading and processing
 */

class ModelerBase {
public:
    virtual void setModelFile(std::string modelFile_) {}
    virtual inline void clearState() {}
    virtual inline void init(unsigned int sample_rate) {}
    virtual void connect(uint32_t port,void* data) {}
    virtual inline void normalize(int count, float *buf) {}
    virtual inline void compute(int count, float *input0, float *output0) {}
    virtual bool loadModel() { return false;}
    virtual void unloadModel() {}

    ModelerBase() {};
    virtual ~ModelerBase() {};
};


/****************************************************************
 ** NeuralModel - class to handle *.nam neural model loading and processing
 */

class NeuralModel : public ModelerBase {
private:
    nam::DSP*                       model;
    gx_resample::FixedRateResampler smp;

    std::atomic<bool>               ready;

    int                             fSampleRate;
    int                             modelSampleRate;
    int                             needResample;

    float                           loudness;

    bool                            isInited;
    std::mutex                      WMutex;
    std::condition_variable*        SyncWait;

public:
    std::string                     modelFile;
    float                           nGain;

    void setModelFile(std::string modelFile_) override { modelFile = modelFile_;}
    inline void clearState() override;
    inline void init(unsigned int sample_rate) override;
    void connect(uint32_t port,void* data) override;
    inline void normalize(int count, float *buf) override;
    inline void compute(int count, float *input0, float *output0) override;
    bool loadModel() override;
    void unloadModel() override;

    NeuralModel(std::condition_variable *var);
    ~NeuralModel();
};


/****************************************************************
 ** RtNeuralModel - class to handle *.json / *.aidax neural model loading and processing
 */

class RtNeuralModel : public ModelerBase {
private:
    RTNeural::Model<float>*         model;
    gx_resample::FixedRateResampler smp;

    std::atomic<bool>               ready;

    int                             fSampleRate;
    int                             modelSampleRate;
    int                             needResample;

    bool                            isInited;
    std::mutex                      WMutex;
    std::condition_variable*        SyncWait;

    void get_samplerate(std::string config_file, int *mSampleRate);

public:
    std::string                     modelFile;

    void setModelFile(std::string modelFile_) override { modelFile = modelFile_;}
    inline void clearState() override;
    inline void init(unsigned int sample_rate) override;
    void connect(uint32_t port,void* data) override;
    inline void normalize(int count, float *buf) override;
    inline void compute(int count, float *input0, float *output0) override;
    bool loadModel() override;
    void unloadModel() override;

    RtNeuralModel(std::condition_variable *var);
    ~RtNeuralModel();
};


/****************************************************************
 ** ModlerSelector - class to set neural modeler according to the file to load 
 */

class ModelerSelector {
public:

    void setModelFile(std::string modelFile_) {
            if (needNewModeler(modelFile_)) {
                selectModeler();
                modeler->init(sampleRate);
            }
            return modeler->setModelFile(modelFile_);}

    inline void clearState() {
            return modeler->clearState();}

    inline void init(unsigned int sample_rate) {
            sampleRate = sample_rate;
            return modeler->init(sample_rate);}

    void connect(uint32_t port,void* data) {
            return modeler->connect(port, data);}

    inline void normalize(int count, float *buf) {
            return modeler->normalize(count, buf); }

    inline void compute(int count, float *input0, float *output0) {
            return modeler->compute(count, input0, output0);}

    bool loadModel() {
            return modeler->loadModel();}

    void unloadModel() {
            return modeler->unloadModel();}

    ModelerSelector(std::condition_variable *var) :
            noModel(),
            namModel(var),
            rtnModel(var) {
            modeler = &noModel;
            sampleRate = 0;
            isNam = 3;}

    ~ModelerSelector() {}

private:
    ModelerBase *modeler;
    ModelerBase noModel;
    NeuralModel namModel;
    RtNeuralModel rtnModel;

    uint32_t sampleRate;
    int isNam;

    void selectModeler() {
            isNam ?
            modeler = dynamic_cast<ModelerBase*>(&namModel) :
            modeler = dynamic_cast<ModelerBase*>(&rtnModel);}

    bool needNewModeler(std::string newModelFile) {
            bool ret = true;
            int set = 0;
            std::string::size_type idx;
            std::string newExtension;
            idx = newModelFile.rfind('.');
            if(idx != std::string::npos)
                newExtension = newModelFile.substr(idx+1);
            if (!newExtension.empty()) {
                if (newExtension.compare("nam")) set = 0;
                else if (newExtension.compare("json")) set = 1;
                else if (newExtension.compare("aidax")) set = 1;
                ret = (isNam == set) ? false : true;
                isNam = set;
            }
            return ret;}

};

} // end namespace ratatouille
#endif // MODELER_SELECTOR_H_
