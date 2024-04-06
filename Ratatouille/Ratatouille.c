

#define CONTROLS 3

#define GUI_ELEMENTS 0

#define TAB_ELEMENTS 0


#define PLUGIN_UI_URI "urn:brummer:ratatouille_ui"


#include "lv2_plugin.h"


#define XLV2__neural_model "urn:brummer:ratatouille#NAM_Model"
#define XLV2__neural_model1 "urn:brummer:ratatouille#NAM_Model1"
#define XLV2__rtneural_model "urn:brummer:ratatouille#RTN_Model"
#define XLV2__rtneural_model1 "urn:brummer:ratatouille#RTN_Model1"

#define OBJ_BUF_SIZE 1024


typedef struct {
    LV2_URID neural_model;
    LV2_URID neural_model1;
    LV2_URID rtneural_model;
    LV2_URID rtneural_model1;
    LV2_URID atom_Object;
    LV2_URID atom_Int;
    LV2_URID atom_Float;
    LV2_URID atom_Bool;
    LV2_URID atom_Vector;
    LV2_URID atom_Path;
    LV2_URID atom_String;
    LV2_URID atom_URID;
    LV2_URID atom_eventTransfer;
    LV2_URID patch_Put;
    LV2_URID patch_Get;
    LV2_URID patch_Set;
    LV2_URID patch_property;
    LV2_URID patch_value;
} X11LV2URIs;

typedef struct {
    Widget_t *fbutton;
    Widget_t *filebutton;
    FilePicker *filepicker;
    char *filename;
    char *dir_name;
} ModelPicker;

typedef struct {
    LV2_Atom_Forge forge;
    X11LV2URIs   uris;
    ModelPicker ma;
    ModelPicker mb;
    char *fname;
} X11_UI_Private_t;

static inline void map_x11ui_uris(LV2_URID_Map* map, X11LV2URIs* uris) {
    uris->neural_model = map->map(map->handle, XLV2__neural_model);
    uris->neural_model1 = map->map(map->handle, XLV2__neural_model1);
    uris->rtneural_model = map->map(map->handle, XLV2__rtneural_model);
    uris->rtneural_model1 = map->map(map->handle, XLV2__rtneural_model1);
    uris->atom_Object = map->map(map->handle, LV2_ATOM__Object);
    uris->atom_Int = map->map(map->handle, LV2_ATOM__Int);
    uris->atom_Float = map->map(map->handle, LV2_ATOM__Float);
    uris->atom_Bool = map->map(map->handle, LV2_ATOM__Bool);
    uris->atom_Vector = map->map(map->handle, LV2_ATOM__Vector);
    uris->atom_Path = map->map(map->handle, LV2_ATOM__Path);
    uris->atom_String = map->map(map->handle, LV2_ATOM__String);
    uris->atom_URID = map->map(map->handle, LV2_ATOM__URID);
    uris->atom_eventTransfer = map->map(map->handle, LV2_ATOM__eventTransfer);
    uris->patch_Put = map->map(map->handle, LV2_PATCH__Put);
    uris->patch_Get = map->map(map->handle, LV2_PATCH__Get);
    uris->patch_Set = map->map(map->handle, LV2_PATCH__Set);
    uris->patch_property = map->map(map->handle, LV2_PATCH__property);
    uris->patch_value = map->map(map->handle, LV2_PATCH__value);
}

void set_costum_theme(Widget_t *w) {
    w->color_scheme->normal = (Colors) {
         /* cairo    / r  / g  / b  / a  /  */
        .fg =       { 0.850, 0.850, 0.850, 1.000},
        .bg =       { 0.100, 0.100, 0.100, 1.000},
        .base =     { 0.000, 0.000, 0.000, 1.000},
        .text =     { 0.682, 0.686, 0.686, 1.000},
        .shadow =   { 0.000, 0.000, 0.000, 0.200},
        .frame =    { 0.000, 0.000, 0.000, 1.000},
        .light =    { 0.100, 0.100, 0.100, 1.000}
    };

    w->color_scheme->prelight = (Colors) {
         /* cairo    / r  / g  / b  / a  /  */
        .fg =       { 1.000, 0.000, 1.000, 1.000},
        .bg =       { 0.250, 0.250, 0.250, 1.000},
        .base =     { 0.300, 0.300, 0.300, 1.000},
        .text =     { 1.000, 1.000, 1.000, 1.000},
        .shadow =   { 0.100, 0.100, 0.100, 0.400},
        .frame =    { 0.300, 0.300, 0.300, 1.000},
        .light =    { 0.300, 0.300, 0.300, 1.000}
    };

    w->color_scheme->selected = (Colors) {
         /* cairo    / r  / g  / b  / a  /  */
        .fg =       { 0.900, 0.900, 0.900, 1.000},
        .bg =       { 0.200, 0.200, 0.200, 1.000},
        .base =     { 0.500, 0.180, 0.180, 1.000},
        .text =     { 1.000, 1.000, 1.000, 1.000},
        .shadow =   { 0.800, 0.180, 0.180, 0.200},
        .frame =    { 0.500, 0.180, 0.180, 1.000},
        .light =    { 0.500, 0.180, 0.180, 1.000}
    };

    w->color_scheme->active = (Colors) {
         /* cairo    / r  / g  / b  / a  /  */
        .fg =       { 0.000, 1.000, 1.000, 1.000},
        .bg =       { 0.000, 0.000, 0.000, 1.000},
        .base =     { 0.180, 0.380, 0.380, 1.000},
        .text =     { 0.750, 0.750, 0.750, 1.000},
        .shadow =   { 0.180, 0.380, 0.380, 0.500},
        .frame =    { 0.180, 0.380, 0.380, 1.000},
        .light =    { 0.180, 0.380, 0.380, 1.000}
    };

    w->color_scheme->insensitive = (Colors) {
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

#include "lv2_plugin.cc"



int ends_with(const char* name, const char* extension) {
    const char* ldot = strrchr(name, '.');
    if (ldot != NULL) {
        size_t length = strlen(extension);
        return strncmp(ldot + 1, extension, length) == 0;
    }
    return 0;
}

static void notify_dsp(X11_UI *ui) {
    X11_UI_Private_t *ps = (X11_UI_Private_t*)ui->private_ptr;
    const X11LV2URIs* uris = &ps->uris;
    uint8_t obj_buf[OBJ_BUF_SIZE];
    lv2_atom_forge_set_buffer(&ps->forge, obj_buf, OBJ_BUF_SIZE);
    LV2_Atom_Forge_Frame frame;
    LV2_Atom* msg = (LV2_Atom*)lv2_atom_forge_object(&ps->forge, &frame, 0, uris->patch_Get);

    ui->write_function(ui->controller, 5, lv2_atom_total_size(msg),
                       ps->uris.atom_eventTransfer, msg);
}

static inline LV2_Atom* write_set_file(const LV2_URID urid, LV2_Atom_Forge* forge,
                const X11LV2URIs* uris, const char* filename) {
    LV2_Atom_Forge_Frame frame;
    LV2_Atom* set = (LV2_Atom*)lv2_atom_forge_object(
                        forge, &frame, 1, uris->patch_Set);
    lv2_atom_forge_key(forge, uris->patch_property);
    lv2_atom_forge_urid(forge, urid);
    lv2_atom_forge_key(forge, uris->patch_value);
    lv2_atom_forge_path(forge, filename, strlen(filename) + 1);
    lv2_atom_forge_pop(forge, &frame);
    return set;
}

static void file_load_response(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    ModelPicker* m = (ModelPicker*) w->parent_struct;
    Widget_t *p = (Widget_t*)w->parent;
    X11_UI *ui = (X11_UI*) p->parent_struct;
    X11_UI_Private_t *ps = (X11_UI_Private_t*)ui->private_ptr;
    if(user_data !=NULL) {
        int old = ends_with(m->filename, "nam");
        free(m->filename);
        m->filename = NULL;
        m->filename = strdup(*(const char**)user_data);
        LV2_URID urid;
        if ((strcmp(m->filename, "None") == 0) && old) {
            urid = ps->uris.neural_model;
        } else if (ends_with(m->filename, "nam")) {
            if ( m == &ps->ma) urid = ps->uris.neural_model;
            else  urid = ps->uris.neural_model1;
        } else {
            if ( m == &ps->ma) urid = ps->uris.rtneural_model;
            else urid = ps->uris.rtneural_model1;
        }
        uint8_t obj_buf[OBJ_BUF_SIZE];
        lv2_atom_forge_set_buffer(&ps->forge, obj_buf, OBJ_BUF_SIZE);
        LV2_Atom* msg = (LV2_Atom*)write_set_file(urid, &ps->forge, &ps->uris, m->filename);
        ui->write_function(ui->controller, 5, lv2_atom_total_size(msg),
                           ps->uris.atom_eventTransfer, msg);
        free(m->filename);
        m->filename = NULL;
        m->filename = strdup("None");
        expose_widget(ui->win);
        ui->loop_counter = 12;
    }
}

static void dummy_callback(void *w_, void* user_data) {
}

void set_ctl_val_from_host(Widget_t *w, float value) {
    xevfunc store = w->func.value_changed_callback;
    w->func.value_changed_callback = dummy_callback;
    adj_set_value(w->adj, value);
    w->func.value_changed_callback = *(*store);
}

void first_loop(X11_UI *ui) {
    notify_dsp(ui);
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

static void rebuild_file_menu(ModelPicker *m) {
    xevfunc store = m->fbutton->func.value_changed_callback;
    m->fbutton->func.value_changed_callback = dummy_callback;
    combobox_delete_entrys(m->fbutton);
    fp_get_files(m->filepicker, m->dir_name, 0, 1);
    int active_entry = m->filepicker->file_counter-1;
    int i = 0;
    for(;i<m->filepicker->file_counter;i++) {
        combobox_add_entry(m->fbutton, m->filepicker->file_names[i]);
        if (strcmp(basename(m->filename),m->filepicker->file_names[i]) == 0) 
            active_entry = i;
    }
    combobox_add_entry(m->fbutton, "None");
    adj_set_value(m->fbutton->adj, active_entry);
    combobox_set_menu_size(m->fbutton, min(14, m->filepicker->file_counter));
    m->fbutton->func.value_changed_callback = store;
}

void plugin_value_changed(X11_UI *ui, Widget_t *w, PortIndex index) {
    // do special stuff when needed
}

void plugin_set_window_size(int *w,int *h,const char * plugin_uri) {
    (*w) = 500; //set initial width of main window
    (*h) = 359; //set initial height of main window
}

const char* plugin_set_name() {
    return "Ratatouille"; //set plugin name to display on UI
}

void plugin_create_controller_widgets(X11_UI *ui, const char * plugin_uri) {
    set_costum_theme(ui->win);

    X11_UI_Private_t *ps =(X11_UI_Private_t*)malloc(sizeof(X11_UI_Private_t));
    ui->private_ptr = (void*)ps;
    map_x11ui_uris(ui->map, &ps->uris);
    lv2_atom_forge_init(&ps->forge, ui->map);
    ps->ma.filename = strdup("None");
    ps->mb.filename = strdup("None");
    ps->ma.dir_name = NULL;
    ps->mb.dir_name = NULL;
    ps->fname = NULL;
    ps->ma.filepicker = (FilePicker*)malloc(sizeof(FilePicker));
    fp_init(ps->ma.filepicker, "/");
    asprintf(&ps->ma.filepicker->filter ,"%s", ".nam|.aidax|.json");
    ps->ma.filepicker->use_filter = 1;
    ps->mb.filepicker = (FilePicker*)malloc(sizeof(FilePicker));
    fp_init(ps->mb.filepicker, "/");
    asprintf(&ps->mb.filepicker->filter ,"%s", ".nam|.aidax|.json");
    ps->mb.filepicker->use_filter = 1;

    ps->ma.filebutton = add_lv2_file_button (ps->ma.filebutton, ui->win, -1, "Neural Model", ui, 30,  254, 60, 30);
    ps->ma.filebutton->parent_struct = (void*)&ps->ma;
    ps->ma.filebutton->func.user_callback = file_load_response;

    ps->mb.filebutton = add_lv2_file_button (ps->mb.filebutton, ui->win, -2, "Neural Model", ui, 30,  294, 60, 30);
    ps->mb.filebutton->parent_struct = (void*)&ps->mb;
    ps->mb.filebutton->func.user_callback = file_load_response;

    ui->widget[0] = add_lv2_knob (ui->widget[0], ui->win, 2, "Input", ui, 55,  80, 120, 140);
    set_adjustment(ui->widget[0]->adj, 0.0, 0.0, -20.0, 20.0, 0.2, CL_CONTINUOS);
    set_widget_color(ui->widget[0], 0, 0, 0.3, 0.55, 0.91, 1.0);
    set_widget_color(ui->widget[0], 0, 3,  0.682, 0.686, 0.686, 1.0);

    ui->widget[2] = add_lv2_knob (ui->widget[2], ui->win, 4, "Blend", ui, 190,  80, 120, 140);
    set_adjustment(ui->widget[2]->adj, 0.5, 0.5, 0.0, 1.0, 0.01, CL_CONTINUOS);
    set_widget_color(ui->widget[2], 0, 0, 0.3, 0.55, 0.91, 1.0);
    set_widget_color(ui->widget[2], 0, 3,  0.682, 0.686, 0.686, 1.0);

    ui->widget[1] = add_lv2_knob (ui->widget[1], ui->win, 3, "Output ", ui, 325,  80, 120, 140);
    set_adjustment(ui->widget[1]->adj, 0.0, 0.0, -20.0, 20.0, 0.2, CL_CONTINUOS);
    set_widget_color(ui->widget[1], 0, 0, 0.3, 0.55, 0.91, 1.0);
    set_widget_color(ui->widget[1], 0, 3,  0.682, 0.686, 0.686, 1.0);

    ps->ma.fbutton = add_lv2_button(ps->ma.fbutton, ui->win, "", ui, 450,  254, 22, 30);
    ps->ma.fbutton->parent_struct = (void*)&ps->ma;
    combobox_set_pop_position(ps->ma.fbutton, 1);
    combobox_add_entry(ps->ma.fbutton, "None");
    ps->ma.fbutton->func.value_changed_callback = file_menu_callback;

    ps->mb.fbutton = add_lv2_button(ps->mb.fbutton, ui->win, "", ui, 450,  294, 22, 30);
    ps->mb.fbutton->parent_struct = (void*)&ps->mb;
    combobox_set_pop_position(ps->mb.fbutton, 1);
    combobox_add_entry(ps->mb.fbutton, "None");
    ps->mb.fbutton->func.value_changed_callback = file_menu_callback;
}

void plugin_cleanup(X11_UI *ui) {
    X11_UI_Private_t *ps = (X11_UI_Private_t*)ui->private_ptr;
    free(ps->fname);
    free(ps->ma.filename);
    free(ps->mb.filename);
    free(ps->ma.dir_name);
    free(ps->mb.dir_name);
    fp_free(ps->ma.filepicker);
    free(ps->ma.filepicker);
    fp_free(ps->mb.filepicker);
    free(ps->mb.filepicker);
    // clean up used sources when needed
}

Widget_t *get_widget_from_urid(X11_UI *ui, const LV2_URID urid) {
    X11_UI_Private_t *ps = (X11_UI_Private_t*)ui->private_ptr;
    if (urid == ps->uris.neural_model || urid == ps->uris.rtneural_model)
        return ps->ma.filebutton;
    else if (urid == ps->uris.neural_model1 || urid == ps->uris.rtneural_model1)
        return ps->mb.filebutton;
    return NULL;
}

static inline const LV2_Atom* read_set_file(const X11LV2URIs* uris, X11_UI *ui,
                                    ModelPicker **m, const LV2_Atom_Object* obj) {
    if (obj->body.otype != uris->patch_Set) {
        return NULL;
    }
    const LV2_Atom* property = NULL;
    lv2_atom_object_get(obj, uris->patch_property, &property, 0);
    if (property == NULL) return NULL;
    Widget_t *w = get_widget_from_urid(ui,((LV2_Atom_URID*)property)->body);
    if (!w || (property->type != uris->atom_URID)) {
        return NULL;
    }
    const LV2_Atom* file_path = NULL;
    lv2_atom_object_get(obj, uris->patch_value, &file_path, 0);
    if (!file_path || (file_path->type != uris->atom_Path)) {
        return NULL;
    }
    *(m) = (ModelPicker*) w->parent_struct;
    return file_path;
}

static inline void get_file(const LV2_Atom* file_uri, X11_UI* ui, ModelPicker *m) {
    const char* uri = (const char*)LV2_ATOM_BODY(file_uri);
    if (strlen(uri)) {
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
    }
}

void plugin_port_event(LV2UI_Handle handle, uint32_t port_index,
                        uint32_t buffer_size, uint32_t format,
                        const void * buffer) {
    X11_UI* ui = (X11_UI*)handle;
    X11_UI_Private_t *ps = (X11_UI_Private_t*)ui->private_ptr;
    const X11LV2URIs* uris = &ps->uris;
    if (format == ps->uris.atom_eventTransfer) {
        const LV2_Atom* atom = (LV2_Atom*)buffer;
        if (atom->type == ps->uris.atom_Object) {
            const LV2_Atom_Object* obj = (LV2_Atom_Object*)atom;
            if (obj->body.otype == uris->patch_Set) {
                ModelPicker *m = NULL;
                const LV2_Atom* file_uri = read_set_file(uris, ui, &m, obj);
                if (file_uri && m) {
                    get_file(file_uri, ui, m);
                }
            }
        }
    }
    // port value change message from host
    // do special stuff when needed
}

