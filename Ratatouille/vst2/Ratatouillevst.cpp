/*
 * Ratatouillevst.cpp
 *
 * SPDX-License-Identifier:  BSD-3-Clause
 *
 * Copyright (C) 2025 brummer <brummer@web.de>
 */

#include "vestige.h"

#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <stdint.h>
#include <stddef.h>

#define IS_VST2
#include "Ratatouille.cc"

typedef struct ERect {
    short top;
    short left;
    short bottom;
    short right;
} ERect;

#define PLUGIN_UID 'Rabr'

#define WINDOW_WIDTH  610
#define WINDOW_HEIGHT 419

#define FlagsChunks (1 << 5)

/****************************************************************
 ** ratatouille_plugin_t -> the plugin struct
 */

struct ratatouille_plugin_t {
    AEffect* effect;
    Ratatouille *r;
    ERect editorRect;
    int width, height;
    float SampleRate;
    std::string state;
    bool isInited;
    bool guiIsCreated;
};

/****************************************************************
 ** Parameter handling not used here
 */

static void setParameter(AEffect* effect, int32_t index, float value) {
}

static float getParameter(AEffect* effect, int32_t index) {
    return 0.0;
}

static void getParameterName(AEffect*, int32_t index, char* label) {
}

/****************************************************************
 ** The audio process
 */

static void processReplacing(AEffect* effect, float** inputs, float** outputs, int32_t sampleFrames) {
    ratatouille_plugin_t* plug = (ratatouille_plugin_t*)effect->object;

    float* input = inputs[0];
    float* output = outputs[0];
    if(output != input)
        memcpy(output, input, sampleFrames*sizeof(float));
    
    plug->r->process(sampleFrames, output);
}

/****************************************************************
 ** Save and load state
 */

void saveState(ratatouille_plugin_t* plug, void** data, int* size, int isBank) {
    plug->r->saveState(&plug->state);
    *size = strlen(plug->state.c_str());
    // only save data here for later loading
    *data = (void*)plug->state.c_str();
}

void loadState(ratatouille_plugin_t* plug, int size, int isBank) {
    if (plug->state.empty()) return;
    plug->r->readState(plug->state);
}

/****************************************************************
 ** The Dispatcher
 */

static intptr_t dispatcher(AEffect* effect, int32_t opCode, int32_t index, intptr_t value, void* ptr, float opt) {
    ratatouille_plugin_t* plug = (ratatouille_plugin_t*)effect->object;
    switch (opCode) {
        case effEditGetRect:
            if (ptr) *(ERect**)ptr = &plug->editorRect;
            return 1;
        case effGetEffectName:
            strncpy((char*)ptr, "Ratatouille", VestigeMaxNameLen - 1);
            ((char*)ptr)[VestigeMaxNameLen - 1] = '\0';
            return 1;
        case effGetVendorString:
            strncpy((char*)ptr, "brummer", VestigeMaxNameLen - 1);
            ((char*)ptr)[VestigeMaxNameLen - 1] = '\0';
            return 1;
        case effGetProductString:
            strncpy((char*)ptr, "brummer", VestigeMaxNameLen - 1);
            ((char*)ptr)[VestigeMaxNameLen - 1] = '\0';
            return 1;
        case effGetPlugCategory:
            return kPlugCategEffect;
        case effOpen:
            break;
        case effClose:
            plug->r->quitGui();
            delete plug->r;
            free(plug);
            break;
        case effGetParamName:
            getParameterName(effect, index, (char*)ptr);
            break;
        case effSetSampleRate:
            plug->SampleRate = opt;
            plug->r->initEngine((uint32_t)plug->SampleRate, 25, 1);
            plug->isInited = true;
            loadState(plug, 0, 0);
            break;
        case effEditOpen: {
            Window hostWin = (Window)(size_t)ptr;
            plug->r->startGui();
            plug->r->enableEngine(1);
            plug->r->setParent(hostWin);
            plug->r->showGui();
            plug->guiIsCreated = true;
            break;
        }
        case effEditClose: {
            if (plug->guiIsCreated) {
                plug->r->cleanup();
                plug->r->quitMain();
            }
            plug->r->quitGui();
            break;
        }
        case effEditIdle:
            break;
        //case effGetProgram:
        case 23: { // effGetChunk
            void* chunkData = nullptr;
            int   chunkSize = 0;
            saveState(plug, &chunkData, &chunkSize, index); // index=0: program, 1: bank
            *(void**)ptr = chunkData;
            return chunkSize;
        }
        //case effSetProgram:
        case 24: { // effSetChunk
            plug->state = (const char*) ptr;
            // read state, but load it only after we got the sample rate
            if (plug->isInited) loadState(plug, 0, 0);
            break;
        }
        default: break;
    }
    return 0;
}

/****************************************************************
 ** The Main Entry
 */

extern "C" __attribute__ ((visibility ("default")))
AEffect* VSTPluginMain(audioMasterCallback audioMaster) {
    ratatouille_plugin_t* plug = (ratatouille_plugin_t*)calloc(1, sizeof(ratatouille_plugin_t));
    AEffect* effect = (AEffect*)calloc(1, sizeof(AEffect));
    plug->r = new Ratatouille();
    effect->object = plug;
    plug->effect = effect;
    plug->width = WINDOW_WIDTH;
    plug->height = WINDOW_HEIGHT;
    plug->editorRect = {0, 0, (short) plug->height, (short) plug->width};
    plug->SampleRate = 48000.0;
    plug->isInited = false;
    plug->guiIsCreated = false;

    effect->magic = kEffectMagic;
    effect->dispatcher = dispatcher;
    effect->processReplacing = processReplacing;
    effect->setParameter = setParameter;
    effect->getParameter = getParameter;
    effect->numPrograms = 1;
    effect->numParams = 0;
    effect->numInputs = 1;
    effect->numOutputs = 1;
    effect->flags = effFlagsHasEditor | effFlagsCanReplacing | FlagsChunks;
    effect->uniqueID = PLUGIN_UID;
    return effect;
}
