/*
 * RatatouilleClap.cpp
 *
 * SPDX-License-Identifier:  BSD-3-Clause
 *
 * Copyright (C) 2025 brummer <brummer@web.de>
 */

#include <clap.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "Ratatouille.cc"

#define WINDOW_WIDTH  610
#define WINDOW_HEIGHT 419

#if defined(_WIN32)
#define GUIAPI CLAP_WINDOW_API_WIN32
#else
#define GUIAPI CLAP_WINDOW_API_X11
#endif


/****************************************************************
 ** ratatouille_plugin_t -> the plugin struct
 */

// Plugin data structure
typedef struct {
    clap_plugin_t plugin;
    const clap_host_t *host;
    Ratatouille *r;
    bool guiIsCreated;
    uint32_t latency;
    uint32_t width;
    uint32_t height;
} ratatouille_plugin_t;


/****************************************************************
 ** Parameter handling
 */


static uint32_t params_count(const clap_plugin_t* plugin) {
    ratatouille_plugin_t *plug = (ratatouille_plugin_t *)plugin->plugin_data;
    return (uint32_t)plug->r->param.parameter.size();
}

static bool params_get_info(const clap_plugin_t* plugin, uint32_t param_index, clap_param_info_t* param_info) {
    ratatouille_plugin_t *plug = (ratatouille_plugin_t *)plugin->plugin_data;
    if (param_index >= plug->r->param.parameter.size()) return false;
    const auto& def = plug->r->param.parameter[param_index];
    memset(param_info, 0, sizeof(*param_info));
    param_info->id = def.id;
    strncpy(param_info->name, def.name.c_str(), CLAP_NAME_SIZE-1);
    strncpy(param_info->module, def.group.c_str(), CLAP_PATH_SIZE-1);
    param_info->default_value = def.def;
    param_info->min_value = def.min;
    param_info->max_value = def.max;
    uint32_t flags = CLAP_PARAM_IS_AUTOMATABLE;
    if (def.isStepped) flags |= CLAP_PARAM_IS_STEPPED;
    param_info->flags = flags;
    param_info->cookie = nullptr;
    return true;
}

static bool params_get_value(const clap_plugin_t* plugin, clap_id param_id, double* value) {
    ratatouille_plugin_t *plug = (ratatouille_plugin_t *)plugin->plugin_data;
    if (param_id < 0 || param_id >= plug->r->param.parameter.size()) return false;
    *value = plug->r->param.getParam(param_id);
    return true;
}

static bool params_value_to_text(const clap_plugin_t* plugin, clap_id param_id, double value, char* out, uint32_t size) {
    ratatouille_plugin_t *plug = (ratatouille_plugin_t *)plugin->plugin_data;
    if (param_id < 0 || param_id >= plug->r->param.parameter.size()) return false;
    snprintf(out, size, "%.2f", value);
    return true;
}

static bool params_text_to_value(const clap_plugin_t* plugin, clap_id param_id, const char* text, double* out_value) {
    ratatouille_plugin_t *plug = (ratatouille_plugin_t *)plugin->plugin_data;
    if (param_id < 0 || param_id >= plug->r->param.parameter.size()) return false;
    *out_value = atof(text);
    return true;
}

static void sync_params_to_plug(const clap_plugin_t *plugin, const clap_event_header_t *hdr) {
    ratatouille_plugin_t *plug = (ratatouille_plugin_t *)plugin->plugin_data;
    if (hdr->space_id == CLAP_CORE_EVENT_SPACE_ID) {
        switch (hdr->type) {
            case CLAP_EVENT_PARAM_VALUE: {
                const clap_event_param_value_t *ev = (const clap_event_param_value_t *)hdr;
                plug->r->param.setParam(ev->param_id, ev->value);
                break;
            }
        }
    }
}

static void sync_params_to_host(const clap_plugin_t *plugin, const clap_output_events_t *out) {
    ratatouille_plugin_t *plug = (ratatouille_plugin_t *)plugin->plugin_data;
    for (uint32_t i = 0; i < plug->r->param.parameter.size(); i++) {
        clap_event_param_value_t event = {};
        event.header.size = sizeof(event);
        event.header.time = 0;
        event.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
        event.header.type = CLAP_EVENT_PARAM_VALUE;
        event.header.flags = 0;
        event.param_id = i;
        event.cookie = NULL;
        event.note_id = -1;
        event.port_index = -1;
        event.channel = -1;
        event.key = -1;
        event.value = plug->r->param.getParam(i);
        out->try_push(out, &event.header);
    }
}

static void params_flush(const clap_plugin_t *plugin,
                        const clap_input_events_t *in,
                        const clap_output_events_t *out) {
    ratatouille_plugin_t *plug = (ratatouille_plugin_t *)plugin->plugin_data;
    for (uint32_t i = 0; i < in->size(in); ++i) {
        const clap_event_header_t *ev = in->get(in, i);
        if (ev->type == CLAP_EVENT_PARAM_VALUE) {
            auto *p = (const clap_event_param_value_t *)ev;
            if (p->param_id >= 0 && p->param_id < plug->r->param.parameter.size()) {
                plug->r->param.setParam(p->param_id, p->value);
            }
        }
    }
}

const clap_plugin_params_t ratatouille_params = {
    .count         = params_count,
    .get_info      = params_get_info,
    .get_value     = params_get_value,
    .value_to_text = params_value_to_text,
    .text_to_value = params_text_to_value,
    .flush         = params_flush,
};

/****************************************************************
 ** define the audio ports
 */

// Audio Ports
static uint32_t audio_ports_count(const clap_plugin_t *plugin, bool is_input) {
    if (is_input) return 1; // 1 input
    else return 1; // and 1 output
}

static bool audio_ports_get(const clap_plugin_t *plugin, uint32_t index, bool is_input, clap_audio_port_info_t *info) {
    if (index > 0) return false;
    info->id = index;
    snprintf(info->name, sizeof(info->name), "%s", is_input ? "Input" : "Output");
    if (is_input) {
        info->channel_count = 1; // Mono
        info->port_type = CLAP_PORT_MONO;
    } else {
        info->channel_count = 1; // Mono
        info->port_type = CLAP_PORT_MONO;
    }
    info->flags = CLAP_AUDIO_PORT_IS_MAIN;
    return true;
}

static const clap_plugin_audio_ports_t audio_ports = {
    .count = audio_ports_count,
    .get = audio_ports_get,
};

/****************************************************************
 ** Latency reporting
 */

// Latency
static uint32_t ratatouille_latency_get(const clap_plugin_t *plugin) {
    ratatouille_plugin_t *plug = (ratatouille_plugin_t *)plugin->plugin_data;
    plug->r->getLatency(&plug->latency);
    return plug->latency;
}

static const clap_plugin_latency_t latency_extension = {
    .get = ratatouille_latency_get,
};

/****************************************************************
 ** save and load states
 */

// State Management
static bool ratatouille_state_save(const clap_plugin_t *plugin, const clap_ostream_t *stream) {
    ratatouille_plugin_t *plug = (ratatouille_plugin_t *)plugin->plugin_data;
    std::string state;
    plug->r->saveState(&state);
    stream->write(stream, state.c_str(), strlen(state.c_str()));
    return true;
}

static bool ratatouille_state_load(const clap_plugin_t *plugin, const clap_istream_t *stream) {
    ratatouille_plugin_t *plug = (ratatouille_plugin_t *)plugin->plugin_data;
    char state[2048];
    char *curr = state;
    int thisread = stream->read(stream, curr, 2048);
    if (thisread < 0) return false;
    std::string stream_  = state ;
    plug->r->readState(stream_);
    return true;
}

static const clap_plugin_state_t state_extension = {
    .save = ratatouille_state_save,
    .load = ratatouille_state_load,
};

/****************************************************************
 ** GUI handling
 */

// GUI Callbacks
static bool ratatouille_gui_is_api_supported(const clap_plugin *plugin, const char *api, bool is_floating) {
    return strcmp(api, GUIAPI) == 0;
}

static bool ratatouille_gui_get_preferred_api(const clap_plugin_t *plugin, const char **api, bool *isFloating) {
    *api = GUIAPI;
    *isFloating = false;
    return true;
}

static bool ratatouille_gui_set_scale(const clap_plugin_t *plugin, double scale) {
    ratatouille_plugin_t *plug = (ratatouille_plugin_t *)plugin->plugin_data;
    plug->r->getMain()->hdpi = scale;
    return true;
}

static bool ratatouille_gui_get_size(const clap_plugin_t *plugin, uint32_t *width, uint32_t *height) {
    ratatouille_plugin_t *plug = (ratatouille_plugin_t *)plugin->plugin_data;
    *width = plug->r->TopWin->width;
    *height = plug->r->TopWin->height;
    return true;
}

static bool ratatouille_gui_can_resize(const clap_plugin_t *plugin) {
    return true;
}

static bool ratatouille_gui_get_resize_hints(const clap_plugin_t *plugin, clap_gui_resize_hints_t *hints) {
    return false;
}

static bool ratatouille_gui_adjust_size(const clap_plugin_t *plugin, uint32_t *width, uint32_t *height) {
    ratatouille_plugin_t *plug = (ratatouille_plugin_t *)plugin->plugin_data;
    plug->width = *width;
    plug->height = *height;
    return true;   
}

static bool ratatouille_gui_set_transient(const clap_plugin_t *plugin, const clap_window_t *window) {
    return false;
}

static void ratatouille_gui_suggest_title(const clap_plugin_t *plugin, const char *title) {
    title = "Ratatouille";
}

static bool ratatouille_gui_create(const clap_plugin *plugin, const char *api, bool is_floating) {
    ratatouille_plugin_t *plug = (ratatouille_plugin_t *)plugin->plugin_data;
    if (strcmp(api, GUIAPI) == 0) {
        if (!plug->guiIsCreated) {
            plug->r->startGui();
            plug->r->enableEngine(1);
        }
        plug->guiIsCreated = true;
        return true;
    }
    return false;
}

static void ratatouille_gui_destroy(const clap_plugin *plugin) {
    ratatouille_plugin_t *plug = (ratatouille_plugin_t *)plugin->plugin_data;
    if (plug->guiIsCreated) {
        plug->r->cleanup();
        plug->r->quitMain();
    }
    plug->r->quitGui();
    plug->guiIsCreated = false;
}

static bool ratatouille_gui_show(const clap_plugin *plugin) {
    ratatouille_plugin_t *plug = (ratatouille_plugin_t *)plugin->plugin_data;
    plug->r->showGui();
    return true;
}

static bool ratatouille_gui_hide(const clap_plugin *plugin) {
    ratatouille_plugin_t *plug = (ratatouille_plugin_t *)plugin->plugin_data;
    plug->r->hideGui();
    return true;
}

// embed the GUI
static bool ratatouille_gui_set_parent(const clap_plugin_t *plugin, const clap_window_t *window) {
    ratatouille_plugin_t *plug = (ratatouille_plugin_t *)plugin->plugin_data;
    if (!plug->guiIsCreated) {
        #if defined(_WIN32)
        plug->r->startGui((Window)window->win32);
        #else
        plug->r->startGui(window->x11);
        #endif
        plug->r->enableEngine(1);
    }
    plug->guiIsCreated = true;
    #if defined(_WIN32)
    plug->r->setParent((Window)window->win32);
    #else
    plug->r->setParent(window->x11);
    #endif
    //plug->r->showGui();
    return true;
}

static bool ratatouille_gui_set_size(const clap_plugin_t *plugin, uint32_t width, uint32_t height) {
    ratatouille_plugin_t *plug = (ratatouille_plugin_t *)plugin->plugin_data;
    os_resize_window(plug->r->getMain()->dpy, plug->r->TopWin, width, height);
    return true;
}


// Main thread callback (we run our own main thread)
static void ratatouille_on_main_thread(const clap_plugin_t *plugin) {
   // ratatouille_plugin_t *plug = (ratatouille_plugin_t *)plugin->plugin_data;
   // plug->r->runGui();
}

static const clap_plugin_gui_t extensionGUI = {
    .is_api_supported = ratatouille_gui_is_api_supported,
    .get_preferred_api = ratatouille_gui_get_preferred_api,
    .create = ratatouille_gui_create,
    .destroy = ratatouille_gui_destroy,
    .set_scale = ratatouille_gui_set_scale,
    .get_size = ratatouille_gui_get_size,
    .can_resize = ratatouille_gui_can_resize,
    .get_resize_hints = ratatouille_gui_get_resize_hints,
    .adjust_size = ratatouille_gui_adjust_size,
    .set_size = ratatouille_gui_set_size,
    .set_parent = ratatouille_gui_set_parent,
    .set_transient = ratatouille_gui_set_transient,
    .suggest_title = ratatouille_gui_suggest_title,
    .show = ratatouille_gui_show,
    .hide = ratatouille_gui_hide,
};

/****************************************************************
 ** Plugin handling
 */

// Initialize the plugin
static bool ratatouille_init(const clap_plugin_t *plugin) {
    ratatouille_plugin_t *plug = (ratatouille_plugin_t *)plugin->plugin_data;
    plug->r->initEngine(48000, 25, 1);
    return true;
}

// Destroy the plugin
static void ratatouille_destroy(const clap_plugin_t *plugin) {
    ratatouille_plugin_t *plug = (ratatouille_plugin_t *)plugin->plugin_data;
    ratatouille_gui_destroy(plugin);
    delete plug->r;
    free(plug);
}

// Audio processing
static clap_process_status ratatouille_process(const clap_plugin_t *plugin, const clap_process_t *process) {
    ratatouille_plugin_t *plug = (ratatouille_plugin_t *)plugin->plugin_data;
    // Ensure there is one input and two outputs
    if (process->audio_inputs[0].channel_count < 1 || process->audio_outputs[0].channel_count < 1) {
        return false; // Invalid format
    }

    float *input = process->audio_inputs[0].data32[0]; // Mono input channel
    float *output = process->audio_outputs[0].data32[0]; // Mono output
    uint32_t nframes = process->frames_count;
    const uint32_t nev = process->in_events->size(process->in_events);
    uint32_t ev_index = 0;
    uint32_t next_ev_frame = nev > 0 ? 0 : nframes;

    if (plug->r->param.controllerChanged.load(std::memory_order_acquire)) {
        sync_params_to_host(plugin, process->out_events);
        plug->r->param.controllerChanged.store(false, std::memory_order_release);
    }

    for (uint32_t i = 0; i < nframes;++i) {
        while (ev_index < nev && next_ev_frame == i) {
            const clap_event_header_t *hdr = process->in_events->get(process->in_events, ev_index);
            if (hdr->time != i) {
                next_ev_frame = hdr->time;
                break;
            }
            sync_params_to_plug(plugin, hdr);
            ++ev_index;

            if (ev_index == nev) {
                // we reached the end of the event list
                next_ev_frame = nframes;
                break;
            }
        }
    }

    // in-place processing
    if(output != input)
        memcpy(output, input, nframes*sizeof(float));
    
    plug->r->process(nframes, output);
    return CLAP_PROCESS_CONTINUE;
}


// Finally get the sample rate and re-init the engine
static bool ratatouille_activate(const struct clap_plugin *plugin,
                             double                    sample_rate,
                             uint32_t                  min_frames_count,
                             uint32_t                  max_frames_count) {
    ratatouille_plugin_t *plug = (ratatouille_plugin_t *)plugin->plugin_data;
    //if (sample_rate != 48000) 
    plug->r->initEngine(sample_rate, 25, 1);
    return true;
}

static void ratatouille_deactivate(const struct clap_plugin *plugin) {}

static bool ratatouille_start_processing(const struct clap_plugin *plugin) { return true; }

static void ratatouille_stop_processing(const struct clap_plugin *plugin) {}

static void ratatouille_reset(const struct clap_plugin *plugin) {}

// CLAP plugin descriptor
static const clap_plugin_descriptor_t ratatouille_descriptor = {
    .clap_version = CLAP_VERSION_INIT,
    .id = "com.brummer10.Ratatouille",
    .name = "Ratatouille",
    .vendor = "brummer10",
    .url = "https://github.com/brummer10/Ratatouille",
    .manual_url = "https://github.com/brummer10/Ratatouille",
    .support_url = "https://github.com/brummer10/Ratatouille",
    .version = "0.9.11",
    .description = "CLAP plugin wrapper for Ratatouille",
    .features = (const char *[]){ CLAP_PLUGIN_FEATURE_AUDIO_EFFECT, NULL },
};

// Extensions
static const void *ratatouille_get_extension(const clap_plugin_t *plugin, const char *id) {
    if (!strcmp(id, CLAP_EXT_AUDIO_PORTS)) return &audio_ports;
    if (!strcmp(id, CLAP_EXT_LATENCY)) return &latency_extension;
    if (!strcmp(id, CLAP_EXT_GUI)) return &extensionGUI;
    if (!strcmp(id, CLAP_EXT_PARAMS)) return &ratatouille_params;
    if (!strcmp(id, CLAP_EXT_STATE)) return &state_extension;
    return NULL;
}

// Create the plugin
static const clap_plugin_t *ratatouille_create(const clap_host_t *host) {
    ratatouille_plugin_t *plug = (ratatouille_plugin_t *)calloc(1, sizeof(ratatouille_plugin_t));
    if (!plug) return NULL;
    plug->r = new Ratatouille();
    plug->guiIsCreated = false;
    plug->width = WINDOW_WIDTH;
    plug->height = WINDOW_HEIGHT;
    plug->plugin.desc = &ratatouille_descriptor;
    plug->plugin.plugin_data = plug;
    plug->plugin.init = ratatouille_init;
    plug->plugin.destroy = ratatouille_destroy;
    plug->plugin.activate = ratatouille_activate;
    plug->plugin.deactivate = ratatouille_deactivate;
    plug->plugin.start_processing = ratatouille_start_processing;
    plug->plugin.stop_processing = ratatouille_stop_processing;
    plug->plugin.reset = ratatouille_reset;
    plug->plugin.process = ratatouille_process;
    plug->plugin.get_extension = ratatouille_get_extension;
    plug->plugin.on_main_thread = ratatouille_on_main_thread;
    plug->host = host;
    return &plug->plugin;
}


/****************************************************************
 ** the factory entry
 */

static uint32_t plugin_factory_get_plugin_count(const struct clap_plugin_factory *factory) {
   return 1;
}

static const clap_plugin_descriptor_t *plugin_factory_get_ratatouille_descriptor
                    (const struct clap_plugin_factory *factory, uint32_t index) {
   return  &ratatouille_descriptor; //s_plugins[index].desc;
}

static const clap_plugin_t *plugin_factory_create_ratatouille
                        (const struct clap_plugin_factory *factory,
                        const clap_host_t *host, const char *plugin_id) {

   if (!clap_version_is_compatible(host->clap_version)) {
      return NULL;
   }
   return ratatouille_create(host);
}

static const clap_plugin_factory_t plugin_factory = {
    .get_plugin_count = plugin_factory_get_plugin_count,
    .get_plugin_descriptor = plugin_factory_get_ratatouille_descriptor,
    .create_plugin = plugin_factory_create_ratatouille,
};

static const void *entry_get_factory(const char *factory_id) {
    return &plugin_factory;
}

static bool entry_init(const char *plugin_path) {
   // perform the plugin initialization
   return true;
}

static void entry_deinit(void) {
   // perform the plugin de-initialization
}

/****************************************************************
 ** Finally the CLAP plugin entry export
 */

extern "C" const clap_plugin_entry_t clap_entry = {
    .clap_version = CLAP_VERSION_INIT,
    .init = entry_init,
    .deinit = entry_deinit,
    .get_factory = entry_get_factory,
};
