
class RtNeuralMulti {
private:
    RTNeural::Model<float> *modela;
    RTNeural::Model<float> *modelb;
    gx_resample::FixedRateResampler smpa;
    gx_resample::FixedRateResampler smpb;
    std::atomic<bool> ready;
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
    void get_samplerate(std::string config_file, int *mSampleRate);
    bool load_json_afile();
    bool load_json_bfile();
    void unload_json_afile();
    void unload_json_bfile();
    RtNeuralMulti(std::condition_variable *var);
    ~RtNeuralMulti();
};

RtNeuralMulti::RtNeuralMulti(std::condition_variable *var)
    : modela(nullptr), modelb(nullptr), smpa(), smpb(), WCondVar(var) {
    need_aresample = 0;
    need_bresample = 0;
    is_inited = false;
    ready.store(false, std::memory_order_release);
 }

RtNeuralMulti::~RtNeuralMulti() {
    delete modela;
    delete modelb;
}

inline void RtNeuralMulti::clear_state_f()
{
    for (int l0 = 0; l0 < 2; l0 = l0 + 1) fRec0[l0] = 0.0;
    for (int l0 = 0; l0 < 2; l0 = l0 + 1) fRec1[l0] = 0.0;
    for (int l0 = 0; l0 < 2; l0 = l0 + 1) fRec2[l0] = 0.0;
}

inline void RtNeuralMulti::init(unsigned int sample_rate)
{
    fSampleRate = sample_rate;
    clear_state_f();
    is_inited = true;
    load_json_afile();
    load_json_bfile();
}

// connect the Ports used by the plug-in class
void RtNeuralMulti::connect(uint32_t port,void* data)
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

void RtNeuralMulti::compute(int count, float *input0, float *output0)
{
    if (!modela && !modelb) return;
    if (output0 != input0)
        memcpy(output0, input0, count*sizeof(float));
    double fSlow0 = 0.0010000000000000009 * std::pow(1e+01, 0.05 * double((*_fVslider0)));
    double fSlow1 = 0.0010000000000000009 * std::pow(1e+01, 0.05 * double((*_fVslider1)));
    double fSlow2 = 0.0010000000000000009 * double((*_fVslider2));
    for (int i0 = 0; i0 < count; i0 = i0 + 1) {
        fRec0[0] = fSlow0 + 0.999 * fRec0[1];
        output0[i0] = float(double(output0[i0]) * fRec0[0]);
        fRec0[1] = fRec0[0];
    }
    float bufa[count];
    memcpy(bufa, output0, count*sizeof(float));
    float bufb[count];
    memcpy(bufb, output0, count*sizeof(float));
    //process model A
    if (modela && ready.load(std::memory_order_acquire)) {
        if (need_aresample) {
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
            for (int i0 = 0; i0 < ReCounta; i0 = i0 + 1) {
                 bufa1[i0] = modela->forward (&bufa1[i0]);
            }
            if (need_aresample == 1) {
                smpa.down(bufa1, bufa);
            } else if (need_aresample == 2) {
                smpa.up(ReCounta, bufa1, bufa);
            }
        } else {
            for (int i0 = 0; i0 < count; i0 = i0 + 1) {
                 bufa[i0] = modela->forward (&bufa[i0]);
            }
        }
    }

    // process model B
    if (modelb && ready.load(std::memory_order_acquire)) {
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
            } else if (need_aresample == 2) {
                smpb.down(bufb, bufb1);
            } else {
                memcpy(bufb1, bufb, ReCountb * sizeof(float));
            }
            for (int i0 = 0; i0 < ReCountb; i0 = i0 + 1) {
                 bufb1[i0] = modelb->forward (&bufb1[i0]);
            }
            if (need_bresample == 1) {
                smpb.down(bufb1, bufb);
            } else if (need_bresample == 2) {
                smpb.up(ReCountb, bufb1, bufb);
            }
        } else {
            for (int i0 = 0; i0 < count; i0 = i0 + 1) {
                 bufb[i0] = modelb->forward (&bufb[i0]);
            }
        }
    }

    if (modela && modelb && ready.load(std::memory_order_acquire)) {
        for (int i0 = 0; i0 < count; i0 = i0 + 1) {
            fRec2[0] = fSlow2 + 0.999 * fRec2[1];
            output0[i0] = bufa[i0] * (1.0 - fRec2[0]) + bufb[i0] * fRec2[0];
            fRec2[1] = fRec2[0];
        }
    } else if (modela && ready.load(std::memory_order_acquire)) {
        memcpy(output0, bufa, count*sizeof(float));
    } else if (modelb && ready.load(std::memory_order_acquire)) {
        memcpy(output0, bufb, count*sizeof(float));
    }
    
    for (int i0 = 0; i0 < count; i0 = i0 + 1) {
        fRec1[0] = fSlow1 + 0.999 * fRec1[1];
        output0[i0] = float(double(output0[i0]) * fRec1[0]);
        fRec1[1] = fRec1[0];
    }
}

void RtNeuralMulti::get_samplerate(std::string config_file, int *mSampleRate) {
    std::ifstream infile(config_file);
    infile.imbue(std::locale::classic());
    std::string line;
    std::string key;
    std::string value;
    if (infile.is_open()) {
        while (std::getline(infile, line)) {
            std::istringstream buf(line);
            buf >> key;
            buf >> value;
            if (key.compare("\"samplerate\":") == 0) {
                value.erase(std::remove(value.begin(), value.end(), '\"'), value.end());
                (*mSampleRate) = std::stoi(value);
                break;
            }
            key.clear();
            value.clear();
        }
        infile.close();
    }
}

// non rt callback
bool RtNeuralMulti::load_json_afile() {
    if (!load_afile.empty() && is_inited) {
       // fprintf(stderr, "Load file %s\n", load_afile.c_str());
        std::unique_lock<std::mutex> lk(WMutex);
        ready.store(false, std::memory_order_release);
        WCondVar->wait(lk);
        delete modela;
       // fprintf(stderr, "delete model\n");
        modela = nullptr;
        maSampleRate = 0;
        need_aresample = 0;
        clear_state_f();
        try {
            get_samplerate(std::string(load_afile), &maSampleRate);
            std::ifstream jsonStream(std::string(load_afile), std::ifstream::binary);
            modela = RTNeural::json_parser::parseJson<float>(jsonStream).release();
        } catch (const std::exception&) {
            load_afile = "None";
        }
        
        if (modela) {
            modela->reset();
            if (maSampleRate <= 0) maSampleRate = 48000;
            if (maSampleRate > fSampleRate) {
                smpa.setup(fSampleRate, maSampleRate);
                need_aresample = 1;
            } else if (maSampleRate < fSampleRate) {
                smpa.setup(maSampleRate, fSampleRate);
                need_aresample = 2;
            } 
            // fprintf(stderr, "A: %s\n", load_afile.c_str());
       } 
        ready.store(true, std::memory_order_release);
    }
    if (modela) return true;
    return false;
}

// non rt callback
void RtNeuralMulti::unload_json_afile() {
    std::unique_lock<std::mutex> lk(WMutex);
    ready.store(false, std::memory_order_release);
    WCondVar->wait(lk);
    delete modela;
   // fprintf(stderr, "delete model\n");
    modela = nullptr;
    maSampleRate = 0;
    need_aresample = 0;
    clear_state_f();
    ready.store(true, std::memory_order_release);
}

// non rt callback
bool RtNeuralMulti::load_json_bfile() {
    if (!load_bfile.empty() && is_inited) {
        std::unique_lock<std::mutex> lk(WMutex);
        ready.store(false, std::memory_order_release);
        WCondVar->wait(lk);
        delete modelb;
        modelb = nullptr;
        mbSampleRate = 0;
        need_bresample = 0;
        clear_state_f();
        try {
            get_samplerate(std::string(load_bfile), &mbSampleRate);
            std::ifstream jsonStream(std::string(load_bfile), std::ifstream::binary);
            modelb = RTNeural::json_parser::parseJson<float>(jsonStream).release();
        } catch (const std::exception&) {
            load_bfile = "None";
        }
        
        if (modelb) {
            modelb->reset();
            if (mbSampleRate <= 0) mbSampleRate = 48000;
            if (mbSampleRate > fSampleRate) {
                smpb.setup(fSampleRate, mbSampleRate);
                need_bresample = 1;
            } else if (mbSampleRate < fSampleRate) {
                smpb.setup(mbSampleRate, fSampleRate);
                need_bresample = 2;
            } 
            // fprintf(stderr, "B: %s\n", load_bfile.c_str());
        } 
        ready.store(true, std::memory_order_release);
    }
    if (modelb) return true;
    return false;
}

// non rt callback
void RtNeuralMulti::unload_json_bfile() {
    std::unique_lock<std::mutex> lk(WMutex);
    ready.store(false, std::memory_order_release);
    WCondVar->wait(lk);
    delete modelb;
    modelb = nullptr;
    mbSampleRate = 0;
    need_bresample = 0;
    clear_state_f();
    ready.store(true, std::memory_order_release);
}

