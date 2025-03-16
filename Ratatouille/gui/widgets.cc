/*
 * widgets.cc
 *
 * SPDX-License-Identifier:  BSD-3-Clause
 *
 * Copyright (C) 2025 brummer <brummer@web.de>
 */



void boxShadowInset(cairo_t* const cr, int x, int y, int width, int height, bool fill) {
    cairo_pattern_t *pat = cairo_pattern_create_linear (x, y, x + width, y);
    cairo_pattern_add_color_stop_rgba
        (pat, 1, 0.33, 0.33, 0.33, 1.0);
    cairo_pattern_add_color_stop_rgba
        (pat, 0.9844, 0.33 * 0.6, 0.33 * 0.6, 0.33 * 0.6, 0.0);
    cairo_pattern_add_color_stop_rgba
        (pat, 0.05, 0.05 * 2.0, 0.05 * 2.0, 0.05 * 2.0, 0.0);
    cairo_pattern_add_color_stop_rgba
        (pat, 0, 0.05, 0.05, 0.05, 1.0);
    cairo_set_source(cr, pat);
    if (fill) cairo_fill_preserve (cr);
    else cairo_paint (cr);
    cairo_pattern_destroy (pat);
    pat = NULL;
    pat = cairo_pattern_create_linear (x, y, x, y + height);
    cairo_pattern_add_color_stop_rgba
        (pat, 1, 0.33, 0.33, 0.33, 1.0);
    cairo_pattern_add_color_stop_rgba
        (pat, 0.93, 0.33 * 0.6, 0.33 * 0.6, 0.33 * 0.6, 0.0);
    cairo_pattern_add_color_stop_rgba
        (pat, 0.1, 0.05 * 2.0, 0.05 * 2.0, 0.05 * 2.0, 0.0);
    cairo_pattern_add_color_stop_rgba 
        (pat, 0, 0.05, 0.05, 0.05, 1.0);
    cairo_set_source(cr, pat);
    if (fill) cairo_fill_preserve (cr);
    else cairo_paint (cr);
    cairo_pattern_destroy (pat);
}

void boxShadowOutset(cairo_t* const cr, int x, int y, int width, int height, bool fill) {
    cairo_pattern_t *pat = cairo_pattern_create_linear (x, y, x + width, y);
    cairo_pattern_add_color_stop_rgba
        (pat, 0,0.33,0.33,0.33, 1.0);
    cairo_pattern_add_color_stop_rgba
        (pat, 0.03,0.33 * 0.6,0.33 * 0.6,0.33 * 0.6, 0.0);
    cairo_pattern_add_color_stop_rgba
        (pat, 0.99, 0.05 * 2.0, 0.05 * 2.0, 0.05 * 2.0, 0.0);
    cairo_pattern_add_color_stop_rgba 
        (pat, 1, 0.05, 0.05, 0.05, 1.0);
    cairo_set_source(cr, pat);
    if (fill) cairo_fill_preserve (cr);
    else cairo_paint (cr);
    cairo_pattern_destroy (pat);
    pat = NULL;
    pat = cairo_pattern_create_linear (x, y, x, y + height);
    cairo_pattern_add_color_stop_rgba
        (pat, 0,0.33,0.33,0.33, 1.0);
    cairo_pattern_add_color_stop_rgba
        (pat, 0.03,0.33 * 0.6,0.33 * 0.6,0.33 * 0.6, 0.0);
    cairo_pattern_add_color_stop_rgba
        (pat, 0.97, 0.05 * 2.0, 0.05 * 2.0, 0.05 * 2.0, 0.0);
    cairo_pattern_add_color_stop_rgba
        (pat, 1, 0.05, 0.05, 0.05, 1.0);
    cairo_set_source(cr, pat);
    if (fill) cairo_fill_preserve (cr);
    else cairo_paint (cr);
    cairo_pattern_destroy (pat);
}

void round_rectangle(cairo_t *cr,float x, float y, float width, float height, float round) {
    float r = height* round;
    cairo_new_path (cr);
    cairo_arc(cr, x+r, y+r, r, M_PI, 3*M_PI/2);
    cairo_arc(cr, x+width-1-r, y+r, r, 3*M_PI/2, 0);
    cairo_arc(cr, x+width-1-r, y+height-1-r, r, 0, M_PI/2);
    cairo_arc(cr, x+r, y+height-1-r, r, M_PI/2, M_PI);
    cairo_close_path(cr);
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

// draw the window
static void draw_window(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    
    set_pattern(w,&w->color_scheme->selected,&w->color_scheme->normal,BACKGROUND_);
    cairo_paint (w->crb);

    round_rectangle(w->crb, 10 * w->app->hdpi, 10 * w->app->hdpi,
        w->width-20 * w->app->hdpi, w->height-20 * w->app->hdpi, 0.08);

    cairo_pattern_t *pat = cairo_pattern_create_for_surface(w->image);
    cairo_pattern_set_extend (pat, CAIRO_EXTEND_REPEAT);
    cairo_set_source(w->crb, pat);
    cairo_fill_preserve (w->crb);

    boxShadowOutset(w->crb,10 * w->app->hdpi,10 * w->app->hdpi,
        w->width-20 * w->app->hdpi,w->height-20 * w->app->hdpi, true);
    cairo_stroke (w->crb);
    cairo_pattern_destroy (pat);

#ifndef HIDE_NAME
    cairo_text_extents_t extents;
    use_text_color_scheme(w, NORMAL_);
    cairo_set_font_size (w->crb, w->app->big_font+8);
    cairo_text_extents(w->crb,w->label , &extents);
    double tw = extents.width/2.0;
#endif

    widget_set_scale(w);

    cairo_set_source_rgba(w->crb, 0.1, 0.1, 0.1, 0.333);
    round_rectangle(w->crb, 25 * w->app->hdpi, 70 * w->app->hdpi,
        560 * w->app->hdpi, 145 * w->app->hdpi, 0.08);
    cairo_fill_preserve (w->crb);
    boxShadowInset(w->crb,25 * w->app->hdpi,70 * w->app->hdpi,
        560 * w->app->hdpi,145 * w->app->hdpi, true);
    cairo_stroke (w->crb);

    cairo_set_source_rgba(w->crb, 0.1, 0.1, 0.1, 1);
    round_rectangle(w->crb, 30 * w->app->hdpi, 244 * w->app->hdpi,
                                            550 * w->app->hdpi, 30 * w->app->hdpi, 0.5);
    cairo_fill_preserve (w->crb);
    boxShadowInset(w->crb,30 * w->app->hdpi,244 * w->app->hdpi,
                                            550 * w->app->hdpi, 30 * w->app->hdpi, true);
    cairo_fill (w->crb);

    cairo_set_source_rgba(w->crb, 0.1, 0.1, 0.1, 1);
    round_rectangle(w->crb, 30 * w->app->hdpi, 284 * w->app->hdpi,
                                            550 * w->app->hdpi, 30 * w->app->hdpi, 0.5);
    cairo_fill_preserve (w->crb);
    boxShadowInset(w->crb,30 * w->app->hdpi,284 * w->app->hdpi,
                                            550 * w->app->hdpi, 30 * w->app->hdpi, true);
    cairo_fill (w->crb);

    cairo_set_source_rgba(w->crb, 0.1, 0.1, 0.1, 1);
    round_rectangle(w->crb, 30 * w->app->hdpi, 324 * w->app->hdpi,
                                            550 * w->app->hdpi, 30 * w->app->hdpi, 0.5);
    cairo_fill_preserve (w->crb);
    boxShadowInset(w->crb,30 * w->app->hdpi,324 * w->app->hdpi,
                                            550 * w->app->hdpi, 30 * w->app->hdpi, true);
    cairo_fill (w->crb);

    cairo_set_source_rgba(w->crb, 0.1, 0.1, 0.1, 1);
    round_rectangle(w->crb, 30 * w->app->hdpi, 364 * w->app->hdpi,
                                            550 * w->app->hdpi, 30 * w->app->hdpi, 0.5);
    cairo_fill_preserve (w->crb);
    boxShadowInset(w->crb,30 * w->app->hdpi,364 * w->app->hdpi,
                                            550 * w->app->hdpi, 30 * w->app->hdpi, true);
    cairo_fill (w->crb);

    use_text_color_scheme(w, NORMAL_);
#ifdef USE_ATOM
    X11_UI* ui = (X11_UI*)w->parent_struct;
    X11_UI_Private_t *ps = (X11_UI_Private_t*)ui->private_ptr;
    if (strlen(ps->ma.filename)) {
        char label[124];
        memset(label, '\0', sizeof(char)*124);
        cairo_text_extents_t extents_f;
        cairo_set_font_size (w->crb, w->app->normal_font);
        int slen = strlen(basename(ps->ma.filename));
        
        if ((slen - 4) > 58) {
            utf8crop(label,basename(ps->ma.filename), 58);
            strcat(label,"...");
            tooltip_set_text(ps->ma.filebutton,basename(ps->ma.filename));
            ps->ma.filebutton->flags |= HAS_TOOLTIP;
        } else {
            strcpy(label, basename(ps->ma.filename));
            ps->ma.filebutton->flags &= ~HAS_TOOLTIP;
            hide_tooltip(ps->ma.filebutton);
        }

        cairo_text_extents(w->crb, label, &extents_f);
        double twf = extents_f.width/2.0;
        cairo_move_to (w->crb, max(100 * w->app->hdpi,(w->scale.init_width*0.5)-twf), 264 * w->app->hdpi );
        cairo_show_text(w->crb, label);
    }
    if (strlen(ps->mb.filename)) {
        char label[124];
        memset(label, '\0', sizeof(char)*124);
        cairo_text_extents_t extents_f;
        cairo_set_font_size (w->crb, w->app->normal_font);
        int slen = strlen(basename(ps->mb.filename));
        
        if ((slen - 4) > 58) {
            utf8crop(label,basename(ps->mb.filename), 58);
            strcat(label,"...");
            tooltip_set_text(ps->mb.filebutton,basename(ps->mb.filename));
            ps->mb.filebutton->flags |= HAS_TOOLTIP;
        } else {
            strcpy(label, basename(ps->mb.filename));
            ps->mb.filebutton->flags &= ~HAS_TOOLTIP;
            hide_tooltip(ps->mb.filebutton);
        }

        cairo_text_extents(w->crb, label, &extents_f);
        double twf = extents_f.width/2.0;
        cairo_move_to (w->crb, max(100 * w->app->hdpi,(w->scale.init_width*0.5)-twf), 304 * w->app->hdpi );
        cairo_show_text(w->crb, label);
    }
    if (strlen(ps->ir.filename)) {
        char label[124];
        memset(label, '\0', sizeof(char)*124);
        cairo_text_extents_t extents_f;
        cairo_set_font_size (w->crb, w->app->normal_font);
        int slen = strlen(basename(ps->ir.filename));
        
        if ((slen - 4) > 58) {
            utf8crop(label,basename(ps->ir.filename), 58);
            strcat(label,"...");
            tooltip_set_text(ps->ir.filebutton,basename(ps->ir.filename));
            ps->ir.filebutton->flags |= HAS_TOOLTIP;
        } else {
            strcpy(label, basename(ps->ir.filename));
            ps->ir.filebutton->flags &= ~HAS_TOOLTIP;
            hide_tooltip(ps->ir.filebutton);
        }

        cairo_text_extents(w->crb, label, &extents_f);
        double twf = extents_f.width/2.0;
        cairo_move_to (w->crb, max(100 * w->app->hdpi,(w->scale.init_width*0.5)-twf), 344 * w->app->hdpi );
        cairo_show_text(w->crb, label);
    }
    if (strlen(ps->ir1.filename)) {
        char label[124];
        memset(label, '\0', sizeof(char)*124);
        cairo_text_extents_t extents_f;
        cairo_set_font_size (w->crb, w->app->normal_font);
        int slen = strlen(basename(ps->ir1.filename));
        
        if ((slen - 4) > 58) {
            utf8crop(label,basename(ps->ir1.filename), 58);
            strcat(label,"...");
            tooltip_set_text(ps->ir1.filebutton,basename(ps->ir1.filename));
            ps->ir1.filebutton->flags |= HAS_TOOLTIP;
        } else {
            strcpy(label, basename(ps->ir1.filename));
            ps->ir1.filebutton->flags &= ~HAS_TOOLTIP;
            hide_tooltip(ps->ir1.filebutton);
        }

        cairo_text_extents(w->crb, label, &extents_f);
        double twf = extents_f.width/2.0;
        cairo_move_to (w->crb, max(100 * w->app->hdpi,(w->scale.init_width*0.5)-twf), 384 * w->app->hdpi );
        cairo_show_text(w->crb, label);
    }
#endif
#ifndef HIDE_NAME
    cairo_set_font_size (w->crb, w->app->big_font+8);

    cairo_move_to (w->crb, (w->scale.init_width*0.5)-tw-1, (w->scale.init_y+42 * w->app->hdpi)-1);
    cairo_text_path(w->crb, w->label);
    cairo_set_line_width(w->crb, 1);
    cairo_set_source_rgba(w->crb, 0.1, 0.1, 0.1, 1);
    cairo_stroke (w->crb);

    cairo_move_to (w->crb, (w->scale.init_width*0.5)-tw+1, (w->scale.init_y+42 * w->app->hdpi)+1);
    cairo_text_path(w->crb, w->label);
    cairo_set_line_width(w->crb, 1);
    cairo_set_source_rgba(w->crb, 0.33, 0.33, 0.33, 1);
    cairo_stroke (w->crb);

    cairo_set_source_rgba(w->crb, 0.2, 0.2, 0.2, 1);
    cairo_move_to (w->crb, (w->scale.init_width*0.5)-tw, w->scale.init_y+42 * w->app->hdpi);
    cairo_show_text(w->crb, w->label);

    cairo_move_to (w->crb, 50 * w->app->hdpi, w->scale.init_y+54 * w->app->hdpi);
    cairo_line_to (w->crb, 450 * w->app->hdpi, w->scale.init_y+54 * w->app->hdpi);
    //use_text_color_scheme(w, NORMAL_);
    cairo_stroke (w->crb);

    cairo_move_to (w->crb, 50 * w->app->hdpi, w->scale.init_y+55 * w->app->hdpi);
    cairo_line_to (w->crb, 560 * w->app->hdpi, w->scale.init_y+55 * w->app->hdpi);
    cairo_set_source_rgba(w->crb, 0.33, 0.33, 0.33, 1);
    //use_text_color_scheme(w, NORMAL_);
    cairo_stroke (w->crb);

    cairo_move_to (w->crb, 50 * w->app->hdpi, w->scale.init_y+53 * w->app->hdpi);
    cairo_line_to (w->crb, 560 * w->app->hdpi, w->scale.init_y+53 * w->app->hdpi);
    cairo_set_source_rgba(w->crb, 0.1, 0.1, 0.1, 1);
    //use_text_color_scheme(w, NORMAL_);
    cairo_stroke (w->crb);
#endif
    widget_reset_scale(w);
    cairo_new_path (w->crb);
}

// if controller value changed send notify to host
static void value_changed(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    X11_UI* ui = (X11_UI*)w->parent_struct;
    float v = adj_get_value(w->adj);
    sendValueChanged(ui, w->data, v);
}

void knobShadowOutset(cairo_t* const cr, int width, int height, int x, int y) {
    cairo_pattern_t *pat = cairo_pattern_create_linear (x, y, x + width, y + height);
    cairo_pattern_add_color_stop_rgba
        (pat, 0, 0.33, 0.33, 0.33, 1);
    cairo_pattern_add_color_stop_rgba
        (pat, 0.45, 0.33 * 0.6, 0.33 * 0.6, 0.33 * 0.6, 0.4);
    cairo_pattern_add_color_stop_rgba
        (pat, 0.65, 0.05 * 2.0, 0.05 * 2.0, 0.05 * 2.0, 0.4);
    cairo_pattern_add_color_stop_rgba 
        (pat, 1, 0.05, 0.05, 0.05, 1);
    cairo_pattern_set_extend(pat, CAIRO_EXTEND_NONE);
    cairo_set_source(cr, pat);
    cairo_fill_preserve (cr);
    cairo_pattern_destroy (pat);
}

void knobShadowInset(cairo_t* const cr, int width, int height, int x, int y) {
    cairo_pattern_t* pat = cairo_pattern_create_linear (x, y, x + width, y + height);
    cairo_pattern_add_color_stop_rgba
        (pat, 1, 0.33, 0.33, 0.33, 1);
    cairo_pattern_add_color_stop_rgba
        (pat, 0.65, 0.33 * 0.6, 0.33 * 0.6, 0.33 * 0.6, 0.4);
    cairo_pattern_add_color_stop_rgba
        (pat, 0.55, 0.05 * 2.0, 0.05 * 2.0, 0.05 * 2.0, 0.4);
    cairo_pattern_add_color_stop_rgba
        (pat, 0, 0.05, 0.05, 0.05, 1);
    cairo_pattern_set_extend(pat, CAIRO_EXTEND_NONE);
    cairo_set_source(cr, pat);
    cairo_fill (cr);
    cairo_pattern_destroy (pat);
}

static void draw_my_knob(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;    

    /** get size for the knob **/
    const int width = w->width;
    const int height = w->height - (w->height * 0.15);

    const int grow = (width > height) ? height:width;
    const int knob_x = grow-1;
    const int knob_y = grow-1;

    const int knobx = (width - knob_x) * 0.5;
    const int knobx1 = width* 0.5;

    const int knoby = (height - knob_y) * 0.5;
    const int knoby1 = height * 0.5;

    /** get geometric values for the knob **/
    const double scale_zero = 20 * (M_PI/180); // defines "dead zone"
    const double state = adj_get_state(w->adj);
    const double angle = scale_zero + state * 2 * (M_PI - scale_zero);

    const double pointer_off =knob_x/3.5;
    const double radius = min(knob_x-pointer_off, knob_y-pointer_off) / 2;
    const double lengh_x = (knobx+radius+pointer_off/2) - radius * sin(angle);
    const double lengh_y = (knoby+radius+pointer_off/2) + radius * cos(angle);
    const double radius_x = (knobx+radius+pointer_off/2) - radius * sin(angle);
    const double radius_y = (knoby+radius+pointer_off/2) + radius * cos(angle);

    /** draw the knob **/
    cairo_push_group (w->crb);

    cairo_arc(w->crb,knobx1, knoby1, knob_x/2.1, 0, 2 * M_PI );
    knobShadowOutset(w->crb, width, height, 0, 0);
    cairo_stroke_preserve (w->crb);
    cairo_new_path (w->crb);

    cairo_arc(w->crb,knobx1, knoby1, knob_x/2.4, 0, 2 * M_PI );
    knobShadowOutset(w->crb, width, height, 0, 0);
    cairo_set_line_width(w->crb,knobx1/10);
    cairo_set_source_rgba(w->crb, 0.05, 0.05, 0.05, 1);
    cairo_stroke_preserve (w->crb);
    cairo_new_path (w->crb);

    cairo_arc(w->crb,knobx1, knoby1, knob_x/3.1, 0, 2 * M_PI );
    use_bg_color_scheme(w, get_color_state(w));
    cairo_fill_preserve (w->crb);
    knobShadowInset(w->crb, width, height, 0, 0);
    cairo_new_path (w->crb);

    /** create a rotating pointer on the kob**/
    cairo_set_line_cap(w->crb, CAIRO_LINE_CAP_ROUND); 
    cairo_set_line_join(w->crb, CAIRO_LINE_JOIN_BEVEL);
    cairo_move_to(w->crb, radius_x, radius_y);
    cairo_line_to(w->crb,lengh_x,lengh_y);
    cairo_set_line_width(w->crb,knobx1/10);
    use_fg_color_scheme(w, NORMAL_);
    cairo_stroke_preserve(w->crb);
    cairo_new_path (w->crb);

    /** create a indicator ring around the knob **/
    const double add_angle = 90 * (M_PI / 180.);
    cairo_new_sub_path(w->crb);
    use_fg_color_scheme(w, NORMAL_);
    cairo_set_line_width(w->crb,3/w->scale.ascale);
    cairo_arc (w->crb, knobx1, knoby1, knob_x/2.4,
            add_angle + scale_zero, add_angle + angle);
    cairo_stroke(w->crb);

    /** show value on the kob**/
    use_text_color_scheme(w, get_color_state(w));
    cairo_text_extents_t extents;
    cairo_select_font_face (w->crb, "Sans", CAIRO_FONT_SLANT_NORMAL,
                               CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size (w->crb, w->app->normal_font/w->scale.ascale);
    char s[17];
    char sa[17];
    int o = 0;
    float value = adj_get_value(w->adj);
    float v = copysign(1, (int)(value * 10));
    value = copysign(value, v);
    if (fabs(w->adj->step)>0.99) {
        snprintf(s, 16,"%d",  (int) value);
        o = 4;
    } else if (fabs(w->adj->step)<0.09) {
        snprintf(s, 16, "%.2f", value);
        o = 1;
    } else {
        snprintf(s, 16, "%.1f", value);
    }
    snprintf(sa, strlen(s),"%s",  "000000000000000");
    cairo_text_extents(w->crb, sa, &extents);
    int wx = extents.width * 0.5;
    cairo_text_extents(w->crb, s, &extents);
    cairo_move_to (w->crb, knobx1 - wx - o, knoby1+extents.height/2);
    cairo_show_text(w->crb, s);
    cairo_new_path (w->crb);

    /** show label below the knob**/
    use_text_color_scheme(w, get_color_state(w));
    cairo_set_font_size (w->crb, (w->app->normal_font+4)/w->scale.ascale);
    cairo_text_extents(w->crb,w->label , &extents);
    cairo_move_to (w->crb, (width*0.5)-(extents.width/2), height + (height * 0.15)-(extents.height*0.1));
    cairo_show_text(w->crb, w->label);
    cairo_new_path (w->crb);

    cairo_pop_group_to_source (w->crb);
    cairo_paint (w->crb);
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

void roundrec(cairo_t *cr, double x, double y, double width, double height, double r) {
    cairo_arc(cr, x+r, y+r, r, M_PI, 3*M_PI/2);
    cairo_arc(cr, x+width-r, y+r, r, 3*M_PI/2, 0);
    cairo_arc(cr, x+width-r, y+height-r, r, 0, M_PI/2);
    cairo_arc(cr, x+r, y+height-r, r, M_PI/2, M_PI);
    cairo_close_path(cr);
}

void switchLight(cairo_t *cr, int x, int y, int w) {
    cairo_pattern_t *pat = cairo_pattern_create_linear (x, y, x + w, y);
    cairo_pattern_add_color_stop_rgba
        (pat, 1, 0.3, 0.55, 0.91, 1.0 * 0.8);
    cairo_pattern_add_color_stop_rgba
        (pat, 0.5, 0.3, 0.55, 0.91, 1.0 * 0.4);
    cairo_pattern_add_color_stop_rgba
        (pat, 0, 0.3, 0.55, 0.91, 1.0 * 0.2);
    cairo_pattern_set_extend(pat, CAIRO_EXTEND_NONE);
    cairo_set_source(cr, pat);
    cairo_fill_preserve (cr);
    cairo_pattern_destroy (pat);

}

static void draw_my_switch(void *w_, void* user_data) {
    Widget_t *wid = (Widget_t*)w_;    
    const int w = wid->width;
    const int h = wid->height * 0.5;
    const int state = (int)adj_get_state(wid->adj);

    const int centerH = h * 0.5;
    const int centerW = state ? w - centerH : centerH ;
    const int offset = h * 0.2;

    cairo_push_group (wid->crb);
    
    roundrec(wid->crb, 1, 1, w-2, h-2, centerH);
    knobShadowOutset(wid->crb, w  , h, 0, 0);
    cairo_stroke_preserve (wid->crb);

    cairo_new_path(wid->crb);
    roundrec(wid->crb, offset, offset, w - (offset * 2), h - (offset * 2), centerH-offset);
    cairo_set_source_rgba(wid->crb, 0.05, 0.05, 0.05, 1);
    cairo_fill_preserve(wid->crb);
    if (state) {
        switchLight(wid->crb, offset, offset, w - (offset * 2));
    }
    cairo_set_source_rgba(wid->crb, 0.05, 0.05, 0.05, 1);
    cairo_set_line_width(wid->crb,1);
    cairo_stroke_preserve (wid->crb);

    cairo_new_path(wid->crb);
    cairo_arc(wid->crb,centerW, centerH, h/2.8, 0, 2 * M_PI );
    use_bg_color_scheme(wid, PRELIGHT_);
    cairo_fill_preserve(wid->crb);
    knobShadowOutset(wid->crb, w * 0.5 , h, centerW - centerH, 0);
    cairo_set_source_rgba(wid->crb, 0.05, 0.05, 0.05, 1);
    cairo_set_line_width(wid->crb,1);
    cairo_stroke_preserve (wid->crb);

    cairo_new_path(wid->crb);
    cairo_arc(wid->crb,centerW, centerH, h/3.6, 0, 2 * M_PI );
    if(wid->state==1) use_bg_color_scheme(wid, PRELIGHT_);
    else use_bg_color_scheme(wid, NORMAL_);
    cairo_fill_preserve(wid->crb);
    knobShadowInset(wid->crb, w * 0.5 , h, centerW - centerH, 0);
    cairo_stroke (wid->crb);

    /** show label below the knob**/
    cairo_text_extents_t extents;
    cairo_select_font_face (wid->crb, "Sans", CAIRO_FONT_SLANT_NORMAL,
                               CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_source_rgba(wid->crb, 0.4, 0.4, 0.4, 1);
    cairo_set_font_size (wid->crb, wid->app->small_font/wid->scale.ascale);
    cairo_text_extents(wid->crb,wid->label , &extents);
    cairo_move_to (wid->crb, (w*0.5)-(extents.width/2), h*2 -(extents.height*0.4));
    cairo_show_text(wid->crb, wid->label);
    cairo_new_path (wid->crb);

    cairo_pop_group_to_source (wid->crb);
    cairo_paint (wid->crb);
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

static void draw_my_slider(void *w_, void* user_data) {
    Widget_t *wid = (Widget_t*)w_;    
    const int w = wid->width;
    const int h = wid->height * 0.5;
    const float state = adj_get_state(wid->adj);

    const int centerH = h * 0.5;
    const int centerW = (w-centerH -8.0) * (state) + centerH  ;
    const int offset = h * 0.2;

    cairo_push_group (wid->crb);
    
    roundrec(wid->crb, 1, 1, w-2, h-2, centerH);
    knobShadowOutset(wid->crb, w  , h, 0, 0);
    cairo_stroke_preserve (wid->crb);

    cairo_new_path(wid->crb);
    roundrec(wid->crb, offset, offset, w - (offset * 2), h - (offset * 2), centerH-offset);
    cairo_set_source_rgba(wid->crb, 0.05, 0.05, 0.05, 1);
    cairo_fill_preserve(wid->crb);

    cairo_set_source_rgba(wid->crb, 0.05, 0.05, 0.05, 1);
    cairo_set_line_width(wid->crb,1);
    cairo_stroke_preserve (wid->crb);

    roundrec(wid->crb, offset+1, offset+1, centerW -2, h - (offset * 2)-2, centerH-offset);
    switchLight(wid->crb, offset+1, offset+1, centerW - (offset * 2));
    cairo_stroke_preserve (wid->crb);

    cairo_new_path(wid->crb);
    cairo_arc(wid->crb,centerW, centerH, h/2.8, 0, 2 * M_PI );
    use_bg_color_scheme(wid, PRELIGHT_);
    cairo_fill_preserve(wid->crb);
    knobShadowOutset(wid->crb, w * 0.5 , h, centerW - centerH, 0);
    cairo_set_source_rgba(wid->crb, 0.05, 0.05, 0.05, 1);
    cairo_set_line_width(wid->crb,1);
    cairo_stroke_preserve (wid->crb);

    cairo_new_path(wid->crb);
    cairo_arc(wid->crb,centerW, centerH, h/3.6, 0, 2 * M_PI );
    if(wid->state==1) use_bg_color_scheme(wid, PRELIGHT_);
    else use_bg_color_scheme(wid, NORMAL_);
    cairo_fill_preserve(wid->crb);
    knobShadowInset(wid->crb, w * 0.5 , h, centerW - centerH, 0);
    cairo_stroke (wid->crb);

    /** show label below the knob**/
    cairo_text_extents_t extents;
    cairo_select_font_face (wid->crb, "Sans", CAIRO_FONT_SLANT_NORMAL,
                               CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_source_rgba(wid->crb, 0.4, 0.4, 0.4, 1);
    cairo_set_font_size (wid->crb, wid->app->small_font/wid->scale.ascale);
    cairo_text_extents(wid->crb,wid->label , &extents);
    cairo_move_to (wid->crb, (w*0.5)-(extents.width/2), h*2 -(extents.height*0.4));
    cairo_show_text(wid->crb, wid->label);
    cairo_new_path (wid->crb);

    cairo_pop_group_to_source (wid->crb);
    cairo_paint (wid->crb);
}

void slider_released(void *w_, void* button_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    XButtonEvent *xbutton = (XButtonEvent*)button_;
    if (xbutton->button != Button1) return;
    Adjustment_t *adj = w->adj;
    float value = adj->value;
    value = adj->value + adj->step;
    if (value>adj->max_value) value = adj->min_value;
    check_value_changed(adj, &value);
}

Widget_t* add_lv2_slider(Widget_t *w, Widget_t *p, int index, const char * label,
                                X11_UI* ui, int x, int y, int width, int height) {
    w = add_hslider(p, label, x, y, width, height);
    w->scale.gravity = CENTER;
    w->parent_struct = ui;
    w->data = index;
    w->func.expose_callback = draw_my_slider;
    w->func.button_release_callback = slider_released;
    w->func.value_changed_callback = value_changed;
    return w;
}

void draw_my_label(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    Metrics_t metrics;
    os_get_window_metrics(w, &metrics);
    int width = metrics.width;
    int height = metrics.height;
    float center = (float)width/2;
    if (!metrics.visible) return;

    cairo_text_extents_t extents;
    char s[64];
    float value = adj_get_value(w->adj);
    if (w->data == 22) snprintf(s, 63,"Latency: %.2fms",  value);
    else snprintf(s, 63,"Xruns: %.0f",  value);
    cairo_select_font_face (w->crb, "Sans", CAIRO_FONT_SLANT_NORMAL,
                               CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size (w->crb, w->app->small_font/w->scale.ascale);
    cairo_set_source_rgba(w->crb, 0.4, 0.4, 0.4, 1);
    cairo_text_extents(w->crb,"Latenco: 0.00ms" , &extents);
    cairo_move_to (w->crb, center-extents.width/2, height-(extents.height*0.4) );
    cairo_show_text(w->crb, s);
    cairo_new_path (w->crb);
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
    int width = metrics.width-3;
    int height = metrics.height-4;
    if (!metrics.visible) return;
    if (!w->state && (int)w->adj_y->value)
        w->state = 3;

    float offset = 0.0;
    if(w->state==0) {
        use_fg_color_scheme(w, NORMAL_);
    } else if(w->state==1) {
        use_fg_color_scheme(w, PRELIGHT_);
        offset = 1.0;
    } else if(w->state==2) {
        use_fg_color_scheme(w, SELECTED_);
        offset = 2.0;
    } else if(w->state==3) {
        use_fg_color_scheme(w, ACTIVE_);
        offset = 1.0;
    }
    use_text_color_scheme(w, get_color_state(w));
    int wa = width/1.1;
    int h = height/2.2;
    int wa1 = width/1.55;
    int h1 = height/1.3;
    int wa2 = width/2.8;
   
    cairo_move_to(w->crb, wa+offset, h+offset);
    cairo_line_to(w->crb, wa1+offset, h1+offset);
    cairo_line_to(w->crb, wa2+offset, h+offset);
    cairo_line_to(w->crb, wa+offset, h+offset);
    cairo_fill(w->crb);
}

Widget_t* add_lv2_button(Widget_t *w, Widget_t *p, const char * label,
                                X11_UI* ui, int x, int y, int width, int height) {
    w = add_combobox(p, label, x-415, y, width+415, height);
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

void draw_image_(Widget_t *w, int width_t, int height_t, float offset) {
    int width, height;
    os_get_surface_size(w->image, &width, &height);
    //double half_width = (width/height >=2) ? width*0.5 : width;
    double x = (double)width_t/(double)(width);
    double y = (double)height_t/(double)height;
    double x1 = (double)height/(double)height_t;
    double y1 = (double)(width)/(double)width_t;
    double off_set = offset*x1;
    cairo_scale(w->crb, x,y);
    if((int)w->adj_y->value) {
        roundrec(w->crb,0, 0, width, height, height* 0.22);
        cairo_set_source_rgba(w->crb, 0.3, 0.3, 0.3, 0.4);
        cairo_fill(w->crb);
    }
    cairo_set_source_surface (w->crb, w->image, off_set, off_set);
    cairo_rectangle(w->crb,0, 0, width, height);
    cairo_fill(w->crb);
    cairo_scale(w->crb, x1,y1);
}

void draw_i_button(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    if (!w) return;
    Metrics_t metrics;
    os_get_window_metrics(w, &metrics);
    int width = metrics.width-5;
    int height = metrics.height-5;
    if (!metrics.visible) return;
    if (w->image) {
        float offset = 0.0;
        if(w->state==1 && ! (int)w->adj_y->value) {
            offset = 1.0;
        } else if(w->state==1) {
            offset = 2.0;
        } else if(w->state==2) {
            offset = 2.0;
        } else if(w->state==3) {
            offset = 1.0;
        }
        
       draw_image_(w, width, height,offset);
   }
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
    fbutton->func.expose_callback = draw_i_button;
    return fbutton;
}


Widget_t* add_lv2_file_button(Widget_t *w, Widget_t *p, int index, const char * label,
                                X11_UI* ui, int x, int y, int width, int height) {
    w = add_my_file_button(p, x, y, width, height, "neural", "", ".nam|.aidax|.json");
    widget_get_png(w, LDVAR(neuraldir_png));
    w->data = index;
    return w;
}

Widget_t* add_lv2_irfile_button(Widget_t *w, Widget_t *p, int index, const char * label,
                                X11_UI* ui, int x, int y, int width, int height) {
    w = add_my_file_button(p, x, y, width, height, "IR File", "", ".wav");
    widget_get_png(w, LDVAR(wavdir_png));
    w->data = index;
    return w;
}

Widget_t* add_lv2_toggle_button(Widget_t *w, Widget_t *p, int index, const char * label,
                                X11_UI* ui, int x, int y, int width, int height) {
    w = add_image_toggle_button(p, "", x, y, width, height);
    w->parent_struct = ui;
    w->data = index;
    widget_get_png(w, LDVAR(norm_png));
    w->func.expose_callback = draw_i_button;
    w->func.value_changed_callback = value_changed;
    return w;
}

Widget_t* add_lv2_erase_button(Widget_t *w, Widget_t *p, int index, const char * label,
                                X11_UI* ui, int x, int y, int width, int height) {
    w = add_image_button(p, "", x, y, width, height);
    w->parent_struct = ui;
    w->data = index;
    widget_get_png(w, LDVAR(eject_png));
    w->func.expose_callback = draw_i_button;
    w->func.value_changed_callback = value_changed;
    return w;
}
