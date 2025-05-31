/*
 * Ratatouille.c
 *
 * SPDX-License-Identifier:  BSD-3-Clause
 *
 * Copyright (C) 2024 brummer <brummer@web.de>
 */

#ifdef STANDALONE
#include "standalone.h"
#elif defined(CLAPPLUG)
#include "clapplug.h"
#else
#include "lv2_plugin.cc"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include "widgets.cc"

void set_custom_theme(X11_UI *ui) {
    ui->main.color_scheme->normal = (Colors) {
         /* cairo    / r  / g  / b  / a  /  */
        .fg =       { 0.686, 0.729, 0.773, 1.000},
        .bg =       { 0.129, 0.149, 0.180, 1.000},
        .base =     { 0.000, 0.000, 0.000, 1.000},
        .text =     { 0.686, 0.729, 0.773, 1.000},
        .shadow =   { 0.000, 0.000, 0.000, 0.200},
        .frame =    { 0.000, 0.000, 0.000, 1.000},
        .light =    { 0.100, 0.100, 0.100, 1.000}
    };

    ui->main.color_scheme->prelight = (Colors) {
         /* cairo    / r  / g  / b  / a  /  */
        .fg =       { 1.000, 0.000, 1.000, 1.000},
        .bg =       { 0.250, 0.250, 0.250, 1.000},
        .base =     { 0.300, 0.300, 0.300, 1.000},
        .text =     { 1.000, 1.000, 1.000, 1.000},
        .shadow =   { 0.100, 0.100, 0.100, 0.400},
        .frame =    { 0.300, 0.300, 0.300, 1.000},
        .light =    { 0.300, 0.300, 0.300, 1.000}
    };

    ui->main.color_scheme->selected = (Colors) {
         /* cairo    / r  / g  / b  / a  /  */
        .fg =       { 0.900, 0.900, 0.900, 1.000},
        .bg =       { 0.176, 0.200, 0.231, 1.000},
        .base =     { 0.500, 0.180, 0.180, 1.000},
        .text =     { 1.000, 1.000, 1.000, 1.000},
        .shadow =   { 0.800, 0.180, 0.180, 0.200},
        .frame =    { 0.500, 0.180, 0.180, 1.000},
        .light =    { 0.500, 0.180, 0.180, 1.000}
    };

    ui->main.color_scheme->active = (Colors) {
         /* cairo    / r  / g  / b  / a  /  */
        .fg =       { 0.000, 1.000, 1.000, 1.000},
        .bg =       { 0.000, 0.000, 0.000, 1.000},
        .base =     { 0.180, 0.380, 0.380, 1.000},
        .text =     { 0.750, 0.750, 0.750, 1.000},
        .shadow =   { 0.180, 0.380, 0.380, 0.500},
        .frame =    { 0.180, 0.380, 0.380, 1.000},
        .light =    { 0.180, 0.380, 0.380, 1.000}
    };

    ui->main.color_scheme->insensitive = (Colors) {
         /* cairo    / r  / g  / b  / a  /  */
        .fg =       { 0.850, 0.850, 0.850, 0.500},
        .bg =       { 0.100, 0.100, 0.100, 0.500},
        .base =     { 0.000, 0.000, 0.000, 0.500},
        .text =     { 0.900, 0.900, 0.900, 0.500},
        .shadow =   { 0.000, 0.000, 0.000, 0.100},
        .frame =    { 0.000, 0.000, 0.000, 0.500},
        .light =    { 0.100, 0.100, 0.100, 0.500}
    };
}

static void file_load_response(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    ModelPicker* m = (ModelPicker*) w->parent_struct;
    Widget_t *p = (Widget_t*)w->parent;
    X11_UI *ui = (X11_UI*) p->parent_struct;
    if(user_data !=NULL) {
        int old = 0;
        if (ends_with(m->filename, "nam") ||
                ends_with(m->filename, "json") ||
                ends_with(m->filename, "aidax")) {
            old = 1;
        } else if (ends_with(m->filename, "wav")) {
            old = 2;
        }
        free(m->filename);
        m->filename = NULL;
        m->filename = strdup(*(const char**)user_data);

        sendFileName(ui, m, old);

        free(m->filename);
        m->filename = NULL;
        m->filename = strdup("None");
        expose_widget(ui->win);
        ui->loop_counter = 12;
    }
}

void set_ctl_val_from_host(Widget_t *w, float value) {
    xevfunc store = w->func.value_changed_callback;
    w->func.value_changed_callback = dummy_callback;
    adj_set_value(w->adj, value);
    w->func.value_changed_callback = *(*store);
}

static void file_menu_callback(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    ModelPicker* m = (ModelPicker*) w->parent_struct;
    Widget_t *p = (Widget_t*)w->parent;
    X11_UI *ui = (X11_UI*) p->parent_struct;
    X11_UI_Private_t *ps = (X11_UI_Private_t*)ui->private_ptr;
    if (!m->filepicker->file_counter) return;
    int v = (int)adj_get_value(w->adj);
    if (v >= m->filepicker->file_counter) {
        free(ps->fname);
        ps->fname = NULL;
        asprintf(&ps->fname, "%s", "None");
    } else {
        free(ps->fname);
        ps->fname = NULL;
        asprintf(&ps->fname, "%s%s%s", m->dir_name, PATH_SEPARATOR, m->filepicker->file_names[v]);
    }
    file_load_response(m->filebutton, (void*)&ps->fname);
}

void plugin_set_window_size(int *w,int *h,const char * plugin_uri) {
    (*w) = 610; //set initial width of main window
    (*h) = 419; //set initial height of main window
}

const char* plugin_set_name() {
    return "Ratatouille"; //set plugin name to display on UI
}

void plugin_create_controller_widgets(X11_UI *ui, const char * plugin_uri) {

    ui->win->label = plugin_set_name();
    widget_get_png(ui->win, LDVAR(texture_png));
    // connect the expose func
    ui->win->func.expose_callback = draw_window;
    ui->win->func.key_press_callback = set_precision;
    ui->win->func.key_release_callback = reset_precision;

    X11_UI_Private_t *ps =(X11_UI_Private_t*)malloc(sizeof(X11_UI_Private_t));
    ui->private_ptr = (void*)ps;
    ps->ma.filename = strdup("None");
    ps->mb.filename = strdup("None");
    ps->ir.filename = strdup("None");
    ps->ir1.filename = strdup("None");
    ps->ma.dir_name = NULL;
    ps->mb.dir_name = NULL;
    ps->ir.dir_name = NULL;
    ps->ir1.dir_name = NULL;
    ps->fname = NULL;
    ps->ma.filepicker = (FilePicker*)malloc(sizeof(FilePicker));
    fp_init(ps->ma.filepicker, "/");
    asprintf(&ps->ma.filepicker->filter ,"%s", ".nam|.aidax|.json");
    ps->ma.filepicker->use_filter = 1;
    ps->mb.filepicker = (FilePicker*)malloc(sizeof(FilePicker));
    fp_init(ps->mb.filepicker, "/");
    asprintf(&ps->mb.filepicker->filter ,"%s", ".nam|.aidax|.json");
    ps->mb.filepicker->use_filter = 1;
    ps->ir.filepicker = (FilePicker*)malloc(sizeof(FilePicker));
    fp_init(ps->ir.filepicker, "/");
    asprintf(&ps->ir.filepicker->filter ,"%s", ".wav");
    ps->ir.filepicker->use_filter = 1;
    ps->ir1.filepicker = (FilePicker*)malloc(sizeof(FilePicker));
    fp_init(ps->ir1.filepicker, "/");
    asprintf(&ps->ir1.filepicker->filter ,"%s", ".wav");
    ps->ir1.filepicker->use_filter = 1;

    ui->widget[15] = add_lv2_switch (ui->widget[15], ui->win, 20, "Buffer", ui, 50,  22, 30, 30);

    ui->widget[17] = add_lv2_label (ui->widget[17], ui->win, 22, "Latency", ui, 115,  22, 130, 30);
    ui->widget[18] = add_lv2_label (ui->widget[18], ui->win, 23, "Xrun", ui, 510,  205, 100, 30);

    ui->widget[16] = add_lv2_switch (ui->widget[16], ui->win, 21, "Phase", ui, 90,  22, 30, 30);
    ui->widget[10] = add_lv2_switch (ui->widget[10], ui->win, 14, "", ui, 505,  22, 50, 50);

    ps->ma.filebutton = add_lv2_file_button (ps->ma.filebutton, ui->win, -1, "Neural Model", ui, 40,  248, 25, 25);
    ps->ma.filebutton->parent_struct = (void*)&ps->ma;
    ps->ma.filebutton->func.user_callback = file_load_response;

    ps->mb.filebutton = add_lv2_file_button (ps->mb.filebutton, ui->win, -2, "Neural Model", ui, 40,  288, 25, 25);
    ps->mb.filebutton->parent_struct = (void*)&ps->mb;
    ps->mb.filebutton->func.user_callback = file_load_response;

    ps->ir.filebutton = add_lv2_irfile_button (ps->ir.filebutton, ui->win, -3, "IR File", ui, 40,  328, 25, 25);
    ps->ir.filebutton->parent_struct = (void*)&ps->ir;
    ps->ir.filebutton->func.user_callback = file_load_response;

    ps->ir1.filebutton = add_lv2_irfile_button (ps->ir1.filebutton, ui->win, -4, "IR File", ui, 40,  368, 25, 25);
    ps->ir1.filebutton->parent_struct = (void*)&ps->ir1;
    ps->ir1.filebutton->func.user_callback = file_load_response;

    ui->widget[0] = add_lv2_knob (ui->widget[0], ui->win, 2, "Input(A)", ui, 35,  90, 90, 110);
    set_adjustment(ui->widget[0]->adj, 0.0, 0.0, -20.0, 20.0, 0.2, CL_CONTINUOS);
    set_widget_color(ui->widget[0], (Color_state)0, (Color_mod)0, 0.259, 0.518, 0.894, 1.0);
    set_widget_color(ui->widget[0], (Color_state)0, (Color_mod)3,  0.686, 0.729, 0.773, 1.0);

    ui->widget[7] = add_lv2_knob (ui->widget[7], ui->win, 11, "Input(B)", ui, 125,  90, 90, 110);
    set_adjustment(ui->widget[7]->adj, 0.0, 0.0, -20.0, 20.0, 0.2, CL_CONTINUOS);
    set_widget_color(ui->widget[7], (Color_state)0, (Color_mod)0, 0.259, 0.518, 0.894, 1.0);
    set_widget_color(ui->widget[7], (Color_state)0, (Color_mod)3,  0.686, 0.729, 0.773, 1.0);

    ui->widget[2] = add_lv2_knob (ui->widget[2], ui->win, 4, "Blend(A|B)", ui, 215,  90, 90, 110);
    set_adjustment(ui->widget[2]->adj, 0.5, 0.5, 0.0, 1.0, 0.01, CL_CONTINUOS);
    set_widget_color(ui->widget[2], (Color_state)0, (Color_mod)0, 0.259, 0.518, 0.894, 1.0);
    set_widget_color(ui->widget[2], (Color_state)0, (Color_mod)3,  0.686, 0.729, 0.773, 1.0);

    ui->widget[4] = add_lv2_knob (ui->widget[4], ui->win, 8, "Delay(Δ)", ui, 305,  90, 90, 110);
    set_adjustment(ui->widget[4]->adj, 0.0, 0.0, -4096.0, 4096.0, 16.0, CL_CONTINUOS);
    set_widget_color(ui->widget[4], (Color_state)0, (Color_mod)0, 0.259, 0.518, 0.894, 1.0);
    set_widget_color(ui->widget[4], (Color_state)0, (Color_mod)3,  0.686, 0.729, 0.773, 1.0);

    ui->widget[3] = add_lv2_knob (ui->widget[3], ui->win, 7, "Mix (IR)", ui, 395,  90, 90, 110);
    set_adjustment(ui->widget[3]->adj, 0.5, 0.5, 0.0, 1.0, 0.01, CL_CONTINUOS);
    set_widget_color(ui->widget[3], (Color_state)0, (Color_mod)0, 0.259, 0.518, 0.894, 1.0);
    set_widget_color(ui->widget[3], (Color_state)0, (Color_mod)3,  0.686, 0.729, 0.773, 1.0);

    ui->widget[1] = add_lv2_knob (ui->widget[1], ui->win, 3, "Output ", ui, 485,  90, 90, 110);
    set_adjustment(ui->widget[1]->adj, 0.0, 0.0, -20.0, 20.0, 0.2, CL_CONTINUOS);
    set_widget_color(ui->widget[1], (Color_state)0, (Color_mod)0, 0.259, 0.518, 0.894, 1.0);
    set_widget_color(ui->widget[1], (Color_state)0, (Color_mod)3,  0.686, 0.729, 0.773, 1.0);

    ps->ma.fbutton = add_lv2_button(ps->ma.fbutton, ui->win, "", ui, 515,  244, 22, 30);
    ps->ma.fbutton->parent_struct = (void*)&ps->ma;
    combobox_set_pop_position(ps->ma.fbutton, 0);
    combobox_set_entry_length(ps->ma.fbutton, 60);
    combobox_add_entry(ps->ma.fbutton, "None");
    ps->ma.fbutton->func.value_changed_callback = file_menu_callback;

    ps->mb.fbutton = add_lv2_button(ps->mb.fbutton, ui->win, "", ui, 515,  284, 22, 30);
    ps->mb.fbutton->parent_struct = (void*)&ps->mb;
    combobox_set_pop_position(ps->mb.fbutton, 0);
    combobox_set_entry_length(ps->mb.fbutton, 60);
    combobox_add_entry(ps->mb.fbutton, "None");
    ps->mb.fbutton->func.value_changed_callback = file_menu_callback;

    ui->widget[8] = add_lv2_toggle_button (ui->widget[8], ui->win, 12, "", ui, 70,  248, 25, 25);
    ui->widget[9] = add_lv2_toggle_button (ui->widget[9], ui->win, 13, "", ui, 70,  288, 25, 25);

    ui->widget[11] = add_lv2_erase_button (ui->widget[11], ui->win, 15, "", ui, 545,  248, 25, 25);
    ui->widget[12] = add_lv2_erase_button (ui->widget[12], ui->win, 16, "", ui, 545,  288, 25, 25);

    ps->ir.fbutton = add_lv2_button(ps->ir.fbutton, ui->win, "", ui, 515,  324, 22, 30);
    ps->ir.fbutton->parent_struct = (void*)&ps->ir;
    combobox_set_pop_position(ps->ir.fbutton, 0);
    combobox_set_entry_length(ps->ir.fbutton, 60);
    combobox_add_entry(ps->ir.fbutton, "None");
    ps->ir.fbutton->func.value_changed_callback = file_menu_callback;

    ps->ir1.fbutton = add_lv2_button(ps->ir1.fbutton, ui->win, "", ui, 515,  364, 22, 30);
    ps->ir1.fbutton->parent_struct = (void*)&ps->ir1;
    combobox_set_pop_position(ps->ir1.fbutton, 0);
    combobox_set_entry_length(ps->ir1.fbutton, 60);
    combobox_add_entry(ps->ir1.fbutton, "None");
    ps->ir1.fbutton->func.value_changed_callback = file_menu_callback;

    ui->widget[5] = add_lv2_toggle_button (ui->widget[5], ui->win, 9, "", ui, 70,  328, 25, 25);
    ui->widget[6] = add_lv2_toggle_button (ui->widget[6], ui->win, 10, "", ui, 70,  368, 25, 25);

    ui->widget[13] = add_lv2_erase_button (ui->widget[13], ui->win, 17, "", ui, 545,  328, 25, 25);
    ui->widget[14] = add_lv2_erase_button (ui->widget[14], ui->win, 18, "", ui, 545,  368, 25, 25);
}

void plugin_cleanup(X11_UI *ui) {
    X11_UI_Private_t *ps = (X11_UI_Private_t*)ui->private_ptr;
    free(ps->fname);
    free(ps->ma.filename);
    free(ps->mb.filename);
    free(ps->ma.dir_name);
    free(ps->mb.dir_name);
    free(ps->ir.filename);
    free(ps->ir.dir_name);
    free(ps->ir1.filename);
    free(ps->ir1.dir_name);
    fp_free(ps->ma.filepicker);
    free(ps->ma.filepicker);
    fp_free(ps->mb.filepicker);
    free(ps->mb.filepicker);
    fp_free(ps->ir.filepicker);
    free(ps->ir.filepicker);
    fp_free(ps->ir1.filepicker);
    free(ps->ir1.filepicker);
    // clean up used sources when needed
}


#ifdef __cplusplus
}
#endif
