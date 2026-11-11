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
        .bg =       { 0.122, 0.137, 0.157, 1.000},
        .base =     { 0.078, 0.090, 0.106, 1.000},
        .text =     { 0.686, 0.729, 0.773, 1.000},
        .shadow =   { 0.000, 0.000, 0.000, 0.200},
        .frame =    { 0.173, 0.196, 0.227, 1.000},
        .light =    { 0.173, 0.196, 0.227, 1.000}
    };

    ui->main.color_scheme->prelight = (Colors) {
         /* cairo    / r  / g  / b  / a  /  */
        .fg =       { 0.259, 0.518, 0.894, 1.000},
        .bg =       { 0.173, 0.196, 0.227, 1.000},
        .base =     { 0.235, 0.263, 0.302, 1.000},
        .text =     { 0.843, 0.871, 0.902, 1.000},
        .shadow =   { 0.100, 0.100, 0.100, 0.400},
        .frame =    { 0.235, 0.263, 0.302, 1.000},
        .light =    { 0.235, 0.263, 0.302, 1.000}
    };

    ui->main.color_scheme->selected = (Colors) {
         /* cairo    / r  / g  / b  / a  /  */
        .fg =       { 0.900, 0.900, 0.900, 1.000},
        .bg =       { 0.173, 0.196, 0.227, 1.000},
        .base =     { 0.259, 0.518, 0.894, 1.000},
        .text =     { 1.000, 1.000, 1.000, 1.000},
        .shadow =   { 0.259, 0.518, 0.894, 0.200},
        .frame =    { 0.259, 0.518, 0.894, 1.000},
        .light =    { 0.259, 0.518, 0.894, 1.000}
    };

    ui->main.color_scheme->active = (Colors) {
         /* cairo    / r  / g  / b  / a  /  */
        .fg =       { 0.259, 0.518, 0.894, 1.000},
        .bg =       { 0.078, 0.090, 0.106, 1.000},
        .base =     { 0.259, 0.518, 0.894, 1.000},
        .text =     { 0.843, 0.871, 0.902, 1.000},
        .shadow =   { 0.259, 0.518, 0.894, 0.500},
        .frame =    { 0.259, 0.518, 0.894, 1.000},
        .light =    { 0.259, 0.518, 0.894, 1.000}
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

static void create_file_row(X11_UI *ui, ModelPicker *m, int is_ir, int file_id, int norm_widget,
                int norm_port, int erase_widget, int erase_port, int row_x, int row_y, int row_w) {

    const int text_x = row_x + 82; // start of the file name area
    const int combo_r = row_x + row_w - 30; // right edge of the combobox (arrow incl.)

    if (is_ir)
        m->filebutton = add_lv2_irfile_button(m->filebutton, ui->win, file_id,
                            "IR File", ui, row_x + 6, row_y + 4, 22, 22);
    else
        m->filebutton = add_lv2_file_button(m->filebutton, ui->win, file_id,
                            "Neural Model", ui, row_x + 6, row_y + 4, 22, 22);
    m->filebutton->parent_struct = (void*)m;
    m->filebutton->func.user_callback = file_load_response;

    ui->widget[norm_widget] = add_lv2_toggle_button(ui->widget[norm_widget], ui->win,
                            norm_port, "Norm", ui, row_x + 34, row_y + 5, 40, 20);

    m->fbutton = add_lv2_button(m->fbutton, ui->win, "", ui,
                            text_x - 4, row_y, combo_r - (text_x - 4), 30);
    m->fbutton->parent_struct = (void*)m;
    combobox_set_pop_position(m->fbutton, 0);
    combobox_set_entry_length(m->fbutton, 60);
    combobox_add_entry(m->fbutton, "None");
    m->fbutton->func.value_changed_callback = file_menu_callback;

    ui->widget[erase_widget] = add_lv2_erase_button(ui->widget[erase_widget], ui->win,
                            erase_port, "", ui, row_x + row_w - 28, row_y + 4, 22, 22);
}

static void init_picker(ModelPicker *m, const char *filter) {
    m->filename = strdup("None");
    m->dir_name = NULL;
    m->filepicker = (FilePicker*)malloc(sizeof(FilePicker));
    fp_init(m->filepicker, "/");
    asprintf(&m->filepicker->filter ,"%s", filter);
    m->filepicker->use_filter = 1;
}

void plugin_create_controller_widgets(X11_UI *ui, const char * plugin_uri) {

    ui->win->label = plugin_set_name();
    // connect the expose func
    ui->win->func.expose_callback = draw_window;
    ui->win->func.key_press_callback = set_precision;
    ui->win->func.key_release_callback = reset_precision;

    X11_UI_Private_t *ps =(X11_UI_Private_t*)malloc(sizeof(X11_UI_Private_t));
    ui->private_ptr = (void*)ps;
    ps->fname = NULL;
    init_picker(&ps->ma, ".nam|.aidax|.json");
    init_picker(&ps->mb, ".nam|.aidax|.json");
    init_picker(&ps->ir, ".wav");
    init_picker(&ps->ir1, ".wav");

    // header
    ui->widget[15] = add_lv2_switch (ui->widget[15], ui->win, 20, "Buffer", ui, 170, 17, 86, 22);
    ui->widget[16] = add_lv2_switch (ui->widget[16], ui->win, 21, "Phase", ui, 258, 17, 86, 22);
    ui->widget[17] = add_lv2_label (ui->widget[17], ui->win, 22, "Latency", ui, 350, 17, 120, 22);
    ui->widget[18] = add_lv2_label (ui->widget[18], ui->win, 23, "Xrun", ui, 474, 17, 70, 22);
    ui->widget[10] = add_lv2_switch (ui->widget[10], ui->win, 14, "", ui, 558, 17, 40, 22);

    // models card
    ui->widget[19] = add_lv2_slider (ui->widget[19], ui->win, 24, "Slim size", ui, 294, 59, 180, 17);
    set_adjustment(ui->widget[19]->adj, 1.0, 1.0, 0.0, 1.0, 0.1, CL_CONTINUOS);

    ui->widget[0] = add_lv2_knob (ui->widget[0], ui->win, 2, "Input A", ui, 35, 76, 90, 104);
    set_adjustment(ui->widget[0]->adj, 0.0, 0.0, -20.0, 20.0, 0.2, CL_CONTINUOS);
    set_widget_color(ui->widget[0], (Color_state)0, (Color_mod)0, 0.259, 0.518, 0.894, 1.0);
    set_widget_color(ui->widget[0], (Color_state)0, (Color_mod)3, 0.686, 0.729, 0.773, 1.0);

    ui->widget[7] = add_lv2_knob (ui->widget[7], ui->win, 11, "Input B", ui, 147, 76, 90, 104);
    set_adjustment(ui->widget[7]->adj, 0.0, 0.0, -20.0, 20.0, 0.2, CL_CONTINUOS);
    set_widget_color(ui->widget[7], (Color_state)0, (Color_mod)0, 0.259, 0.518, 0.894, 1.0);
    set_widget_color(ui->widget[7], (Color_state)0, (Color_mod)3, 0.686, 0.729, 0.773, 1.0);

    ui->widget[2] = add_lv2_knob (ui->widget[2], ui->win, 4, "Blend", ui, 259, 76, 90, 104);
    set_adjustment(ui->widget[2]->adj, 0.5, 0.5, 0.0, 1.0, 0.01, CL_CONTINUOS);
    set_widget_color(ui->widget[2], (Color_state)0, (Color_mod)0, 0.259, 0.518, 0.894, 1.0);
    set_widget_color(ui->widget[2], (Color_state)0, (Color_mod)3, 0.686, 0.729, 0.773, 1.0);

    ui->widget[4] = add_lv2_knob (ui->widget[4], ui->win, 8, "Delay", ui, 371, 76, 90, 104);
    set_adjustment(ui->widget[4]->adj, 0.0, 0.0, -4096.0, 4096.0, 16.0, CL_CONTINUOS);
    set_widget_color(ui->widget[4], (Color_state)0, (Color_mod)0, 0.259, 0.518, 0.894, 1.0);
    set_widget_color(ui->widget[4], (Color_state)0, (Color_mod)3, 0.686, 0.729, 0.773, 1.0);

    // impulse response card
    ui->widget[3] = add_lv2_knob (ui->widget[3], ui->win, 7, "Mix", ui, 24, 298, 90, 104);
    set_adjustment(ui->widget[3]->adj, 0.5, 0.5, 0.0, 1.0, 0.01, CL_CONTINUOS);
    set_widget_color(ui->widget[3], (Color_state)0, (Color_mod)0, 0.259, 0.518, 0.894, 1.0);
    set_widget_color(ui->widget[3], (Color_state)0, (Color_mod)3, 0.686, 0.729, 0.773, 1.0);


    // output card
    ui->widget[1] = add_lv2_knob (ui->widget[1], ui->win, 3, "Output", ui, 498, 176, 96, 112);
    set_adjustment(ui->widget[1]->adj, 0.0, 0.0, -20.0, 20.0, 0.2, CL_CONTINUOS);
    set_widget_color(ui->widget[1], (Color_state)0, (Color_mod)0, 0.259, 0.518, 0.894, 1.0);
    set_widget_color(ui->widget[1], (Color_state)0, (Color_mod)3, 0.686, 0.729, 0.773, 1.0);

    // file rows
    create_file_row(ui, &ps->ma,  0, -1,  8, 12, 11, 15,  24, 186, 450);
    create_file_row(ui, &ps->mb,  0, -2,  9, 13, 12, 16,  24, 222, 450);
    create_file_row(ui, &ps->ir,  1, -3,  5,  9, 13, 17, 124, 316, 350);
    create_file_row(ui, &ps->ir1, 1, -4,  6, 10, 14, 18, 124, 354, 350);
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
