/*
 * Ratatouille.cpp
 *
 * SPDX-License-Identifier:  BSD-3-Clause
 *
 * Copyright (C) 2024 brummer <brummer@web.de>
 */

#ifdef _WIN32
#define MINGW_STDTHREAD_REDUNDANCY_WARNING
#endif

#include <atomic>
#include <mutex>
#include <condition_variable>
#include <cstdlib>
#include <cmath>
#include <iostream>
#include <cstring>
#include <thread>
#include <unistd.h>

#include <lv2/core/lv2.h>
#include <lv2/atom/atom.h>
#include <lv2/atom/util.h>
#include "lv2/atom/forge.h"
#include <lv2/midi/midi.h>
#include <lv2/urid/urid.h>
#include "lv2/patch/patch.h"
#include "lv2/options/options.h"
#include "lv2/state/state.h"
#include "lv2/worker/worker.h"
#include <lv2/buf-size/buf-size.h>

///////////////////////// MACRO SUPPORT ////////////////////////////////

#define PLUGIN_URI "urn:brummer:ratatouille"
#define XLV2__MODELFILE "urn:brummer:ratatouille#Neural_Model"
#define XLV2__MODELFILE1 "urn:brummer:ratatouille#Neural_Model1"
#define XLV2__IRFILE "urn:brummer:ratatouille#irfile"
#define XLV2__IRFILE1 "urn:brummer:ratatouille#irfile1"

#define XLV2__GUI "urn:brummer:ratatouille#gui"


#include "gx_resampler.h"

#include "dcblocker.cc"
#include "cdelay.cc"

#include "ModelerSelector.h"
#include "ParallelThread.h"

#include "fftconvolver.cc"
#include "fftconvolver.h"


namespace ratatouille {

/////////////////////////// DENORMAL PROTECTION   //////////////////////

class DenormalProtection {
private:
#ifdef __SSE__
    uint32_t  mxcsr_mask;
    uint32_t  mxcsr;
    uint32_t  old_mxcsr;
#endif

public:
    inline void set_() {
#ifdef __SSE__
        old_mxcsr = _mm_getcsr();
        mxcsr = old_mxcsr;
        _mm_setcsr((mxcsr | _MM_DENORMALS_ZERO_MASK | _MM_FLUSH_ZERO_MASK) & mxcsr_mask);
#endif
    };
    inline void reset_() {
#ifdef __SSE__
        _mm_setcsr(old_mxcsr);
#endif
    };

    inline DenormalProtection() {
#ifdef __SSE__
        mxcsr_mask = 0xffbf; // Default MXCSR mask
        mxcsr      = 0;
        uint8_t fxsave[512] __attribute__ ((aligned (16))); // Structure for storing FPU state with FXSAVE command

        memset(fxsave, 0, sizeof(fxsave));
        __builtin_ia32_fxsave(&fxsave);
        uint32_t mask = *(reinterpret_cast<uint32_t *>(&fxsave[0x1c])); // Obtain the MXCSR mask from FXSAVE structure
        if (mask != 0)
            mxcsr_mask = mask;
#endif
    };

    inline ~DenormalProtection() {};
};

////////////////////////////// PLUG-IN CLASS ///////////////////////////

class Xratatouille
{
private:
    dcblocker::Dsp*              dcb;
    cdeleay::Dsp*                cdelay;
    ModelerSelector              slotA;
    ModelerSelector              slotB;
    DoubleThreadConvolver        conv;
    DoubleThreadConvolver        conv1;
    ParallelThread               xrworker;
    ParallelThread               pro;
    DenormalProtection           MXCSR;

    int32_t                      rt_prio;
    int32_t                      rt_policy;
    float*                       input0;
    float*                       output0;
    float*                       _inputGain;
    float*                       _outputGain;
    float*                       _blend;
    float*                       _mix;
    float*                       _bufb;
    float*                       _normA;
    float*                       _normB;
    uint32_t                     normA;
    uint32_t                     normB;
    double                       fRec0[2];
    double                       fRec3[2];
    double                       fRec2[2];
    double                       fRec1[2];
    uint32_t                     bufsize;
    uint32_t                     s_rate;
    bool                         doit;

    std::string                  model_file;
    std::string                  model_file1;

    std::string                  ir_file;
    std::string                  ir_file1;

    std::atomic<bool>            _execute;
    std::atomic<bool>            _notify_ui;
    std::atomic<bool>            _restore;
    std::atomic<int>             _ab;
    std::atomic<bool>            _neuralA;
    std::atomic<bool>            _neuralB;

    std::condition_variable      Sync;
    std::mutex                   WMutex;


    // LV2 stuff
    LV2_URID_Map*                map;
    LV2_Worker_Schedule*         schedule;
    const LV2_Atom_Sequence*     control;
    LV2_Atom_Sequence*           notify;
    LV2_Atom_Forge               forge;
    LV2_Atom_Forge_Frame         notify_frame;

    LV2_URID                     xlv2_model_file;
    LV2_URID                     xlv2_model_file1;
    LV2_URID                     xlv2_ir_file;
    LV2_URID                     xlv2_ir_file1;
    LV2_URID                     xlv2_gui;
    LV2_URID                     atom_Object;
    LV2_URID                     atom_Int;
    LV2_URID                     atom_Float;
    LV2_URID                     atom_Bool;
    LV2_URID                     atom_Vector;
    LV2_URID                     atom_Path;
    LV2_URID                     atom_String;
    LV2_URID                     atom_URID;
    LV2_URID                     atom_eventTransfer;
    LV2_URID                     patch_Put;
    LV2_URID                     patch_Get;
    LV2_URID                     patch_Set;
    LV2_URID                     patch_property;
    LV2_URID                     patch_value;
    // private functions
    inline void run_dsp_(uint32_t n_samples);
    inline void connect_(uint32_t port,void* data);
    inline void init_dsp_(uint32_t rate);
    inline void connect_all__ports(uint32_t port, void* data);
    inline void activate_f();
    inline void clean_up();
    inline void do_work_mono();
    inline void deactivate_f();
    inline void processSlotB();
    inline void processConv1();
    inline void map_uris(LV2_URID_Map* map);
    inline LV2_Atom* write_set_file(LV2_Atom_Forge* forge,
            const LV2_URID xlv2_model, const char* filename);
    inline const LV2_Atom* read_set_file(const LV2_Atom_Object* obj);

public:
    // LV2 Descriptor
    static const LV2_Descriptor descriptor;
    static const void* extension_data(const char* uri);
    // static wrapper to private functions
    static void deactivate(LV2_Handle instance);
    static void cleanup(LV2_Handle instance);
    static void run(LV2_Handle instance, uint32_t n_samples);
    static void activate(LV2_Handle instance);
    static void connect_port(LV2_Handle instance, uint32_t port, void* data);

    static LV2_State_Status save_state(LV2_Handle instance,
                                       LV2_State_Store_Function store,
                                       LV2_State_Handle handle, uint32_t flags,
                                       const LV2_Feature* const* features);

    static LV2_State_Status restore_state(LV2_Handle instance,
                                          LV2_State_Retrieve_Function retrieve,
                                          LV2_State_Handle handle, uint32_t flags,
                                          const LV2_Feature* const*   features);

    static LV2_Handle instantiate(const LV2_Descriptor* descriptor,
                                double rate, const char* bundle_path,
                                const LV2_Feature* const* features);
  
    static LV2_Worker_Status work(LV2_Handle                 instance,
                                LV2_Worker_Respond_Function respond,
                                LV2_Worker_Respond_Handle   handle,
                                uint32_t size, const void*    data);
  
    static LV2_Worker_Status work_response(LV2_Handle  instance,
                                         uint32_t    size,
                                         const void* data);
    Xratatouille();
    ~Xratatouille();
};

// constructor
Xratatouille::Xratatouille() :
    dcb(dcblocker::plugin()),
    cdelay(cdeleay::plugin()),
    slotA(&Sync),
    slotB(&Sync),
    conv(DoubleThreadConvolver()),
    conv1(DoubleThreadConvolver()),
    rt_prio(0),
    rt_policy(0),
    input0(NULL),
    output0(NULL),
    _blend(0),
    _mix(0),
    _normA(0),
    _normB(0) {
        xrworker.start();
        xrworker.process.set<&Xratatouille::do_work_mono>(*this);
        //xrworker.process = [=] () {do_work_mono();};
        pro.start();
        };

// destructor
Xratatouille::~Xratatouille() {
    dcb->del_instance(dcb);
    cdelay->del_instance(cdelay);
    conv.stop_process();
    conv.cleanup();
    conv1.stop_process();
    conv1.cleanup();
    xrworker.stop();
    pro.stop();
};

///////////////////////// PRIVATE CLASS  FUNCTIONS /////////////////////

inline void Xratatouille::map_uris(LV2_URID_Map* map) {
    xlv2_model_file =       map->map(map->handle, XLV2__MODELFILE);
    xlv2_model_file1 =      map->map(map->handle, XLV2__MODELFILE1);
    xlv2_ir_file =          map->map(map->handle, XLV2__IRFILE);
    xlv2_ir_file1 =         map->map(map->handle, XLV2__IRFILE1);
    xlv2_gui =              map->map(map->handle, XLV2__GUI);
    atom_Object =           map->map(map->handle, LV2_ATOM__Object);
    atom_Int =              map->map(map->handle, LV2_ATOM__Int);
    atom_Float =            map->map(map->handle, LV2_ATOM__Float);
    atom_Bool =             map->map(map->handle, LV2_ATOM__Bool);
    atom_Vector =           map->map(map->handle, LV2_ATOM__Vector);
    atom_Path =             map->map(map->handle, LV2_ATOM__Path);
    atom_String =           map->map(map->handle, LV2_ATOM__String);
    atom_URID =             map->map(map->handle, LV2_ATOM__URID);
    atom_eventTransfer =    map->map(map->handle, LV2_ATOM__eventTransfer);
    patch_Put =             map->map(map->handle, LV2_PATCH__Put);
    patch_Get =             map->map(map->handle, LV2_PATCH__Get);
    patch_Set =             map->map(map->handle, LV2_PATCH__Set);
    patch_property =        map->map(map->handle, LV2_PATCH__property);
    patch_value =           map->map(map->handle, LV2_PATCH__value);
}

void Xratatouille::init_dsp_(uint32_t rate)
{
    s_rate = rate;
    dcb->init(rate);
    cdelay->init(rate);
    slotA.init(rate);
    slotB.init(rate);

    if (!rt_policy) rt_policy = 1; //SCHED_FIFO;
    pro.setThreadName("RT");
    pro.setPriority(rt_prio, rt_policy);
    pro.process.set<&Xratatouille::processSlotB>(*this);

    model_file = "None";
    model_file1 = "None";

    ir_file = "None";
    ir_file1 = "None";
    bufsize = 0;

    _execute.store(false, std::memory_order_release);
    _notify_ui.store(false, std::memory_order_release);
    _restore.store(false, std::memory_order_release);
    _ab.store(0, std::memory_order_release);
    _neuralA.store(false, std::memory_order_release);
    _neuralB.store(false, std::memory_order_release);

    for (int l0 = 0; l0 < 2; l0 = l0 + 1) fRec0[l0] = 0.0;
    for (int l0 = 0; l0 < 2; l0 = l0 + 1) fRec3[l0] = 0.0;
    for (int l0 = 0; l0 < 2; l0 = l0 + 1) fRec2[l0] = 0.0;
    for (int l0 = 0; l0 < 2; l0 = l0 + 1) fRec1[l0] = 0.0;
}

// connect the Ports used by the plug-in class
void Xratatouille::connect_(uint32_t port,void* data)
{
    switch (port)
    {
        case 0:
            input0 = static_cast<float*>(data);
            break;
        case 1:
            output0 = static_cast<float*>(data);
            break;
        case 2:
            _inputGain = static_cast<float*>(data);
            break;
        case 3:
            _outputGain = static_cast<float*>(data);
            break;
        case 4:
            _blend = static_cast<float*>(data);
            break;
        case 5:
            control = (const LV2_Atom_Sequence*)data;
            break;
        case 6:
            notify = (LV2_Atom_Sequence*)data;
            break;
        case 7:
            _mix = static_cast<float*>(data);
            break;
        case 9:
            _normA = static_cast<float*>(data);
            break;
        case 10:
            _normB = static_cast<float*>(data);
            break;
        default:
            break;
    }
}

void Xratatouille::activate_f()
{
    // allocate the internal DSP mem
}

void Xratatouille::clean_up()
{
    for (int l0 = 0; l0 < 2; l0 = l0 + 1) fRec2[l0] = 0.0;
    for (int l0 = 0; l0 < 2; l0 = l0 + 1) fRec1[l0] = 0.0;
    // delete the internal DSP mem
}

void Xratatouille::deactivate_f()
{
    // delete the internal DSP mem
}

void Xratatouille::do_work_mono()
{
    // load Model in slot A
    if (_ab.load(std::memory_order_acquire) == 1) {
        slotA.setModelFile(model_file);
        if (!slotA.loadModel()) {
            model_file = "None";
            _neuralA.store(false, std::memory_order_release);
        } else {
            _neuralA.store(true, std::memory_order_release);
        }
    // load Model in slot B
    } else if (_ab.load(std::memory_order_acquire) == 2) {
        slotB.setModelFile(model_file1);
        if (!slotB.loadModel()) {
            model_file1 = "None";
            _neuralB.store(false, std::memory_order_release);
        } else {
            _neuralB.store(true, std::memory_order_release);
        }
    // load Models in slots A and B
    } else if (_ab.load(std::memory_order_acquire) == 3) {
        slotA.setModelFile(model_file);
        if (!slotA.loadModel()) {
            model_file = "None";
            _neuralA.store(false, std::memory_order_release);
        } else {
            _neuralA.store(true, std::memory_order_release);
        }
        slotB.setModelFile(model_file1);
        if (!slotB.loadModel()) {
            model_file1 = "None";
            _neuralB.store(false, std::memory_order_release);
        } else {
            _neuralB.store(true, std::memory_order_release);
        }
    // load IR file in first convolver
    } else if (_ab.load(std::memory_order_acquire) == 7) {
        if (conv.is_runnable()) {
            conv.set_not_runnable();
            conv.stop_process();
            std::unique_lock<std::mutex> lk(WMutex);
            Sync.wait(lk);
        }

        conv.cleanup();
        conv.set_samplerate(s_rate);
        conv.set_buffersize(bufsize);

        conv.configure(ir_file, 1.0, 0, 0, 0, 0, 0);
        while (!conv.checkstate());
        if(!conv.start(rt_prio, rt_policy)) {
            ir_file = "None";
            printf("impulse convolver update fail\n");
        }
    // load IR file in second convolver
    } else if (_ab.load(std::memory_order_acquire) == 8) {
        if (conv1.is_runnable()) {
            conv1.set_not_runnable();
            conv1.stop_process();
            std::unique_lock<std::mutex> lk(WMutex);
            Sync.wait(lk);
        }

        conv1.cleanup();
        conv1.set_samplerate(s_rate);
        conv1.set_buffersize(bufsize);

        conv1.configure(ir_file1, 1.0, 0, 0, 0, 0, 0);
        while (!conv1.checkstate());
        if(!conv1.start(rt_prio, rt_policy)) {
            ir_file1 = "None";
            printf("impulse convolver1 update fail\n");
        }
    // load all models and IR files
    } else if (_ab.load(std::memory_order_acquire) > 10) {
        if (model_file != "None") {
            slotA.setModelFile(model_file);
            if (!slotA.loadModel()) {
                model_file = "None";
                _neuralA.store(false, std::memory_order_release);
            } else {
                _neuralA.store(true, std::memory_order_release);
            }
        } 
        if (model_file1 != "None") {
            slotB.setModelFile(model_file1);
            if (!slotB.loadModel()) {
                model_file1 = "None";
                _neuralB.store(false, std::memory_order_release);
            } else {
                _neuralB.store(true, std::memory_order_release);
            }
        } 

        if (ir_file != "None") {
            if (conv.is_runnable()) {
                conv.set_not_runnable();
                conv.stop_process();
                std::unique_lock<std::mutex> lk(WMutex);
                Sync.wait(lk);
            }

            conv.cleanup();
            conv.set_samplerate(s_rate);
            conv.set_buffersize(bufsize);

            conv.configure(ir_file, 1.0, 0, 0, 0, 0, 0);
            while (!conv.checkstate());
            if(!conv.start(rt_prio, rt_policy)) {
                ir_file = "None";
                printf("impulse convolver update fail\n");
            }
        } else {
            if (conv.is_runnable()) {
                conv.set_not_runnable();
                conv.stop_process();
            }            
        }
        if (ir_file1 != "None") {
            if (conv1.is_runnable()) {
                conv1.set_not_runnable();
                conv1.stop_process();
                std::unique_lock<std::mutex> lk(WMutex);
                Sync.wait(lk);
            }

            conv1.cleanup();
            conv1.set_samplerate(s_rate);
            conv1.set_buffersize(bufsize);

            conv1.configure(ir_file1, 1.0, 0, 0, 0, 0, 0);
            while (!conv1.checkstate());
            if(!conv1.start(rt_prio, rt_policy)) {
                ir_file1 = "None";
                printf("impulse convolver1 update fail\n");
            }
        } else {
            if (conv1.is_runnable()) {
                conv1.set_not_runnable();
                conv1.stop_process();
            }            
        }
    }
    // set wait function time out for parallel processor thread
    pro.setTimeOut(std::max(100,static_cast<int>((bufsize/(s_rate*0.000001))*0.1)));
    // set flag that work is done ready
    _execute.store(false, std::memory_order_release);
    // set flag that GUI need information about changed state
    _notify_ui.store(true, std::memory_order_release);
}

// prepare atom message with file path
inline LV2_Atom* Xratatouille::write_set_file(LV2_Atom_Forge* forge,
                    const LV2_URID xlv2_model, const char* filename) {

    LV2_Atom_Forge_Frame frame;
    lv2_atom_forge_frame_time(forge, 0);
    LV2_Atom* set = (LV2_Atom*)lv2_atom_forge_object(
                        forge, &frame, 1, patch_Set);

    lv2_atom_forge_key(forge, patch_property);
    lv2_atom_forge_urid(forge, xlv2_model);
    lv2_atom_forge_key(forge, patch_value);
    lv2_atom_forge_path(forge, filename, strlen(filename) + 1);

    lv2_atom_forge_pop(forge, &frame);
    return set;
}

// read atom message with file path
inline const LV2_Atom* Xratatouille::read_set_file(const LV2_Atom_Object* obj) {
    if (obj->body.otype != patch_Set) {
        return NULL;
    }

    const LV2_Atom* property = NULL;
    lv2_atom_object_get(obj, patch_property, &property, 0);

    if (property && (property->type == atom_URID)) {
        if (((LV2_Atom_URID*)property)->body == xlv2_model_file)
            _ab.store(1, std::memory_order_release);
        else if (((LV2_Atom_URID*)property)->body == xlv2_model_file1)
            _ab.store(2, std::memory_order_release);
        else if (((LV2_Atom_URID*)property)->body == xlv2_ir_file)
            _ab.store(7, std::memory_order_release);
        else if (((LV2_Atom_URID*)property)->body == xlv2_ir_file1)
            _ab.store(8, std::memory_order_release);
        else return NULL;
    }

    const LV2_Atom* file_path = NULL;
    lv2_atom_object_get(obj, patch_value, &file_path, 0);
    if (!file_path || (file_path->type != atom_Path)) {
        return NULL;
    }

    return file_path;
}

// process slotB in parallel thread
inline void Xratatouille::processSlotB() {
    slotB.compute(bufsize, _bufb, _bufb);
}

// process second convolver in parallel thread
inline void Xratatouille::processConv1() {
    conv1.compute(bufsize, _bufb, _bufb);
}

void Xratatouille::run_dsp_(uint32_t n_samples)
{
    if(n_samples<1) return;
    MXCSR.set_();
    const uint32_t notify_capacity = this->notify->atom.size;
    lv2_atom_forge_set_buffer(&forge, (uint8_t*)notify, notify_capacity);
    lv2_atom_forge_sequence_head(&forge, &notify_frame, 0);

    LV2_ATOM_SEQUENCE_FOREACH(control, ev) {
        if (lv2_atom_forge_is_object_type(&forge, ev->body.type)) {
            const LV2_Atom_Object* obj = (LV2_Atom_Object*)&ev->body;
            if (obj->body.otype == patch_Get) {
                if (model_file != "None")
                    write_set_file(&forge, xlv2_model_file, model_file.data());
                if (model_file1 != "None")
                    write_set_file(&forge, xlv2_model_file1, model_file1.data());
                if (ir_file != "None")
                    write_set_file(&forge, xlv2_ir_file, ir_file.data());
                if (ir_file1 != "None")
                    write_set_file(&forge, xlv2_ir_file1, ir_file1.data());
           } else if (obj->body.otype == patch_Set) {
                const LV2_Atom* file_path = read_set_file(obj);
                if (file_path) {
                    if (_ab.load(std::memory_order_acquire) == 1)
                        model_file = (const char*)(file_path+1);
                    else if (_ab.load(std::memory_order_acquire) == 2)
                        model_file1 = (const char*)(file_path+1);
                    else if (_ab.load(std::memory_order_acquire) == 7)
                        ir_file = (const char*)(file_path+1);
                    else if (_ab.load(std::memory_order_acquire) == 8)
                        ir_file1 = (const char*)(file_path+1);
                    if (!_execute.load(std::memory_order_acquire)) {
                        bufsize = n_samples;
                        _execute.store(true, std::memory_order_release);
                        xrworker.runProcess();
                        //schedule->schedule_work(schedule->handle,  sizeof(bool), &doit);
                    }
                }
            }
        }
    }

    if (!_execute.load(std::memory_order_acquire) && _restore.load(std::memory_order_acquire)) {
        _execute.store(true, std::memory_order_release);
        bufsize = n_samples;
        xrworker.runProcess();
        //schedule->schedule_work(schedule->handle,  sizeof(bool), &doit);
        _restore.store(false, std::memory_order_release);
    }
    // check if normalisation is pressed for conv
    if (normA != static_cast<uint32_t>(*(_normA)) && !_execute.load(std::memory_order_acquire)) {
        normA = static_cast<uint32_t>(*(_normA));
        bufsize = n_samples;
        _ab.store(7, std::memory_order_release);
        conv.set_normalisation(normA);
        if (ir_file.compare("None") != 0) {
            _execute.store(true, std::memory_order_release);
            xrworker.runProcess();
            //schedule->schedule_work(schedule->handle,  sizeof(bool), &doit);
            _restore.store(false, std::memory_order_release);
        }
    }
    // check if normalisation is pressed for conv1
    if (normB != static_cast<uint32_t>(*(_normB)) && !_execute.load(std::memory_order_acquire)) {
        normB = static_cast<uint32_t>(*(_normB));
        bufsize = n_samples;
        _ab.store(8, std::memory_order_release);
        conv1.set_normalisation(normB);
        if (ir_file1.compare("None") != 0) {
            _execute.store(true, std::memory_order_release);
            xrworker.runProcess();
            //schedule->schedule_work(schedule->handle,  sizeof(bool), &doit);
            _restore.store(false, std::memory_order_release);
        }
    }

    // do inplace processing on default
    if(output0 != input0)
        memcpy(output0, input0, n_samples*sizeof(float));

    // get controller values from host
    double fSlow0 = 0.0010000000000000009 * std::pow(1e+01, 0.05 * double(*(_inputGain)));
    double fSlow3 = 0.0010000000000000009 * std::pow(1e+01, 0.05 * double(*(_outputGain)));
    double fSlow2 = 0.0010000000000000009 * double(*(_blend));
    double fSlow1 = 0.0010000000000000009 * double(*(_mix));

    if (_neuralA.load(std::memory_order_acquire) || _neuralB.load(std::memory_order_acquire)) {
        // input volume
        for (int i0 = 0; i0 < n_samples; i0 = i0 + 1) {
            fRec0[0] = fSlow0 + 0.999 * fRec0[1];
            output0[i0] = float(double(output0[i0]) * fRec0[0]);
            fRec0[1] = fRec0[0];
        }
    }

    float bufa[n_samples];
    memcpy(bufa, output0, n_samples*sizeof(float));
    float bufb[n_samples];
    memcpy(bufb, output0, n_samples*sizeof(float));
    bufsize = n_samples;

    // process slot B in parallel
    _bufb = bufb;
    if (_neuralB.load(std::memory_order_acquire) && pro.getProcess()) {
        pro.runProcess();
    } else {
        processSlotB();
    }

    // process slot A
    slotA.compute(n_samples, bufa, bufa);

    //wait for parallel processed slot B when needed
    if (_neuralB.load(std::memory_order_acquire)) {
        pro.processWait();
    }

    cdelay->compute(n_samples, bufb, bufb);

    // mix output when needed
    if (_neuralA.load(std::memory_order_acquire) && _neuralB.load(std::memory_order_acquire)) {
        for (int i0 = 0; i0 < n_samples; i0 = i0 + 1) {
            fRec2[0] = fSlow2 + 0.999 * fRec2[1];
            output0[i0] = bufa[i0] * (1.0 - fRec2[0]) + bufb[i0] * fRec2[0];
            fRec2[1] = fRec2[0];
        }
    } else if (_neuralA.load(std::memory_order_acquire)) {
        memcpy(output0, bufa, n_samples*sizeof(float));
    } else if (_neuralB.load(std::memory_order_acquire)) {
        memcpy(output0, bufb, n_samples*sizeof(float));
    }

    if (_neuralA.load(std::memory_order_acquire) || _neuralB.load(std::memory_order_acquire)) {
        // output volume
        for (int i0 = 0; i0 < n_samples; i0 = i0 + 1) {
            fRec3[0] = fSlow3 + 0.999 * fRec3[1];
            output0[i0] = float(double(output0[i0]) * fRec3[0]);
            fRec3[1] = fRec3[0];
        }
    }

    // run dcblocker
    dcb->compute(n_samples, output0, output0);

    // set buffer for mix control
    memcpy(bufa, output0, n_samples*sizeof(float));
    memcpy(bufb, output0, n_samples*sizeof(float));

    // process conv
    if (!_execute.load(std::memory_order_acquire) && conv.is_runnable())
        conv.compute(n_samples, bufa, bufa);

    // process conv1
    if (!_execute.load(std::memory_order_acquire) && conv1.is_runnable())
        conv1.compute(n_samples, bufb, bufb);

    // mix output when needed
    if ((!_execute.load(std::memory_order_acquire) && conv.is_runnable()) && conv1.is_runnable()) {
        for (int i0 = 0; i0 < n_samples; i0 = i0 + 1) {
            fRec1[0] = fSlow1 + 0.999 * fRec1[1];
            output0[i0] = bufa[i0] * (1.0 - fRec1[0]) + bufb[i0] * fRec1[0];
            fRec1[1] = fRec1[0];
        }
    } else if (!_execute.load(std::memory_order_acquire) && conv.is_runnable()) {
        memcpy(output0, bufa, n_samples*sizeof(float));
    } else if (!_execute.load(std::memory_order_acquire) && conv1.is_runnable()) {
        memcpy(output0, bufb, n_samples*sizeof(float));
    }

    // notify UI on changed model files
    if (_notify_ui.load(std::memory_order_acquire)) {
        _notify_ui.store(false, std::memory_order_release);

        write_set_file(&forge, xlv2_model_file, model_file.data());
        write_set_file(&forge, xlv2_model_file1, model_file1.data());

        write_set_file(&forge, xlv2_ir_file, ir_file.data());
        write_set_file(&forge, xlv2_ir_file1, ir_file1.data());
        _ab.store(0, std::memory_order_release);
    }
    // notify neural modeller that process cycle is done
    Sync.notify_all();
    MXCSR.reset_();
}

void Xratatouille::connect_all__ports(uint32_t port, void* data)
{
    // connect the Ports used by the plug-in class
    connect_(port,data);
    slotA.connect(port,data);
    slotB.connect(port,data);
    cdelay->connect(port, data);
}

////////////////////// STATIC CLASS  FUNCTIONS  ////////////////////////

LV2_State_Status Xratatouille::save_state(LV2_Handle instance,
                                     LV2_State_Store_Function store,
                                     LV2_State_Handle handle, uint32_t flags,
                                     const LV2_Feature* const* features) {

    Xratatouille* self = static_cast<Xratatouille*>(instance);

    store(handle,self->xlv2_model_file,self->model_file.data(), strlen(self->model_file.data()) + 1,
          self->atom_String, LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE);

    store(handle,self->xlv2_model_file1,self->model_file1.data(), strlen(self->model_file1.data()) + 1,
          self->atom_String, LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE);

    store(handle,self->xlv2_ir_file,self->ir_file.data(), strlen(self->ir_file.data()) + 1,
          self->atom_String, LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE);

    store(handle,self->xlv2_ir_file1,self->ir_file1.data(), strlen(self->ir_file1.data()) + 1,
          self->atom_String, LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE);

    return LV2_STATE_SUCCESS;
}



LV2_State_Status Xratatouille::restore_state(LV2_Handle instance,
                                        LV2_State_Retrieve_Function retrieve,
                                        LV2_State_Handle handle, uint32_t flags,
                                        const LV2_Feature* const*   features) {

    Xratatouille* self = static_cast<Xratatouille*>(instance);

    size_t      size;
    uint32_t    type;
    uint32_t    fflags;
    const void* name = retrieve(handle, self->xlv2_model_file, &size, &type, &fflags);

    if (name) {
        self->model_file = (const char*)(name);
        if (!self->model_file.empty() && (self->model_file != "None")) {
            self->_ab.fetch_add(1, std::memory_order_relaxed);
        }
    }

    name = retrieve(handle, self->xlv2_model_file1, &size, &type, &fflags);

    if (name) {
        self->model_file1 = (const char*)(name);
        if (!self->model_file1.empty() && (self->model_file1 != "None")) {
            self->_ab.fetch_add(2, std::memory_order_relaxed);
        }
    }

    name = retrieve(handle, self->xlv2_ir_file, &size, &type, &fflags);

    if (name) {
        self->ir_file = (const char*)(name);
        if (!self->ir_file.empty() && (self->ir_file != "None")) {
            self->_ab.fetch_add(12, std::memory_order_relaxed);
        }
    }

    name = retrieve(handle, self->xlv2_ir_file1, &size, &type, &fflags);

    if (name) {
        self->ir_file1 = (const char*)(name);
        if (!self->ir_file1.empty() && (self->ir_file1 != "None")) {
            self->_ab.fetch_add(12, std::memory_order_relaxed);
        }
    }

    self-> _restore.store(true, std::memory_order_release);
    return LV2_STATE_SUCCESS;
}

LV2_Handle 
Xratatouille::instantiate(const LV2_Descriptor* descriptor,
                            double rate, const char* bundle_path,
                            const LV2_Feature* const* features)
{
    // init the plug-in class
    Xratatouille *self = new Xratatouille();
    if (!self) {
        return NULL;
    }

    const LV2_Options_Option* options  = NULL;
    uint32_t bufsize = 0;

    for (int32_t i = 0; features[i]; ++i) {
        if (!strcmp(features[i]->URI, LV2_URID__map)) {
            self->map = (LV2_URID_Map*)features[i]->data;
        }
        else if (!strcmp(features[i]->URI, LV2_WORKER__schedule)) {
            self->schedule = (LV2_Worker_Schedule*)features[i]->data;
        }
        else if (!strcmp(features[i]->URI, LV2_OPTIONS__options)) {
            options = (const LV2_Options_Option*)features[i]->data;
        }
    }

    if (!self->schedule) {
        fprintf(stderr, "Missing feature work:schedule.\n");
        self->_execute.store(true, std::memory_order_release);
    }

    if (!self->map) {
        fprintf(stderr, "Missing feature uri:map.\n");
    }
    else if (!options) {
        fprintf(stderr, "Missing feature options.\n");
    }
    else {
        LV2_URID bufsz_max = self->map->map(self->map->handle, LV2_BUF_SIZE__maxBlockLength);
        LV2_URID bufsz_    = self->map->map(self->map->handle,"http://lv2plug.in/ns/ext/buf-size#nominalBlockLength");
        LV2_URID atom_Int = self->map->map(self->map->handle, LV2_ATOM__Int);
        LV2_URID tshed_pol = self->map->map (self->map->handle, "http://ardour.org/lv2/threads/#schedPolicy");
        LV2_URID tshed_pri = self->map->map (self->map->handle, "http://ardour.org/lv2/threads/#schedPriority");

        for (const LV2_Options_Option* o = options; o->key; ++o) {
            if (o->context == LV2_OPTIONS_INSTANCE &&
              o->key == bufsz_ && o->type == atom_Int) {
                bufsize = *(const int32_t*)o->value;
            } else if (o->context == LV2_OPTIONS_INSTANCE &&
              o->key == bufsz_max && o->type == atom_Int) {
                if (!bufsize)
                    bufsize = *(const int32_t*)o->value;
            } else if (o->context == LV2_OPTIONS_INSTANCE &&
                o->key == tshed_pol && o->type == atom_Int) {
                self->rt_policy = *(const int32_t*)o->value;
            } else if (o->context == LV2_OPTIONS_INSTANCE &&
                o->key == tshed_pri && o->type == atom_Int) {
                self->rt_prio = *(const int32_t*)o->value;
            }
        }

        if (bufsize == 0) {
            fprintf(stderr, "No maximum buffer size given.\n");
        } else {
            self->bufsize = bufsize;
            printf("using block size: %d\n", bufsize);
        }
    }

    self->map_uris(self->map);
    lv2_atom_forge_init(&self->forge, self->map);
    self->init_dsp_((uint32_t)rate);

    return (LV2_Handle)self;
}

void Xratatouille::connect_port(LV2_Handle instance, 
                                   uint32_t port, void* data)
{
    // connect all ports
    static_cast<Xratatouille*>(instance)->connect_all__ports(port, data);
}

void Xratatouille::activate(LV2_Handle instance)
{
    // allocate needed mem
    static_cast<Xratatouille*>(instance)->activate_f();
}

void Xratatouille::run(LV2_Handle instance, uint32_t n_samples)
{
    // run dsp
    static_cast<Xratatouille*>(instance)->run_dsp_(n_samples);
}

void Xratatouille::deactivate(LV2_Handle instance)
{
    // free allocated mem
    static_cast<Xratatouille*>(instance)->deactivate_f();
}

void Xratatouille::cleanup(LV2_Handle instance)
{
    // well, clean up after us
    Xratatouille* self = static_cast<Xratatouille*>(instance);
    self->clean_up();
    delete self;
}

LV2_Worker_Status Xratatouille::work(LV2_Handle instance,
     LV2_Worker_Respond_Function respond,
     LV2_Worker_Respond_Handle   handle,
     uint32_t                    size,
     const void*                 data)
{
  static_cast<Xratatouille*>(instance)->do_work_mono();
  return LV2_WORKER_SUCCESS;
}

LV2_Worker_Status Xratatouille::work_response(LV2_Handle instance,
              uint32_t    size,
              const void* data)
{
  //printf("worker respose.\n");
  return LV2_WORKER_SUCCESS;
}

const void* Xratatouille::extension_data(const char* uri)
{
    static const LV2_Worker_Interface worker = { work, work_response, NULL };
    static const LV2_State_Interface  state  = { save_state, restore_state };

    if (!strcmp(uri, LV2_WORKER__interface)) {
        return &worker;
    }
    else if (!strcmp(uri, LV2_STATE__interface)) {
        return &state;
    }

    return NULL;
}

const LV2_Descriptor Xratatouille::descriptor =
{
    PLUGIN_URI ,
    Xratatouille::instantiate,
    Xratatouille::connect_port,
    Xratatouille::activate,
    Xratatouille::run,
    Xratatouille::deactivate,
    Xratatouille::cleanup,
    Xratatouille::extension_data
};

} // end namespace ratatouille

////////////////////////// LV2 SYMBOL EXPORT ///////////////////////////

LV2_SYMBOL_EXPORT
const LV2_Descriptor*
lv2_descriptor(uint32_t index)
{
    switch (index)
    {
        case 0:
            return &ratatouille::Xratatouille::descriptor;
        default:
            return NULL;
    }
}

///////////////////////////// FIN //////////////////////////////////////
