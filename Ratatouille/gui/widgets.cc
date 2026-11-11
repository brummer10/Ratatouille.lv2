/*
 * widgets.cc
 *
 * SPDX-License-Identifier:  BSD-3-Clause
 *
 * Copyright (C) 2025 brummer <brummer@web.de>
 */

/****************************************************************
 *                  Flat dark look
 ****************************************************************/

// palette (r, g, b)
#define C_BG       0.078, 0.090, 0.106   // window + rows
#define C_CARD     0.122, 0.137, 0.157   // cards
#define C_LINE     0.173, 0.196, 0.227   // borders, tracks
#define C_LINE_HI  0.235, 0.263, 0.302   // hover fill
#define C_TEXT     0.686, 0.729, 0.773   // normal text
#define C_TEXT_HI  0.843, 0.871, 0.902   // title, hover text
#define C_MUTED    0.490, 0.529, 0.576   // captions, icons
#define C_ACCENT   0.259, 0.518, 0.894   // blue accent
#define C_WARN     0.890, 0.620, 0.180   // xrun warning

// a file row: position and the text area, in base (unscaled) units
typedef struct {
    double x, y, w, h;   // row rectangle
    double tx, tw;       // file name text start and max. width
} RowGeom;

static const RowGeom rows[4] = {
    {  24, 186, 450, 30, 106, 314 },   // model A
    {  24, 222, 450, 30, 106, 314 },   // model B
    { 124, 316, 350, 30, 206, 214 },   // IR 1
    { 124, 354, 350, 30, 206, 214 }    // IR 2
};

void roundrec(cairo_t *cr, double x, double y, double width, double height, double r) {
    cairo_arc(cr, x+r, y+r, r, M_PI, 3*M_PI/2);
    cairo_arc(cr, x+width-r, y+r, r, 3*M_PI/2, 0);
    cairo_arc(cr, x+width-r, y+height-r, r, 0, M_PI/2);
    cairo_arc(cr, x+r, y+height-r, r, M_PI/2, M_PI);
    cairo_close_path(cr);
}

static void flat_box(cairo_t *cr, double x, double y, double w, double h, double r) {
    cairo_new_path(cr);
    roundrec(cr, x, y, w, h, min(r, min(w, h) * 0.5));
}

char* utf8crop(char* dst, const char* src, size_t sizeDest ) {
    if( sizeDest ){
        size_t sizeSrc = strlen(src);
        while( sizeSrc >= sizeDest ){
            const char* lastByte = src + sizeSrc;
            while( lastByte-- > src )
                if((*lastByte & 0xC0) != 0x80)
                    break;
            sizeSrc = lastByte - src;
        }
        memcpy(dst, src, sizeSrc);
        dst[sizeSrc] = '\0';
    }
    return dst;
}

static void draw_card(cairo_t *cr, double x, double y, double w, double h, double s) {
    flat_box(cr, x, y, w, h, 10 * s);
    cairo_set_source_rgb(cr, C_CARD);
    cairo_fill_preserve(cr);
    cairo_set_line_width(cr, 1.0);
    cairo_set_source_rgb(cr, C_LINE);
    cairo_stroke(cr);
}

#ifdef USE_ATOM
// draw the file name of a ModelPicker, left aligned and cropped to the row
static void draw_file_label(Widget_t *w, ModelPicker *m, const RowGeom *r) {
    const double s = w->app->hdpi;
    if (!m->filename || !strlen(m->filename)) return;
    char label[124];
    char shown[132];
    const char *base = basename(m->filename);
    const size_t blen = strlen(base);
    cairo_text_extents_t ext;
    cairo_font_extents_t fe;

    cairo_set_font_size(w->crb, w->app->normal_font);

    size_t n = blen + 1;
    if (n > 120) n = 120;
    for (;;) {
        utf8crop(label, base, n);
        snprintf(shown, sizeof(shown), "%s%s", label,
                                    strlen(label) < blen ? "..." : "");
        cairo_text_extents(w->crb, shown, &ext);
        if (ext.x_advance <= r->tw * s || n <= 5) break;
        n--;
    }

    if (strlen(label) < blen) {
        tooltip_set_text(m->filebutton, base);
        m->filebutton->flags |= HAS_TOOLTIP;
    } else {
        m->filebutton->flags &= ~HAS_TOOLTIP;
        hide_tooltip(m->filebutton);
    }

    if (strcmp(base, "None") == 0) cairo_set_source_rgb(w->crb, C_MUTED);
    else cairo_set_source_rgb(w->crb, C_TEXT);
    cairo_font_extents(w->crb, &fe);
    cairo_move_to(w->crb, r->tx * s,
        (r->y + r->h * 0.5) * s + (fe.ascent - fe.descent) * 0.5);
    cairo_text_path (w->crb, shown);
    cairo_fill (w->crb);
    cairo_new_path(w->crb);
}
#endif

// draw the window
static void draw_window(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    cairo_t *cr = w->crb;
    const double s = w->app->hdpi;

    cairo_set_source_rgb(cr, C_BG);
    cairo_paint(cr);

    widget_set_scale(w);

    draw_card(cr,  12 * s,  56 * s, 474 * s, 206 * s, s);   // neural models
    draw_card(cr,  12 * s, 272 * s, 474 * s, 135 * s, s);   // impulse response
    draw_card(cr, 494 * s,  56 * s, 104 * s, 351 * s, s);   // output

    for (int i = 0; i < 4; i++) {
        flat_box(cr, rows[i].x * s, rows[i].y * s, rows[i].w * s, rows[i].h * s, 8 * s);
        cairo_set_source_rgb(cr, C_BG);
        cairo_fill(cr);
    }

    cairo_set_font_size(cr, w->app->small_font);
    cairo_set_source_rgb(cr, C_MUTED);
    cairo_move_to(cr, 24 * s,  73 * s);
    cairo_text_path (cr, "Neural models");
    cairo_fill (w->crb);
    cairo_move_to(cr, 24 * s, 289 * s);
    cairo_text_path (cr, "Impulse response");
    cairo_fill (w->crb);

#ifdef USE_ATOM
    X11_UI* ui = (X11_UI*)w->parent_struct;
    X11_UI_Private_t *ps = (X11_UI_Private_t*)ui->private_ptr;
    ModelPicker *mp[4] = { &ps->ma, &ps->mb, &ps->ir, &ps->ir1 };
    for (int i = 0; i < 4; i++) draw_file_label(w, mp[i], &rows[i]);
#endif

#ifndef HIDE_NAME
    cairo_set_font_size(cr, w->app->big_font + 6);
    cairo_set_source_rgb(cr, C_TEXT_HI);
    cairo_move_to(cr, 24 * s, 36 * s);
    cairo_text_path (w->crb, w->label);
    cairo_fill (w->crb);
#endif
    widget_reset_scale(w);
    cairo_new_path(cr);
}

// if controller value changed send notify to host
static void value_changed(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    X11_UI* ui = (X11_UI*)w->parent_struct;
    float v = adj_get_value(w->adj);
    sendValueChanged(ui, w->data, v);
}

static void draw_my_knob(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;

    /** geometry **/
    const double width = w->width;
    const double height = w->height - (w->height * 0.15);
    const double grow = (width > height) ? height : width;
    const double cx = width * 0.5;
    const double cy = height * 0.5;
    const double R = grow * 0.44; // track radius
    const double lw = max(3.0, grow * 0.05); // track width
    const double body = R * 0.74; // knob body radius

    /** angles, 270 degree sweep with the gap at the bottom **/
    const double a0 = 135 * (M_PI / 180);
    const double sweep = 270 * (M_PI / 180);
    const double state = adj_get_state(w->adj);
    const double a = a0 + state * sweep;
    double zero = 0.0;  // bipolar knobs start at 0
    if (w->adj->min_value < 0.0 && w->adj->max_value > 0.0)
        zero = -w->adj->min_value / (w->adj->max_value - w->adj->min_value);
    const double az = a0 + zero * sweep;

    cairo_push_group(w->crb);
    cairo_set_line_cap(w->crb, CAIRO_LINE_CAP_ROUND);

    /** track **/
    cairo_new_path(w->crb);
    cairo_set_line_width(w->crb, lw);
    cairo_set_source_rgb(w->crb, C_LINE);
    cairo_arc(w->crb, cx, cy, R, a0, a0 + sweep);
    cairo_stroke(w->crb);

    /** value arc **/
    if (fabs(a - az) > 0.001) {
        cairo_new_path(w->crb);
        use_fg_color_scheme(w, NORMAL_);
        cairo_arc(w->crb, cx, cy, R, min(a, az), max(a, az));
        cairo_stroke(w->crb);
    }

    /** body **/
    cairo_new_path(w->crb);
    cairo_arc(w->crb, cx, cy, body, 0, 2 * M_PI);
    if (w->state == 1) cairo_set_source_rgb(w->crb, 0.106, 0.122, 0.145);
    else cairo_set_source_rgb(w->crb, C_BG);
    cairo_fill_preserve(w->crb);
    cairo_set_line_width(w->crb, 1.0);
    cairo_set_source_rgb(w->crb, C_LINE);
    cairo_stroke(w->crb);

    /** pointer **/
    cairo_new_path(w->crb);
    cairo_set_line_width(w->crb, max(2.0, grow * 0.035));
    cairo_set_source_rgb(w->crb, C_TEXT_HI);
    cairo_move_to(w->crb, cx + cos(a) * body * 0.70, cy + sin(a) * body * 0.70);
    cairo_line_to(w->crb, cx + cos(a) * body * 0.94, cy + sin(a) * body * 0.94);
    cairo_stroke(w->crb);

    /** value in the center **/
    cairo_text_extents_t extents;
    cairo_set_font_size(w->crb, w->app->small_font / w->scale.ascale);
    use_text_color_scheme(w, get_color_state(w));
    char s[17];
    float value = adj_get_value(w->adj);
    float v = copysign(1, (int)(value * 10));
    value = copysign(value, v);                   // no "-0.0"
    if (fabs(w->adj->step) > 0.99) snprintf(s, 16, "%d", (int)value);
    else if (fabs(w->adj->step) < 0.09) snprintf(s, 16, "%.2f", value);
    else snprintf(s, 16, "%.1f", value);
    cairo_text_extents(w->crb, s, &extents);
    cairo_move_to(w->crb, cx - (extents.width * 0.5 + extents.x_bearing),
                          cy - (extents.y_bearing + extents.height * 0.5));
    cairo_text_path (w->crb, s);
    cairo_fill (w->crb);
    cairo_new_path(w->crb);

    /** label below **/
    cairo_set_font_size(w->crb, w->app->normal_font / w->scale.ascale);
    use_text_color_scheme(w, NORMAL_);
    cairo_text_extents(w->crb, w->label, &extents);
    cairo_move_to(w->crb, cx - (extents.width * 0.5 + extents.x_bearing),
                          w->height - 3);
    cairo_text_path (w->crb, w->label);
    cairo_fill (w->crb);
    cairo_new_path(w->crb);

    cairo_pop_group_to_source(w->crb);
    cairo_paint(w->crb);
}

void set_precision(void *w_, void* xkey_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    X11_UI* ui = (X11_UI*)w->parent_struct;
    XKeyEvent *xkey = (XKeyEvent*)xkey_;
    if ((xkey->keycode == XKeysymToKeycode(w->app->dpy,XK_Control_L) ||
        xkey->keycode == XKeysymToKeycode(w->app->dpy,XK_Control_R))) {
        ui->widget[4]->adj->step = 1;
    }
}

void reset_precision(void *w_, void* xkey_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    X11_UI* ui = (X11_UI*)w->parent_struct;
    XKeyEvent *xkey = (XKeyEvent*)xkey_;
    if ((xkey->keycode == XKeysymToKeycode(w->app->dpy,XK_Control_L) ||
        xkey->keycode == XKeysymToKeycode(w->app->dpy,XK_Control_R))) {
        ui->widget[4]->adj->step = 16;
    }
}

Widget_t* add_lv2_knob(Widget_t *w, Widget_t *p, int index, const char * label,
                                X11_UI* ui, int x, int y, int width, int height) {
    w = add_knob(p, label, x, y, width, height);
    w->parent_struct = ui;
    w->data = index;
    w->func.expose_callback = draw_my_knob;
    w->func.value_changed_callback = value_changed;
    return w;
}

static void draw_my_switch(void *w_, void* user_data) {
    Widget_t *wid = (Widget_t*)w_;
    const double W = wid->width;
    const double H = wid->height;
    const int on = (int)adj_get_state(wid->adj);
    const int has_label = (wid->label && strlen(wid->label));

    const double ph = H * 0.64;
    const double pw = min(ph * 1.9, W - 2);
    const double px = has_label ? 1.0 : (W - pw) * 0.5;
    const double py = (H - ph) * 0.5;

    cairo_push_group(wid->crb);

    flat_box(wid->crb, px, py, pw, ph, ph * 0.5);
    if (on) cairo_set_source_rgb(wid->crb, C_ACCENT);
    else if (wid->state == 1) cairo_set_source_rgb(wid->crb, C_LINE_HI);
    else cairo_set_source_rgb(wid->crb, C_LINE);
    cairo_fill(wid->crb);

    const double tr = ph * 0.5 - ph * 0.16;
    const double tcx = on ? px + pw - ph * 0.5 : px + ph * 0.5;
    cairo_new_path(wid->crb);
    cairo_arc(wid->crb, tcx, py + ph * 0.5, tr, 0, 2 * M_PI);
    if (on) cairo_set_source_rgb(wid->crb, 1.0, 1.0, 1.0);
    else cairo_set_source_rgb(wid->crb, C_MUTED);
    cairo_fill(wid->crb);

    if (has_label) {
        cairo_font_extents_t fe;
        cairo_set_font_size(wid->crb, wid->app->small_font / wid->scale.ascale);
        cairo_font_extents(wid->crb, &fe);
        cairo_set_source_rgb(wid->crb, C_TEXT);
        cairo_move_to(wid->crb, px + pw + ph * 0.6,
                                H * 0.5 + (fe.ascent - fe.descent) * 0.5);
        cairo_text_path (wid->crb, wid->label);
        cairo_fill (wid->crb);
    }
    cairo_new_path(wid->crb);

    cairo_pop_group_to_source(wid->crb);
    cairo_paint(wid->crb);
}

Widget_t* add_lv2_switch(Widget_t *w, Widget_t *p, int index, const char * label,
                                X11_UI* ui, int x, int y, int width, int height) {
    w = add_toggle_button(p, label, x, y, width, height);
    w->parent_struct = ui;
    w->data = index;
    w->func.expose_callback = draw_my_switch;
    w->func.value_changed_callback = value_changed;
    return w;
}


#define SLIDER_TRACK_X0 (62.0 / 180.0)
#define SLIDER_TRACK_X1 (148.0 / 180.0)

static void draw_my_slider(void *w_, void* user_data) {
    Widget_t *wid = (Widget_t*)w_;
    cairo_t *cr = wid->crb;
    const double W = wid->width;
    const double H = wid->height;
    const double state = adj_get_state(wid->adj);
    const double x0 = W * SLIDER_TRACK_X0;
    const double x1 = W * SLIDER_TRACK_X1;
    const double th = max(3.0, H * 0.22); // track thickness
    const double r = H * 0.30 * ((wid->state == 1 || wid->state == 2) ? 1.15 : 1.0);
    const double cy = H * 0.5;
    const double tx = x0 + (x1 - x0) * state;
    cairo_font_extents_t fe;
    cairo_text_extents_t ext;
    char s[16];

    cairo_push_group(cr);

    // label
    cairo_set_font_size(cr, wid->app->small_font / wid->scale.ascale);
    cairo_font_extents(cr, &fe);
    cairo_set_source_rgb(cr, C_MUTED);
    cairo_move_to(cr, 0, cy + (fe.ascent - fe.descent) * 0.5);
    cairo_text_path (cr, wid->label);
    cairo_fill (cr);

    // track and filled part
    flat_box(cr, x0, cy - th * 0.5, x1 - x0, th, th * 0.5);
    cairo_set_source_rgb(cr, C_LINE);
    cairo_fill(cr);
    flat_box(cr, x0, cy - th * 0.5, max(tx - x0, th), th, th * 0.5);
    cairo_set_source_rgb(cr, C_ACCENT);
    cairo_fill(cr);

    // thumb
    cairo_new_path(cr);
    cairo_arc(cr, tx, cy, r, 0, 2 * M_PI);
    cairo_set_source_rgb(cr, C_TEXT_HI);
    cairo_fill(cr);

    // value, right aligned
    snprintf(s, 15, "%.1f", adj_get_value(wid->adj));
    cairo_text_extents(cr, s, &ext);
    cairo_set_source_rgb(cr, C_TEXT);
    cairo_move_to(cr, W - ext.x_advance, cy + (fe.ascent - fe.descent) * 0.5);
    cairo_show_text(cr, s);
    cairo_new_path(cr);

    cairo_pop_group_to_source(cr);
    cairo_paint(cr);
}

static void slider_pressed(void *w_, void* button_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    XButtonEvent *xbutton = (XButtonEvent*)button_;
    if (xbutton->button != Button1) return;
    Adjustment_t *adj = w->adj;
    const double x0 = w->width * SLIDER_TRACK_X0;
    const double x1 = w->width * SLIDER_TRACK_X1;
    if (xbutton->x >= x0 - 6 && xbutton->x <= x1 + 6) {
        double t = (xbutton->x - x0) / (x1 - x0);
        t = min(1.0, max(0.0, t));
        float value = adj->min_value + t * (adj->max_value - adj->min_value);
        if (adj->step > 0.0)
            value = adj->min_value + round((value - adj->min_value) / adj->step) * adj->step;
        value = min(adj->max_value, max(adj->min_value, value));
        check_value_changed(adj, &value);
        adj_set_start_value(w);
    }
    if (adj->step > 0.0)
        adj->scale = (adj->max_value - adj->min_value) / (adj->step * (x1 - x0));
}

Widget_t* add_lv2_slider(Widget_t *w, Widget_t *p, int index, const char * label,
                                X11_UI* ui, int x, int y, int width, int height) {
    w = add_hslider(p, label, x, y, width, height);
    w->parent_struct = ui;
    w->data = index;
    w->func.expose_callback = draw_my_slider;
    w->func.button_press_callback = slider_pressed;
    w->func.value_changed_callback = value_changed;
    return w;
}

// latency / xrun read out in the header
void draw_my_label(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    Metrics_t metrics;
    os_get_window_metrics(w, &metrics);
    if (!metrics.visible) return;

    char s[64];
    float value = adj_get_value(w->adj);
    if (w->data == 22) snprintf(s, 63, "Latency: %.2f ms", value);
    else snprintf(s, 63, "Xruns: %.0f", value);

    cairo_font_extents_t fe;
    cairo_set_font_size(w->crb, w->app->small_font / w->scale.ascale);
    cairo_font_extents(w->crb, &fe);
    if (w->data != 22 && value > 0.5) cairo_set_source_rgb(w->crb, C_WARN);
    else cairo_set_source_rgb(w->crb, C_MUTED);
    cairo_move_to(w->crb, 0, metrics.height * 0.5 + (fe.ascent - fe.descent) * 0.5);
    cairo_text_path (w->crb, s);
    cairo_fill (w->crb);
    cairo_new_path(w->crb);
}

Widget_t* add_lv2_label(Widget_t *w, Widget_t *p, int index, const char * label,
                                X11_UI* ui, int x, int y, int width, int height) {
    w = add_label(p, label, x, y, width, height);
    w->adj_y = add_adjustment(w,0.0, 0.0, 0.0, 127.0,0.01, CL_CONTINUOS);
    w->adj = w->adj_y;
    w->scale.gravity = CENTER;
    w->parent_struct = ui;
    w->data = index;
    w->func.expose_callback = draw_my_label;
    w->func.value_changed_callback = value_changed;
    return w;
}

static void dummy_expose(void *w_, void* user_data) {
}

void draw_my_button(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    if (!w) return;
    Metrics_t metrics;
    os_get_window_metrics(w, &metrics);
    if (!metrics.visible) return;
    const double cx = metrics.width * 0.5;
    const double cy = metrics.height * 0.5;
    const double sz = min(metrics.width, metrics.height) * 0.28;

    if (w->state == 1 || w->state == 2) cairo_set_source_rgb(w->crb, C_TEXT_HI);
    else cairo_set_source_rgb(w->crb, C_MUTED);
    cairo_set_line_width(w->crb, max(1.5, sz * 0.35));
    cairo_set_line_cap(w->crb, CAIRO_LINE_CAP_ROUND);
    cairo_set_line_join(w->crb, CAIRO_LINE_JOIN_ROUND);
    cairo_new_path(w->crb);
    cairo_move_to(w->crb, cx - sz, cy - sz * 0.5);
    cairo_line_to(w->crb, cx, cy + sz * 0.5);
    cairo_line_to(w->crb, cx + sz, cy - sz * 0.5);
    cairo_stroke(w->crb);
}

Widget_t* add_lv2_button(Widget_t *w, Widget_t *p, const char * label,
                                X11_UI* ui, int x, int y, int width, int height) {
    w = add_combobox(p, label, x, y, width, height);
    w->parent_struct = ui;
    w->func.expose_callback = dummy_expose;
    w->childlist->childs[0]->func.expose_callback = draw_my_button;
    return w;
}

static void my_fdialog_response(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    FileButton *filebutton = (FileButton *)w->private_struct;
    if(user_data !=NULL) {
        char *tmp = strdup(*(const char**)user_data);
        free(filebutton->last_path);
        filebutton->last_path = NULL;
        filebutton->last_path = strdup(dirname(tmp));
        filebutton->path = filebutton->last_path;
        free(tmp);
    }
    w->func.user_callback(w,user_data);
    filebutton->is_active = false;
    adj_set_value(w->adj,0.0);
}

static void my_fbutton_callback(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    FileButton *filebutton = (FileButton *)w->private_struct;
    if (w->flags & HAS_POINTER && adj_get_value(w->adj)){
        filebutton->is_active = true;
        if (!filebutton->w) {
            filebutton->w = open_file_dialog(w,filebutton->path,filebutton->filter);
            filebutton->w->flags |= HIDE_ON_DELETE;
            if (strcmp(filebutton->filter, ".wav") == 0) {
                widget_set_title(filebutton->w, _("File Selector - Select Impulse Response"));
            } else {
                widget_set_title(filebutton->w, _("File Selector - Select Neural Model"));
            }
#ifdef _OS_UNIX_
            Atom wmStateAbove = XInternAtom(w->app->dpy, "_NET_WM_STATE_ABOVE", 1 );
            Atom wmNetWmState = XInternAtom(w->app->dpy, "_NET_WM_STATE", 1 );
            XChangeProperty(w->app->dpy, filebutton->w->widget, wmNetWmState, XA_ATOM, 32, 
                PropModeReplace, (unsigned char *) &wmStateAbove, 1); 
#elif defined _WIN32
            os_set_transient_for_hint(w, filebutton->w);
#endif
        } else {
            widget_show_all(filebutton->w);
        }
    } else if (w->flags & HAS_POINTER && !adj_get_value(w->adj)){
        if(filebutton->is_active)
            widget_hide(filebutton->w);
    }
}

static void my_fbutton_mem_free(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    FileButton *filebutton = (FileButton *)w->private_struct;
    free(filebutton->last_path);
    filebutton->last_path = NULL;
    free(filebutton);
    filebutton = NULL;
}

static void icon_button_bg(Widget_t *w, double W, double H, int active) {
    if (!(w->state == 1 || w->state == 2 || active)) return;
    flat_box(w->crb, 1, 1, W - 2, H - 2, 6);
    if (active) cairo_set_source_rgba(w->crb, C_ACCENT, 0.25);
    else cairo_set_source_rgb(w->crb, C_LINE);
    cairo_fill(w->crb);
}

static void draw_file_button(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    if (!w) return;
    Metrics_t metrics;
    os_get_window_metrics(w, &metrics);
    if (!metrics.visible) return;
    cairo_t *cr = w->crb;
    FileButton *fb = (FileButton*)w->private_struct;
    const int is_ir = (fb && fb->filter && strcmp(fb->filter, ".wav") == 0);
    const int active = (int)adj_get_value(w->adj);
    const double W = metrics.width;
    const double H = metrics.height;
    const double cx = W * 0.5;
    const double cy = H * 0.5;
    const double sz = min(W, H) * 0.56;

    icon_button_bg(w, W, H, active);
    if (active) cairo_set_source_rgb(cr, C_ACCENT);
    else if (w->state == 1) cairo_set_source_rgb(cr, C_TEXT_HI);
    else cairo_set_source_rgb(cr, C_MUTED);
    cairo_set_line_width(cr, max(1.3, sz * 0.09));
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);

    if (is_ir) {
        static const double bars[5] = { 0.30, 0.65, 1.00, 0.65, 0.30 };
        cairo_set_line_width(cr, max(1.6, sz * 0.14));
        for (int i = 0; i < 5; i++) {
            const double bx = cx + (i - 2) * sz * 0.30;
            const double bh = sz * bars[i] * 0.5;
            cairo_new_path(cr);
            cairo_move_to(cr, bx, cy - bh);
            cairo_line_to(cr, bx, cy + bh);
            cairo_stroke(cr);
        }
    } else {
        const double x0 = cx - sz * 0.55;
        const double y0 = cy - sz * 0.40;
        const double fw = sz * 1.10;
        const double fh = sz * 0.80;
        flat_box(cr, x0, y0 + fh * 0.20, fw, fh * 0.80, sz * 0.12);
        cairo_stroke(cr);
        cairo_new_path(cr);
        cairo_move_to(cr, x0 + fw * 0.08, y0 + fh * 0.20);
        cairo_line_to(cr, x0 + fw * 0.12, y0);
        cairo_line_to(cr, x0 + fw * 0.42, y0);
        cairo_line_to(cr, x0 + fw * 0.50, y0 + fh * 0.20);
        cairo_stroke(cr);
    }
    cairo_new_path(cr);
}

Widget_t *add_my_file_button(Widget_t *parent, int x, int y, int width, int height,
                           const char* label, const char *path, const char *filter) {
    FileButton *filebutton = (FileButton*)malloc(sizeof(FileButton));
    filebutton->path = path;
    filebutton->filter = filter;
    filebutton->last_path = NULL;
    filebutton->w = NULL;
    filebutton->is_active = false;
    Widget_t *fbutton = add_toggle_button(parent, label, x, y, width, height);
    fbutton->private_struct = filebutton;
    fbutton->flags |= HAS_MEM;
    fbutton->scale.gravity = CENTER;
    fbutton->func.mem_free_callback = my_fbutton_mem_free;
    fbutton->func.value_changed_callback = my_fbutton_callback;
    fbutton->func.dialog_callback = my_fdialog_response;
    fbutton->func.expose_callback = draw_file_button;
    return fbutton;
}

Widget_t* add_lv2_file_button(Widget_t *w, Widget_t *p, int index, const char * label,
                                X11_UI* ui, int x, int y, int width, int height) {
    w = add_my_file_button(p, x, y, width, height, "neural", "", ".nam|.aidax|.json");
    w->data = index;
    return w;
}

Widget_t* add_lv2_irfile_button(Widget_t *w, Widget_t *p, int index, const char * label,
                                X11_UI* ui, int x, int y, int width, int height) {
    w = add_my_file_button(p, x, y, width, height, "IR File", "", ".wav");
    w->data = index;
    return w;
}

static void draw_chip(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    if (!w) return;
    Metrics_t metrics;
    os_get_window_metrics(w, &metrics);
    if (!metrics.visible) return;
    cairo_t *cr = w->crb;
    const double W = metrics.width;
    const double H = metrics.height;
    const int on = (int)adj_get_value(w->adj);

    flat_box(cr, 1, 1, W - 2, H - 2, H * 0.3);
    if (on) cairo_set_source_rgb(cr, C_ACCENT);
    else if (w->state == 1) cairo_set_source_rgb(cr, C_LINE_HI);
    else cairo_set_source_rgb(cr, C_LINE);
    cairo_fill(cr);

    if (w->label && strlen(w->label)) {
        cairo_text_extents_t ext;
        cairo_font_extents_t fe;
        cairo_set_font_size(cr, w->app->small_font / w->scale.ascale);
        cairo_text_extents(cr, w->label, &ext);
        cairo_font_extents(cr, &fe);
        if (on) cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
        else if (w->state == 1) cairo_set_source_rgb(cr, C_TEXT_HI);
        else cairo_set_source_rgb(cr, C_TEXT);
        cairo_move_to(cr, (W - ext.x_advance) * 0.5,
                          H * 0.5 + (fe.ascent - fe.descent) * 0.5);
        cairo_text_path (w->crb, w->label);
        cairo_fill (w->crb);
    }
    cairo_new_path(cr);
}

Widget_t* add_lv2_toggle_button(Widget_t *w, Widget_t *p, int index, const char * label,
                                X11_UI* ui, int x, int y, int width, int height) {
    w = add_toggle_button(p, label, x, y, width, height);
    w->parent_struct = ui;
    w->data = index;
    w->func.expose_callback = draw_chip;
    w->func.value_changed_callback = value_changed;
    return w;
}

static void draw_eject_button(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    if (!w) return;
    Metrics_t metrics;
    os_get_window_metrics(w, &metrics);
    if (!metrics.visible) return;
    cairo_t *cr = w->crb;
    const double W = metrics.width;
    const double H = metrics.height;
    const double cx = W * 0.5;
    const double cy = H * 0.5;
    const double sz = min(W, H) * 0.52;

    icon_button_bg(w, W, H, 0);
    if (w->state == 1 || w->state == 2) cairo_set_source_rgb(cr, C_TEXT_HI);
    else cairo_set_source_rgb(cr, C_MUTED);
    cairo_new_path(cr);
    cairo_move_to(cr, cx, cy - sz * 0.55);
    cairo_line_to(cr, cx + sz * 0.60, cy + sz * 0.10);
    cairo_line_to(cr, cx - sz * 0.60, cy + sz * 0.10);
    cairo_close_path(cr);
    cairo_fill(cr);
    flat_box(cr, cx - sz * 0.60, cy + sz * 0.30, sz * 1.20, sz * 0.22, sz * 0.06);
    cairo_fill(cr);
    cairo_new_path(cr);
}

Widget_t* add_lv2_erase_button(Widget_t *w, Widget_t *p, int index, const char * label,
                                X11_UI* ui, int x, int y, int width, int height) {
    w = add_button(p, "", x, y, width, height);
    w->parent_struct = ui;
    w->data = index;
    w->func.expose_callback = draw_eject_button;
    w->func.value_changed_callback = value_changed;
    return w;
}
