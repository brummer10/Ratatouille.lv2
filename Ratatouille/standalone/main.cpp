/*
 * main.cpp
 *
 * SPDX-License-Identifier:  BSD-3-Clause
 *
 * Copyright (C) 2024 brummer <brummer@web.de>
 */


#include <errno.h>
#include <unistd.h>
#include <signal.h>

#include <atomic>
#include <iostream>
#include <string>
#include <cmath>

#include <jack/jack.h>
#include <jack/thread.h>

#include "Ratatouille.h"

Ratatouille *r;

jack_client_t *client;
jack_port_t *in_port;
jack_port_t *out_port;

// send value changes to the engine
void sendValueChanged(X11_UI *ui, int port, float value) {
    switch (port) {
        // 0 + 1 audio ports
        case 2:
            r->engine.inputGain = value;
        break;
        case 3:
            r->engine.outputGain = value;
        break;
        case 4:
            r->engine.blend = value;
        break;
        // 5 + 6 atom ports
        case 7:
            r->engine.mix = value;
        break;
        case 8:
            r->engine.delay = value;
            r->engine.cdelay->connect(8,(void*) &value);
        break;
        case 9:
        {
            r->engine._ab.fetch_add(12, std::memory_order_relaxed);
            r->engine.conv.set_normalisation(static_cast<uint32_t>(value));
            if (r->engine.ir_file.compare("None") != 0) {
                r->workToDo.store(true, std::memory_order_release);
            }
        }
        break;
        case 10:
        {
            r->engine._ab.fetch_add(12, std::memory_order_relaxed);
            r->engine.conv1.set_normalisation(static_cast<uint32_t>(value));
            if (r->engine.ir_file1.compare("None") != 0) {
                r->workToDo.store(true, std::memory_order_release);
            }
        }
        break;
        case 11:
            r->engine.inputGain1 = value;
        break;
        case 12:
            r->engine.normSlotA = static_cast<int32_t>(value);
        break;
        case 13:
            r->engine.normSlotB = static_cast<int32_t>(value);
        break;
        case 14:
            r->engine.bypass = static_cast<int32_t>(value);
        break;
        case 15:
        {
            r->engine._ab.fetch_add(1, std::memory_order_relaxed);
            r->engine.model_file = "None";
            r->workToDo.store(true, std::memory_order_release);
        }
        break;
        case 16:
        {
            r->engine._ab.fetch_add(2, std::memory_order_relaxed);
            r->engine.model_file1 = "None";
            r->workToDo.store(true, std::memory_order_release);
         }
        break;
        case 17:
        {
            r->engine._ab.fetch_add(12, std::memory_order_relaxed);
            r->engine.ir_file = "None";
            r->workToDo.store(true, std::memory_order_release);
        }
        break;
        case 18:
        {
            r->engine._ab.fetch_add(12, std::memory_order_relaxed);
            r->engine.ir_file1 = "None";
            r->workToDo.store(true, std::memory_order_release);
        }
        break;
        // 19 latency
        case 20:
        {
            r->engine.buffered = value;
        }
        break;
        case 21:
        {
            r->engine.phasecor_ = value;
        }
        break;
        default:
        break;
    }
}

// send a file name to the engine
void sendFileName(X11_UI *ui, ModelPicker* m, int old){
    X11_UI_Private_t *ps = (X11_UI_Private_t*)r->ui->private_ptr;
    if ((strcmp(m->filename, "None") == 0)) {
        if (old == 1) {
            if ( m == &ps->ma) {
                r->engine.model_file = m->filename;
                r->engine._ab.fetch_add(1, std::memory_order_relaxed);
            } else {
                r->engine.model_file1 = m->filename;
                r->engine._ab.fetch_add(2, std::memory_order_relaxed);
            }
        } else if (old == 2) {
            if ( m == &ps->ir) {
                r->engine.ir_file = m->filename;
                r->engine._ab.fetch_add(12, std::memory_order_relaxed);
            } else {
                r->engine.ir_file1 = m->filename;
                r->engine._ab.fetch_add(12, std::memory_order_relaxed);
            }
        } else return;
    } else if (ends_with(m->filename, "nam") ||
               ends_with(m->filename, "json") ||
               ends_with(m->filename, "aidax")) {
        if ( m == &ps->ma) {
            r->engine.model_file = m->filename;
            r->engine._ab.fetch_add(1, std::memory_order_relaxed);
        } else {
            r->engine.model_file1 = m->filename;
            r->engine._ab.fetch_add(2, std::memory_order_relaxed);
        }
    } else if (ends_with(m->filename, "wav")) {
        if ( m == &ps->ir) {
            r->engine.ir_file = m->filename;
            r->engine._ab.fetch_add(12, std::memory_order_relaxed);
        } else {
            r->engine.ir_file1 = m->filename;
            r->engine._ab.fetch_add(12, std::memory_order_relaxed);
        }
    } else return;
    r->workToDo.store(true, std::memory_order_release);
}

void jack_shutdown (void *arg) {
    fprintf (stderr, "jack shutdown, exit now \n");
    #if defined(__linux__) || defined(__FreeBSD__) || \
        defined(__NetBSD__) || defined(__OpenBSD__)
    XLockDisplay(r->ui->main.dpy);
    #endif
    destroy_widget(r->ui->win, &r->ui->main);
    #if defined(__linux__) || defined(__FreeBSD__) || \
        defined(__NetBSD__) || defined(__OpenBSD__)
    XFlush(r->ui->main.dpy);
    XUnlockDisplay(r->ui->main.dpy);
    #endif
}

int jack_xrun_callback(void *arg) {
    fprintf (stderr, "Xrun \r");
    return 0;
}

int jack_srate_callback(jack_nframes_t samplerate, void* arg) {
    int prio = jack_client_real_time_priority(client);
    if (prio < 0) prio = 25;
    fprintf (stderr, "Samplerate %iHz \n", samplerate);
    r->initEngine(samplerate, prio, 1);
    return 0;
}

int jack_buffersize_callback(jack_nframes_t nframes, void* arg) {
    fprintf (stderr, "Buffersize is %i samples \n", nframes);
    return 0;
}

int jack_process(jack_nframes_t nframes, void *arg) {
    float *input = static_cast<float *>(jack_port_get_buffer (in_port, nframes));
    float *output = static_cast<float *>(jack_port_get_buffer (out_port, nframes));
    
    if(output != input)
        memcpy(output, input, nframes*sizeof(float));

    r->engine.process(nframes, output);

    return 0;
}

void signal_handler (int sig) {
    switch (sig) {
        case SIGINT:
        case SIGHUP:
        case SIGTERM:
        case SIGQUIT:
            fprintf (stderr, "signal %i received, exiting ...\n", sig);
            #if defined(__linux__) || defined(__FreeBSD__) || \
                defined(__NetBSD__) || defined(__OpenBSD__)
            XLockDisplay(r->ui->main.dpy);
            #endif
            destroy_widget(r->ui->win, &r->ui->main);
            #if defined(__linux__) || defined(__FreeBSD__) || \
                defined(__NetBSD__) || defined(__OpenBSD__)
            XFlush(r->ui->main.dpy);
            XUnlockDisplay(r->ui->main.dpy);
            #endif
        break;
        default:
        break;
    }
}

int main (int argc, char *argv[]) {
    #if defined(__linux__) || defined(__FreeBSD__) || \
        defined(__NetBSD__) || defined(__OpenBSD__)
    if(0 == XInitThreads()) 
        fprintf(stderr, "Warning: XInitThreads() failed\n");
    #endif

    r = new Ratatouille();
    r->startGui();

    if ((client = jack_client_open ("ratatouille", JackNoStartServer, NULL)) == 0) {
        fprintf (stderr, "jack server not running?\n");
        destroy_widget(r->ui->win, &r->ui->main);
    }

    signal (SIGQUIT, signal_handler);
    signal (SIGTERM, signal_handler);
    signal (SIGHUP, signal_handler);
    signal (SIGINT, signal_handler);

    if (client) {
        in_port = jack_port_register(
                       client, "in_0", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
        out_port = jack_port_register(
                       client, "out_0", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);

        jack_set_xrun_callback(client, jack_xrun_callback, 0);
        jack_set_sample_rate_callback(client, jack_srate_callback, 0);
        jack_set_buffer_size_callback(client, jack_buffersize_callback, 0);
        jack_set_process_callback(client, jack_process, 0);
        jack_on_shutdown (client, jack_shutdown, 0);

        if (jack_activate (client)) {
            fprintf (stderr, "cannot activate client");
            destroy_widget(r->ui->win, &r->ui->main);
        }

        if (!jack_is_realtime(client)) {
            fprintf (stderr, "jack isn't running with realtime priority\n");
        } else {
            fprintf (stderr, "jack running with realtime priority\n");
        }
        adj_set_value(r->ui->widget[10]->adj, 1.0);
        r->readConfig();
    }

    main_run(&r->ui->main);

    r->cleanup();
    if (client) {
        if (jack_port_connected(in_port)) jack_port_disconnect(client,in_port);
        jack_port_unregister(client,in_port);
        if (jack_port_connected(out_port)) jack_port_disconnect(client,out_port);
        jack_port_unregister(client,out_port);
        jack_client_close (client);
    }
    delete r;

    printf("bye bye\n");
    exit (0);
}
