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

#include "engine.h"
#include "ParallelThread.h"
#define STANDALONE
#include "Ratatouille.c"

X11_UI* ui;
ParallelThread fetch;
ratatouille::Engine *engine;


jack_client_t *client;
jack_port_t *in_port;
jack_port_t *out_port;
double s_time;

// clean up when done
static void cleanup(X11_UI* ui) {
    plugin_cleanup(ui);
    // Xputty free all memory used
    main_quit(&ui->main);
    free(ui->private_ptr);
    free(ui);
}

// send value changes to the engine
void sendValueChanged(X11_UI *ui, int port, float value) {
    switch (port) {
        case 2:
            engine->inputGain = value;
        break;
        case 3:
            engine->outputGain = value;
        break;
        case 4:
            engine->blend = value;
        break;
        // 5 + 6 atom ports
        case 7:
            engine->mix = value;
        break;
        case 8:
            engine->delay = value;
            engine->cdelay->connect(8,(void*) &value);
        break;
        case 9:
        {
            engine->_ab.store(7, std::memory_order_release);
            engine->conv.set_normalisation(static_cast<uint32_t>(value));
            if (engine->ir_file.compare("None") != 0) {
                engine->_execute.store(true, std::memory_order_release);
                engine->xrworker.runProcess();
            }
        }
        break;
        case 10:
        {
            engine->_ab.store(8, std::memory_order_release);
            engine->conv1.set_normalisation(static_cast<uint32_t>(value));
            if (engine->ir_file1.compare("None") != 0) {
                engine->_execute.store(true, std::memory_order_release);
                engine->xrworker.runProcess();
            }
        }
        break;
        case 11:
            engine->inputGain1 = value;
        break;
        case 12:
            engine->normSlotA = static_cast<int32_t>(value);
        break;
        case 13:
            engine->normSlotB = static_cast<int32_t>(value);
        break;
        case 14:
            engine->bypass = static_cast<int32_t>(value);
        break;
        case 15:
        {
            engine->_ab.store(1, std::memory_order_release);
            engine->model_file = "None";
            engine->_execute.store(true, std::memory_order_release);
            engine->xrworker.runProcess();
        }
        break;
        case 16:
        {
            engine->_ab.store(2, std::memory_order_release);
            engine->model_file1 = "None";
            engine->_execute.store(true, std::memory_order_release);
            engine->xrworker.runProcess();
         }
        break;
        case 17:
        {
            engine->_ab.store(7, std::memory_order_release);
            engine->ir_file = "None";
            engine->_execute.store(true, std::memory_order_release);
            engine->xrworker.runProcess();
        }
        break;
        case 18:
        {
            engine->_ab.store(8, std::memory_order_release);
            engine->ir_file1 = "None";
            engine->_execute.store(true, std::memory_order_release);
            engine->xrworker.runProcess();
        }
        break;
        // 19 latency
        case 20:
        {
            engine->buffered = value;
        }
        break;
        case 21:
        {
            engine->phasecor_ = value;
        }
        break;
        default:
        break;
    }
}

// send a file name to the engine
void sendFileName(X11_UI *ui, ModelPicker* m, int old){
    X11_UI_Private_t *ps = (X11_UI_Private_t*)ui->private_ptr;
    if ((strcmp(m->filename, "None") == 0)) {
        if (old == 1) {
            if ( m == &ps->ma) {
                engine->model_file = m->filename;
                engine->_ab.store(1, std::memory_order_release);
            } else {
                engine->model_file1 = m->filename;
                engine->_ab.store(2, std::memory_order_release);
            }
        } else if (old == 2) {
            if ( m == &ps->ir) {
                engine->ir_file = m->filename;
                engine->_ab.store(7, std::memory_order_release);
            } else {
                engine->ir_file1 = m->filename;
                engine->_ab.store(8, std::memory_order_release);
            }
        } else return;
    } else if (ends_with(m->filename, "nam") ||
               ends_with(m->filename, "json") ||
               ends_with(m->filename, "aidax")) {
        if ( m == &ps->ma) {
            engine->model_file = m->filename;
            engine->_ab.store(1, std::memory_order_release);
        } else {
            engine->model_file1 = m->filename;
            engine->_ab.store(2, std::memory_order_release);
        }
    } else if (ends_with(m->filename, "wav")) {
        if ( m == &ps->ir) {
            engine->ir_file = m->filename;
            engine->_ab.store(7, std::memory_order_release);
        } else {
            engine->ir_file1 = m->filename;
            engine->_ab.store(8, std::memory_order_release);
        }
    } else return;
    engine->_execute.store(true, std::memory_order_release);
    engine->xrworker.runProcess();
}

// rebuild file menu when needed
static void rebuild_file_menu(ModelPicker *m) {
    xevfunc store = m->fbutton->func.value_changed_callback;
    m->fbutton->func.value_changed_callback = dummy_callback;
    combobox_delete_entrys(m->fbutton);
    fp_get_files(m->filepicker, m->dir_name, 0, 1);
    int active_entry = m->filepicker->file_counter-1;
    uint i = 0;
    for(;i<m->filepicker->file_counter;i++) {
        combobox_add_entry(m->fbutton, m->filepicker->file_names[i]);
        if (strcmp(basename(m->filename),m->filepicker->file_names[i]) == 0) 
            active_entry = i;
    }
    combobox_add_entry(m->fbutton, "None");
    adj_set_value(m->fbutton->adj, active_entry);
    combobox_set_menu_size(m->fbutton, min(14, m->filepicker->file_counter+1));
    m->fbutton->func.value_changed_callback = store;
}

// confirmation from engine that a file is loaded
static inline void get_file(std::string fileName, ModelPicker *m) {
    if (!fileName.empty() && (fileName.compare("None") != 0)) {
        const char* uri = fileName.c_str();
        if (strcmp(uri, (const char*)m->filename) !=0) {
            free(m->filename);
            m->filename = NULL;
            m->filename = strdup(uri);
            char *dn = strdup(dirname((char*)uri));
            if (m->dir_name == NULL || strcmp((const char*)m->dir_name,
                                                    (const char*)dn) !=0) {
                free(m->dir_name);
                m->dir_name = NULL;
                m->dir_name = strdup(dn);
                FileButton *filebutton = (FileButton*)m->filebutton->private_struct;
                filebutton->path = m->dir_name;
                rebuild_file_menu(m);
            }
            free(dn);
            expose_widget(ui->win);
        }
    } else if (strcmp(m->filename, "None") != 0) {
        free(m->filename);
        m->filename = NULL;
        m->filename = strdup("None");
        expose_widget(ui->win);
    }
}

// timeout loop to check output ports from engine
void checkEngine() {
    #if defined(__linux__) || defined(__FreeBSD__) || \
        defined(__NetBSD__) || defined(__OpenBSD__)
    XLockDisplay(ui->main.dpy);
    #endif
    if (engine->_notify_ui.load(std::memory_order_acquire)) {
        engine->_notify_ui.store(false, std::memory_order_release);
        X11_UI_Private_t *ps = (X11_UI_Private_t*)ui->private_ptr;
        get_file(engine->model_file, &ps->ma);
        get_file(engine->model_file1, &ps->mb);
        get_file(engine->ir_file, &ps->ir);
        get_file(engine->ir_file1, &ps->ir1);
    }
    adj_set_value(ui->widget[17]->adj,(float) engine->latency * s_time);
    #if defined(__linux__) || defined(__FreeBSD__) || \
        defined(__NetBSD__) || defined(__OpenBSD__)
    XFlush(ui->main.dpy);
    XUnlockDisplay(ui->main.dpy);
    #endif
}

void jack_shutdown (void *arg) {
    fprintf (stderr, "jack shutdown, exit now \n");
    #if defined(__linux__) || defined(__FreeBSD__) || \
        defined(__NetBSD__) || defined(__OpenBSD__)
    XLockDisplay(ui->main.dpy);
    #endif
    destroy_widget(ui->win, &ui->main);
    #if defined(__linux__) || defined(__FreeBSD__) || \
        defined(__NetBSD__) || defined(__OpenBSD__)
    XFlush(ui->main.dpy);
    XUnlockDisplay(ui->main.dpy);
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
    engine->init(samplerate, prio, 1);
    s_time = (1.0 / (double)samplerate) * 1000;
    fetch.set<&checkEngine>();
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

    engine->process(nframes, output);

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
            XLockDisplay(ui->main.dpy);
            #endif
            destroy_widget(ui->win, &ui->main);
            #if defined(__linux__) || defined(__FreeBSD__) || \
                defined(__NetBSD__) || defined(__OpenBSD__)
            XFlush(ui->main.dpy);
            XUnlockDisplay(ui->main.dpy);
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
    engine = new ratatouille::Engine();
    float v = 0.0;
    s_time = 0.0;
    engine->cdelay->connect(8,(void*) &v);
    ui = (X11_UI*)malloc(sizeof(X11_UI));
    ui->private_ptr = NULL;
    ui->need_resize = 1;
    ui->loop_counter = 4;
    ui->uiKnowSampleRate = false;
    ui->uiSampleRate = 0;
    ui->f_index = 0;

    int i = 0;
    for(;i<CONTROLS;i++)
        ui->widget[i] = NULL;
    i = 0;
    for(;i<GUI_ELEMENTS;i++)
        ui->elem[i] = NULL;

    main_init(&ui->main);
    set_custom_theme(ui);
    ui->win = create_window(&ui->main, os_get_root_window(&ui->main, IS_WINDOW), 0, 0, 610, 419);
    widget_set_title(ui->win, "Ratatouille");
    widget_set_icon_from_png(ui->win,LDVAR(neuraldir_png));
    ui->win->parent_struct = ui;
    plugin_create_controller_widgets(ui,"standalone");
    widget_show_all(ui->win);
    fetch.startTimeout(120);

    if ((client = jack_client_open ("ratatouille", JackNoStartServer, NULL)) == 0) {
        fprintf (stderr, "jack server not running?\n");
        destroy_widget(ui->win, &ui->main);
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
            destroy_widget(ui->win, &ui->main);
        }

        if (!jack_is_realtime(client)) {
            fprintf (stderr, "jack isn't running with realtime priority\n");
        } else {
            fprintf (stderr, "jack running with realtime priority\n");
        }
        adj_set_value(ui->widget[10]->adj, 1.0);
    }

    main_run(&ui->main);

    cleanup(ui);
    fetch.stop();
    if (client) {
        if (jack_port_connected(in_port)) jack_port_disconnect(client,in_port);
        jack_port_unregister(client,in_port);
        if (jack_port_connected(out_port)) jack_port_disconnect(client,out_port);
        jack_port_unregister(client,out_port);
        jack_client_close (client);
    }
    delete engine;

    printf("bye bye\n");
    exit (0);
}
