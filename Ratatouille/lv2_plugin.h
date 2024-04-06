/*
 *                           0BSD 
 * 
 *                    BSD Zero Clause License
 * 
 *  Copyright (c) 2019 Hermann Meyer
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted.

 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 */


#include <lv2/core/lv2.h>
#include <lv2/ui/ui.h>
#include <lv2/state/state.h>
#include <lv2/worker/worker.h>
#include <lv2/atom/atom.h>
#include <lv2/options/options.h>
#include <lv2/parameters/parameters.h>
#if defined USE_ATOM || defined USE_MIDI
#include <lv2/atom/util.h>
#include <lv2/atom/forge.h>
#include <lv2/midi/midi.h>
#include <lv2/urid/urid.h>
#include <lv2/patch/patch.h>
#endif

#ifndef LV2_UI__scaleFactor
#define LV2_UI__scaleFactor  "http://lv2plug.in/ns/extensions/ui#scaleFactor"
#endif

// xwidgets.h includes xputty.h and all defined widgets from Xputty
#include "xwidgets.h"
#include "xfile-dialog.h"

#pragma once

#ifndef LV2_PLUGIN_H_
#define LV2_PLUGIN_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef int PortIndex;

// main window struct
typedef struct {
    void *parentXwindow;
    Xputty main;
    Widget_t *win;
    Widget_t *widget[CONTROLS];
    Widget_t *elem[GUI_ELEMENTS];
    Widget_t *tab_elem[TAB_ELEMENTS];
    unsigned int f_index;
    void *private_ptr;
    int need_resize;
    int loop_counter;
    bool uiKnowSampleRate;
    int uiSampleRate;
    LV2_URID_Map* map;
    void *controller;
    LV2UI_Write_Function write_function;
    LV2UI_Resize* resize;
} X11_UI;

// inform the engine that the GUI is loaded
void first_loop(X11_UI *ui);

// controller value changed, forward value to host when needed
void plugin_value_changed(X11_UI *ui, Widget_t *w, PortIndex index);

// set the plugin initial window size
void plugin_set_window_size(int *w,int *h,const char * plugin_uri);

// set the plugin name
const char* plugin_set_name();

// create all needed controller 
void plugin_create_controller_widgets(X11_UI *ui, const char * plugin_uri);

Widget_t* add_lv2_button(Widget_t *w, Widget_t *p, const char * label,
                                X11_UI* ui, int x, int y, int width, int height);

Widget_t* add_lv2_knob(Widget_t *w, Widget_t *p, PortIndex index, const char * label,
                                X11_UI* ui, int x, int y, int width, int height);

Widget_t* add_lv2_file_button(Widget_t *w, Widget_t *p, PortIndex index, const char * label,
                                X11_UI* ui, int x, int y, int width, int height);

// free used mem on exit
void plugin_cleanup(X11_UI *ui);

// controller value changed message from host
void plugin_port_event(LV2UI_Handle handle, uint32_t port_index,
                        uint32_t buffer_size, uint32_t format,
                        const void * buffer);
#ifdef __cplusplus
}
#endif

#endif //LV2_PLUGIN_H_
