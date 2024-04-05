
class NeuralAmpMulti {
private:
    nam::DSP* modela;
    nam::DSP* modelb;
    gx_resample::FixedRateResampler smpa;
    gx_resample::FixedRateResampler smpb;
    std::atomic<bool> readyA;
    std::atomic<bool> readyB;
    int fSampleRate;
    int maSampleRate;
    int mbSampleRate;
    float* _fVslider0;
    float* _fVslider1;
    float* _fVslider2;
    double fRec0[2];
    double fRec1[2];
    double fRec2[2];
    int need_aresample;
    int need_bresample;
    float loudnessa;
    float loudnessb;
    bool is_inited;
    std::mutex WMutex;
    std::condition_variable *WCondVar;
public:
    std::string load_afile;
    std::string load_bfile;
    void clear_state_f();
    void init(unsigned int sample_rate);
    void connect(uint32_t port,void* data);
    void compute(int count, float *input0, float *output0);
    bool load_nam_afile();
    bool load_nam_bfile();
    void unload_nam_afile();
    void unload_nam_bfile();
    NeuralAmpMulti(std::condition_variable *var);
    ~NeuralAmpMulti();
};

NeuralAmpMulti::NeuralAmpMulti(std::condition_variable *var)
    : modela(nullptr), modelb(nullptr), smpa(), smpb(), WCondVar(var) {
    loudnessa = 0.0;
    loudnessb = 0.0;
    need_aresample = 0;
    need_bresample = 0;
    is_inited = false;
    readyA.store(false, std::memory_order_release);
    readyB.store(false, std::memory_order_release);
 }

NeuralAmpMulti::~NeuralAmpMulti() {
    delete modela;
    delete modelb;
}

inline void NeuralAmpMulti::clear_state_f()
{
    for (int l0 = 0; l0 < 2; l0 = l0 + 1) fRec0[l0] = 0.0;
    for (int l0 = 0; l0 < 2; l0 = l0 + 1) fRec1[l0] = 0.0;
    for (int l0 = 0; l0 < 2; l0 = l0 + 1) fRec2[l0] = 0.0;
}

inline void NeuralAmpMulti::init(unsigned int sample_rate)
{
    fSampleRate = sample_rate;
    clear_state_f();
    is_inited = true;
    load_nam_afile();
    load_nam_bfile();
}

// connect the Ports used by the plug-in class
void NeuralAmpMulti::connect(uint32_t port,void* data)
{
    switch ((PortIndex)port)
    {
        case 2:
            _fVslider0 = static_cast<float*>(data);
            break;
        case 3:
            _fVslider1 = static_cast<float*>(data);
            break;
        case 4:
            _fVslider2 = static_cast<float*>(data);
            break;
        default:
            break;
    }
}

void NeuralAmpMulti::compute(int count, float *input0, float *output0)
{
    if (!modela && !modelb) return;
    if (output0 != input0)
        memcpy(output0, input0, count*sizeof(float));
    double fSlow0 = 0.0010000000000000009 * std::pow(1e+01, 0.05 * double(*(_fVslider0)));
    double fSlow1 = 0.0010000000000000009 * std::pow(1e+01, 0.05 * double(*(_fVslider1)));
    double fSlow2 = 0.0010000000000000009 * double(*(_fVslider2));
    // input volume
    for (int i0 = 0; i0 < count; i0 = i0 + 1) {
        fRec0[0] = fSlow0 + 0.999 * fRec0[1];
        output0[i0] = float(double(output0[i0]) * fRec0[0]);
        fRec0[1] = fRec0[0];
    }
    float bufa[count];
    memcpy(bufa, output0, count*sizeof(float));
    float bufb[count];
    memcpy(bufb, output0, count*sizeof(float));
    // process model A
    if (modela && readyA.load(std::memory_order_acquire)) {
        if (need_aresample ) {
            int ReCounta = count;
            if (need_aresample == 1) {
                ReCounta = smpa.max_out_count(count);
            } else if (need_aresample == 2) {
                ReCounta = static_cast<int>(ceil((count*static_cast<double>(maSampleRate))/fSampleRate));
            }
            float bufa1[ReCounta];
            memset(bufa1, 0, ReCounta*sizeof(float));
            if (need_aresample == 1) {
                ReCounta = smpa.up(count, bufa, bufa1);
            } else if (need_aresample == 2) {
                smpa.down(bufa, bufa1);
            } else {
                memcpy(bufa1, bufa, ReCounta * sizeof(float));
            }
            modela->process(bufa1, bufa1, ReCounta);
            modela->finalize_(ReCounta);
            if (need_aresample == 1) {
                smpa.down(bufa1, bufa);
            } else if (need_aresample == 2) {
                smpa.up(ReCounta, bufa1, bufa);
            }
        } else {
            modela->process(bufa, bufa, count);
            modela->finalize_(count);
        }
    }
    // process model B
    if (modelb && readyB.load(std::memory_order_acquire)) {
        if (need_bresample) {
            int ReCountb = count;
            if (need_bresample == 1) {
                ReCountb = smpb.max_out_count(count);
            } else if (need_bresample == 2) {
                ReCountb = static_cast<int>(ceil((count*static_cast<double>(mbSampleRate))/fSampleRate));
            }

            float bufb1[ReCountb];
            memset(bufb1, 0, ReCountb*sizeof(float));

            if (need_bresample == 1) {
                ReCountb = smpb.up(count, bufb, bufb1);
            } else if (need_bresample == 2) {
                smpb.down(bufb, bufb1);
            } else {
                memcpy(bufb1, bufb, ReCountb * sizeof(float));
            }

            modelb->process(bufb1, bufb1, ReCountb);
            modelb->finalize_(ReCountb);

            if (need_bresample == 1) {
                smpb.down(bufb1, bufb);
            } else if (need_bresample == 2) {
                smpb.up(ReCountb, bufb1, bufb);
            }
        } else {
            modelb->process(bufb, bufb, count);
            modelb->finalize_(count);
        }
    }
    //mix model A/B
    if (modela && modelb && readyA.load(std::memory_order_acquire) &&
                            readyB.load(std::memory_order_acquire)) {
        for (int i0 = 0; i0 < count; i0 = i0 + 1) {
            fRec2[0] = fSlow2 + 0.999 * fRec2[1];
            output0[i0] = bufa[i0] * (1.0 - fRec2[0]) + bufb[i0] * fRec2[0];
            fRec2[1] = fRec2[0];
        }
    } else if (modela && readyA.load(std::memory_order_acquire)) {
        memcpy(output0, bufa, count*sizeof(float));
    } else if (modelb && readyB.load(std::memory_order_acquire)) {
        memcpy(output0, bufb, count*sizeof(float));
    }
    // output volume
    for (int i0 = 0; i0 < count; i0 = i0 + 1) {
        fRec1[0] = fSlow1 + 0.999 * fRec1[1];
        output0[i0] = float(double(output0[i0]) * fRec1[0]);
        fRec1[1] = fRec1[0];
    }
}

// non rt callback
bool NeuralAmpMulti::load_nam_afile() {
    if (!load_afile.empty() && is_inited) {
       // fprintf(stderr, "Load file %s\n", load_afile.c_str());
        std::unique_lock<std::mutex> lk(WMutex);
        readyA.store(false, std::memory_order_release);
        WCondVar->wait(lk);
        delete modela;
       // fprintf(stderr, "delete modela\n");
        modela = nullptr;
        need_aresample = 0;
        //clear_state_f();
        int32_t warmUpSize = 4096;
        try {
            modela = nam::get_dsp(std::string(load_afile)).release();
        } catch (const std::exception&) {
            load_afile = "None";
        }
        
        if (modela) {
           // fprintf(stderr, "load modela\n");
            if (modela->HasLoudness()) loudnessa = modela->GetLoudness();
            maSampleRate = static_cast<int>(modela->GetExpectedSampleRate());
            //model->SetLoudness(-15.0);
            if (maSampleRate <= 0) maSampleRate = 48000;
            if (maSampleRate > fSampleRate) {
                smpa.setup(fSampleRate, maSampleRate);
                need_aresample = 1;
            } else if (maSampleRate < fSampleRate) {
                smpa.setup(maSampleRate, fSampleRate);
                need_aresample = 2;
            } 
            float* buffer = new float[warmUpSize];
            memset(buffer, 0, warmUpSize * sizeof(float));

            modela->process(buffer, buffer, warmUpSize);
            modela->finalize_(warmUpSize);

            delete[] buffer;
            //fprintf(stderr, "sample rate = %i file = %i l = %f\n",fSampleRate, maSampleRate, loudness);
            //fprintf(stderr, "%s\n", load_file.c_str());
        } 
        readyA.store(true, std::memory_order_release);
    }
    if (modela) return true;
    return false;
}

// non rt callback
void NeuralAmpMulti::unload_nam_afile() {
    std::unique_lock<std::mutex> lk(WMutex);
    readyA.store(false, std::memory_order_release);
    WCondVar->wait(lk);
    delete modela;
   // fprintf(stderr, "delete modela\n");
    modela = nullptr;
    need_aresample = 0;
    //clear_state_f();
    load_afile = "None";
    readyA.store(true, std::memory_order_release);
}

// non rt callback
bool NeuralAmpMulti::load_nam_bfile() {
    if (!load_bfile.empty() && is_inited) {
      //  fprintf(stderr, "Load file %s\n", load_bfile.c_str());
        std::unique_lock<std::mutex> lk(WMutex);
        readyB.store(false, std::memory_order_release);
        WCondVar->wait(lk);
        delete modelb;
       // fprintf(stderr, "delete modelb\n");
        modelb = nullptr;
        need_bresample = 0;
        //clear_state_f();
        int32_t warmUpSize = 4096;
        try {
            modelb = nam::get_dsp(std::string(load_bfile)).release();
        } catch (const std::exception&) {
            load_bfile = "None";
        }
        
        if (modelb) {
           // fprintf(stderr, "load modelb\n");
            if (modelb->HasLoudness()) loudnessb = modelb->GetLoudness();
            mbSampleRate = static_cast<int>(modelb->GetExpectedSampleRate());
            //model->SetLoudness(-15.0);
            if (mbSampleRate <= 0) mbSampleRate = 48000;
            if (mbSampleRate > fSampleRate) {
                smpb.setup(fSampleRate, mbSampleRate);
                need_bresample = 1;
            } else if (mbSampleRate < fSampleRate) {
                smpb.setup(maSampleRate, fSampleRate);
                need_bresample = 2;
            } 
            float* buffer = new float[warmUpSize];
            memset(buffer, 0, warmUpSize * sizeof(float));

            modelb->process(buffer, buffer, warmUpSize);
            modelb->finalize_(warmUpSize);

            delete[] buffer;
            //fprintf(stderr, "sample rate = %i file = %i l = %f\n",fSampleRate, mbSampleRate, loudness);
            //fprintf(stderr, "%s\n", load_file.c_str());
        } 
        readyB.store(true, std::memory_order_release);
    }
    if (modelb) return true;
    return false;
}

// non rt callback
void NeuralAmpMulti::unload_nam_bfile() {
    std::unique_lock<std::mutex> lk(WMutex);
    readyB.store(false, std::memory_order_release);
    WCondVar->wait(lk);
    delete modelb;
   // fprintf(stderr, "delete modelb\n");
    modelb = nullptr;
    need_bresample = 0;
    //clear_state_f();
    load_bfile = "None";
    readyB.store(true, std::memory_order_release);
}
