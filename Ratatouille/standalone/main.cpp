/*
 * main.cpp
 *
 * SPDX-License-Identifier:  BSD-3-Clause
 *
 * Copyright (C) 2025 brummer <brummer@web.de>
 */


#include <errno.h>
#include <unistd.h>
#include <signal.h>

#include <atomic>
#include <iostream>
#include <string>
#include <cmath>

#if defined(HAVE_PA)
#include "xpa.h"
#endif

#include "Ratatouille.cc"

Ratatouille *r;

#if defined(HAVE_JACK)
#include "jack.cc"
#endif


// send value changes from GUI to the engine
void sendValueChanged(X11_UI *ui, int port, float value) {
    r->sendValueChanged(port, value);
}

// send a file name from GUI to the engine
void sendFileName(X11_UI *ui, ModelPicker* m, int old){
    r->sendFileName(m, old);
}

// the portaudio server process callback
#if defined(HAVE_PA)
static int process(const void* inputBuffer, void* outputBuffer,
    unsigned long nframes, const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags, void* data) {

    const float* input = ((const float**)inputBuffer)[0];
    float* output = ((float**)outputBuffer)[0];

    if(output != input)
        memcpy(output, input, nframes*sizeof(float));

    r->process(nframes, output);

    return 0;
}
#endif

int main(int argc, char *argv[]){

    #if defined(__linux__) || defined(__FreeBSD__) || \
        defined(__NetBSD__) || defined(__OpenBSD__)
    if(0 == XInitThreads()) 
        fprintf(stderr, "Warning: XInitThreads() failed\n");
    #endif
    #if defined(HAVE_PA)
    bool runPA = false;
    #endif
    r = new Ratatouille();
    r->startGui();

    #if defined(__linux__) || defined(__FreeBSD__) || \
        defined(__NetBSD__) || defined(__OpenBSD__)
    signal (SIGQUIT, signal_handler);
    signal (SIGTERM, signal_handler);
    signal (SIGHUP, signal_handler);
    signal (SIGINT, signal_handler);
    #endif

    #if defined(HAVE_PA)
    XPa xpa ("Ratatouille");
    if(!xpa.openStream(1, 1, &process, nullptr)) {
        #if defined(HAVE_JACK)
        startJack();
        #else    
        r->quitGui();
        #endif
    } else {
        runPA = true;
        r->initEngine(xpa.getSampleRate(), 25, 1);
        r->enableEngine(1);
        r->readConfig();
        if(!xpa.startStream()) r->quitGui();
    }
    #else
    startJack();
    #endif

    main_run(r->getMain());

    r->cleanup();
    #if defined(HAVE_PA)
    if (runPA) xpa.stopStream();
    #if defined(HAVE_JACK)
    else quitJack();
    #endif
    #else
    quitJack();
    #endif

    delete r;

    printf("bye bye\n");
    return 0;
}

