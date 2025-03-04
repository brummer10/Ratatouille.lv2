/*
 * Ratatouille.h
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
#include <cstring>
#include <string>
#include <cmath>

#include <locale.h>

#include "engine.h"
#include "ParallelThread.h"
#define STANDALONE
#include "Ratatouille.c"

class Ratatouille
{
public:
    X11_UI* ui;
    ratatouille::Engine engine;
    int processCounter;
    std::atomic<bool>   workToDo;

    Ratatouille() : engine() {
        workToDo.store(false, std::memory_order_release);
        processCounter = 0;
        s_time = 0.0;
        engine.cdelay->connect(8,(void*) &s_time);
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
        getConfigFilePath();
    }

    ~Ratatouille() {
        free(ui->private_ptr);
        free(ui);
        fetch.stop();
        //cleanup();
    }

    void startGui() {
        main_init(&ui->main);
        set_custom_theme(ui);
        ui->win = create_window(&ui->main, os_get_root_window(&ui->main, IS_WINDOW), 0, 0, 610, 419);
        widget_set_title(ui->win, "Ratatouille");
        widget_set_icon_from_png(ui->win,LDVAR(neuraldir_png));
        ui->win->parent_struct = ui;
        plugin_create_controller_widgets(ui,"standalone");
        widget_show_all(ui->win);
    }

    void initEngine(uint32_t rate, int32_t prio, int32_t policy) {
        engine.init(rate, prio, policy);
        s_time = (1.0 / (double)rate) * 1000;
        fetch.startTimeout(120);
        fetch.set<Ratatouille, &Ratatouille::checkEngine>(this);
    }

    void readConfig(std::string name = "Default") {
        std::ifstream infile(configFile);
        std::string line;
        std::string key;
        std::string value;
        std::string LoadName = "None";
        if (infile.is_open()) {
            infile.imbue (std::locale("C"));
            while (std::getline(infile, line)) {
                std::istringstream buf(line);
                buf >> key;
                buf >> value;
                if (key.compare("[Preset]") == 0) LoadName = remove_sub(line, "[Preset] ");
                if (name.compare(LoadName) == 0) {
                    if (key.compare("[CONTROLS]") == 0) {
                        for (int i = 0; i < CONTROLS; i++) {
                            adj_set_value(ui->widget[i]->adj, check_stod(value));
                            buf >> value;
                        }
                    } else if (key.compare("[Model]") == 0) {
                        engine.model_file = remove_sub(line, "[Model] ");
                        engine._ab.fetch_add(1, std::memory_order_relaxed);
                    } else if (key.compare("[Model1]") == 0) {
                        engine.model_file1 = remove_sub(line, "[Model1] ");
                        engine._ab.fetch_add(2, std::memory_order_relaxed);
                    } else if (key.compare("[IrFile]") == 0) {
                        engine.ir_file = remove_sub(line, "[IrFile] ");
                        engine._ab.fetch_add(12, std::memory_order_relaxed);
                    } else if (key.compare("[IrFile1]") == 0) {
                        engine.ir_file1 = remove_sub(line, "[IrFile1] ");
                        engine._ab.fetch_add(12, std::memory_order_relaxed);
                    }
                }
                key.clear();
                value.clear();
            }
            infile.close();
            workToDo.store(true, std::memory_order_release);
        }
    }

    void cleanup() {
        fetch.stop();
        saveConfig();
        plugin_cleanup(ui);
        // Xputty free all memory used
        main_quit(&ui->main);
    }

private:
    ParallelThread fetch;
    std::string configFile;
    double s_time;

    float check_stod (const std::string& str) {
        char* point = localeconv()->decimal_point;
        if (std::string(".") != point) {
            std::size_t point_it = str.find(".");
            std::string temp_str = str;
            if (point_it != std::string::npos)
                temp_str.replace(point_it, point_it + 1, point);
            return std::stod(temp_str);
        } else return std::stod(str);
    }

    std::string remove_sub(std::string a, std::string b) {
        std::string::size_type fpos = a.find(b);
        if (fpos != std::string::npos )
            a.erase(a.begin() + fpos, a.begin() + fpos + b.length());
        return (a);
    }

    void getConfigFilePath() {
         if (getenv("XDG_CONFIG_HOME")) {
            std::string path = getenv("XDG_CONFIG_HOME");
            configFile = path + "/ratatouille.conf";
        } else {
        #if defined(__linux__) || defined(__FreeBSD__) || \
            defined(__NetBSD__) || defined(__OpenBSD__)
            std::string path = getenv("HOME");
            configFile = path +"/.config/ratatouille.conf";
        #else
            std::string path = getenv("APPDATA");
            configFile = path +"\\.config\\ratatouille.conf";
        #endif
       }
    }

    void saveConfig(std::string name = "Default",  bool append = false) {
        std::ofstream outfile(configFile, append ? std::ios::app : std::ios::trunc);
        if (outfile.is_open()) {
            outfile.imbue (std::locale("C"));
            outfile << "[Preset] " << name << std::endl;
            outfile << "[CONTROLS] ";
            for(int i = 0;i<CONTROLS;i++) {
                outfile << adj_get_value(ui->widget[i]->adj) << " ";
            }
            outfile << std::endl;
            outfile << "[Model] " << engine.model_file << std::endl;
            outfile << "[Model1] " << engine.model_file1 << std::endl;
            outfile << "[IrFile] " << engine.ir_file << std::endl;
            outfile << "[IrFile1] " << engine.ir_file1 << std::endl;
            outfile.close();
        }
    }

    // rebuild file menu when needed
    void rebuild_file_menu(ModelPicker *m) {
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
    inline void get_file(std::string fileName, ModelPicker *m) {
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
            }
        } else if (strcmp(m->filename, "None") != 0) {
            free(m->filename);
            m->filename = NULL;
            m->filename = strdup("None");
        }
    }

    // timeout loop to check output ports from engine
    void checkEngine() {
        // the early bird die
        if (processCounter < 2) {
            processCounter++;
            return;
        }

        #if defined(__linux__) || defined(__FreeBSD__) || \
            defined(__NetBSD__) || defined(__OpenBSD__)
        XLockDisplay(ui->main.dpy);
        #endif
        adj_set_value(ui->widget[17]->adj,(float) engine.latency * s_time);
        if (engine._notify_ui.load(std::memory_order_acquire)) {
            engine._notify_ui.store(false, std::memory_order_release);
            X11_UI_Private_t *ps = (X11_UI_Private_t*)ui->private_ptr;
            get_file(engine.model_file, &ps->ma);
            get_file(engine.model_file1, &ps->mb);
            get_file(engine.ir_file, &ps->ir);
            get_file(engine.ir_file1, &ps->ir1);
            expose_widget(ui->win);
        }
        #if defined(__linux__) || defined(__FreeBSD__) || \
            defined(__NetBSD__) || defined(__OpenBSD__)
        XFlush(ui->main.dpy);
        XUnlockDisplay(ui->main.dpy);
        #endif
        if (workToDo.load(std::memory_order_acquire)) {
            if (engine.xrworker.getProcess()) {
                workToDo.store(false, std::memory_order_release);
                engine._execute.store(true, std::memory_order_release);
                engine.xrworker.runProcess();
            }
        }
    }

};
