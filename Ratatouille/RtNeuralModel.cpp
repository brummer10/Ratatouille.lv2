/*
 * RtNeuralModel.cc
 *
 * SPDX-License-Identifier:  BSD-3-Clause
 *
 * Copyright (C) 2024 brummer <brummer@web.de>
 */


#include "ModelerSelector.h"


namespace ratatouille {

RtNeuralModel::RtNeuralModel(std::condition_variable *Sync)
    : model(nullptr), smp(), SyncWait(Sync) {
    needResample = 0;
    phaseOffset = 0;
    isInited = false;
    ready.store(false, std::memory_order_release);
 }

RtNeuralModel::~RtNeuralModel() {
    delete model;
}

inline void RtNeuralModel::clearState()
{
}

inline void RtNeuralModel::init(unsigned int sample_rate)
{
    fSampleRate = sample_rate;
    clearState();
    isInited = true;
    loadModel();
}

// connect the Ports used by the plug-in class
void RtNeuralModel::connect(uint32_t port,void* data)
{
}

int RtNeuralModel::getPhaseOffset()
{
    return phaseOffset;
}

inline void RtNeuralModel::normalize(int count, float *buf)
{
    // not implemented
}

inline void RtNeuralModel::compute(int count, float *input0, float *output0)
{
    if (!model ) return;
    if (output0 != input0)
        memcpy(output0, input0, count*sizeof(float));

    float bufa[count];
    memcpy(bufa, output0, count*sizeof(float));

    //process model 
    if (model && ready.load(std::memory_order_acquire)) {
        if (needResample) {
            int ReCounta = count;
            if (needResample == 1) {
                ReCounta = smp.max_out_count(count);
            } else if (needResample == 2) {
                ReCounta = static_cast<int>(ceil((count*static_cast<double>(modelSampleRate))/fSampleRate));
            }
            float bufa1[ReCounta];
            memset(bufa1, 0, ReCounta*sizeof(float));
            if (needResample == 1) {
                ReCounta = smp.up(count, bufa, bufa1);
            } else if (needResample == 2) {
                smp.down(bufa, bufa1);
            } else {
                memcpy(bufa1, bufa, ReCounta * sizeof(float));
            }
            for (int i0 = 0; i0 < ReCounta; i0 = i0 + 1) {
                 bufa1[i0] = model->forward (&bufa1[i0]);
            }
            if (needResample == 1) {
                smp.down(bufa1, bufa);
            } else if (needResample == 2) {
                smp.up(ReCounta, bufa1, bufa);
            }
        } else {
            for (int i0 = 0; i0 < count; i0 = i0 + 1) {
                 bufa[i0] = model->forward (&bufa[i0]);
            }
        }
        memcpy(output0, bufa, count*sizeof(float));
    }
}

void RtNeuralModel::get_samplerate(std::string config_file, int *mSampleRate) {
    std::ifstream infile(config_file);
    infile.imbue(std::locale::classic());
    std::string line;
    std::string key;
    std::string value;
    if (infile.is_open()) {
        while (std::getline(infile, line)) {
            std::istringstream buf(line);
            buf >> key;
            buf >> value;
            if (key.compare("\"samplerate\":") == 0) {
                value.erase(std::remove(value.begin(), value.end(), '\"'), value.end());
                (*mSampleRate) = std::stoi(value);
                break;
            }
            key.clear();
            value.clear();
        }
        infile.close();
    }
}

// non rt callback
bool RtNeuralModel::loadModel() {
    if (!modelFile.empty() && isInited) {
       // fprintf(stderr, "Load file %s\n", modelFile.c_str());
        std::unique_lock<std::mutex> lk(WMutex);
        ready.store(false, std::memory_order_release);
        SyncWait->wait(lk);
        delete model;
       // fprintf(stderr, "delete model\n");
        model = nullptr;
        modelSampleRate = 0;
        needResample = 0;
        phaseOffset = 0;
        //clearState();
        int32_t warmUpSize = 4096;
        try {
            get_samplerate(std::string(modelFile), &modelSampleRate);
            std::ifstream jsonStream(std::string(modelFile), std::ifstream::binary);
            model = RTNeural::json_parser::parseJson<float>(jsonStream).release();
        } catch (const std::exception&) {
            modelFile = "None";
        }
        
        if (model) {
            model->reset();
            if (modelSampleRate <= 0) modelSampleRate = 48000;
            if (modelSampleRate > fSampleRate) {
                smp.setup(fSampleRate, modelSampleRate);
                needResample = 1;
            } else if (modelSampleRate < fSampleRate) {
                smp.setup(modelSampleRate, fSampleRate);
                needResample = 2;
            } 
            // fprintf(stderr, "A: %s\n", modelFile.c_str());

            float* buffer = new float[warmUpSize];
            float angle = 0.0;
            for(int i=0;i<warmUpSize;i++){
                buffer[i] = sin(angle);
                angle += (2 * 3.14159365) / warmUpSize;
            }
            //memset(buffer, 0, warmUpSize * sizeof(float));

            for (int i0 = 0; i0 < warmUpSize; i0 = i0 + 1) {
                buffer[i0] = model->forward (&buffer[i0]);
            }

            for(int i=0;i<warmUpSize;i++){
                if (!std::signbit(buffer[i+1]) != !std::signbit(buffer[i])) {
                    phaseOffset = i;
                    break;
                }
            }

            delete[] buffer;
        } 
        ready.store(true, std::memory_order_release);
    }
    if (model) return true;
    return false;
}

// non rt callback
void RtNeuralModel::unloadModel() {
    std::unique_lock<std::mutex> lk(WMutex);
    ready.store(false, std::memory_order_release);
    SyncWait->wait(lk);
    delete model;
   // fprintf(stderr, "delete model\n");
    model = nullptr;
    modelSampleRate = 0;
    needResample = 0;
    //clearState();
    ready.store(true, std::memory_order_release);
}

} // end namespace ratatouille
