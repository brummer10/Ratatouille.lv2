/*
 * Parameter.h
 *
 * SPDX-License-Identifier:  BSD-3-Clause
 *
 * Copyright (C) 2025 brummer <brummer@web.de>
 */


#include <string>
#include <vector>
#include <cstring>
#include <atomic>

#pragma once
#ifndef PARAMETER_H_
#define PARAMETER_H_

enum ParameterType {
    Is_FLOAT = 0,
    IS_DOUBLE,
    IS_INT,
    IS_UINT,
};

struct Parameter {
    int id;             // clap_id set during register 
    std::string name;   // name of the parameter
    std::string group;  // group of the parameter
    double min;         // min value
    double max;         // max value
    double def;         // default value
    double step;        // default step from controller
    void* value;        // void pointer to the variable holding the value
    bool isStepped;     // is parameter toggled or use integer steps 
    int type;           // controller type 0 = float, 1 = double, 2 = int32_t, 3 = uint32_t,
};

class Params {
public:
    Params() {
        paramChanged.store(false, std::memory_order_release);
        controllerChanged.store(false, std::memory_order_release);
    }
    
    ~Params() {
        parameter.clear();
    }

    std::atomic<bool> paramChanged;         // indicate parameter changed by the host
    std::atomic<bool> controllerChanged;    // indicate parameter changed by the GUI
    std::vector<Parameter> parameter;       // vector holding all parameters

    // register a parameter
    void registerParam(Parameter p) {
        parameter.push_back(p);
    }

    // register a variable as parameter
    void registerParam(std::string name, std::string group,
                    double min, double max, double def, double step,
                    void* value, bool isStepped, int type) {
        int id = static_cast<int>(parameter.size());
        Parameter p = {id, name, group, min, max, def, step, value, isStepped, type};
        parameter.push_back(p);
    }

    // register a variable as parameter
    void registerParam(const char* name, const char* group,
                    double min, double max, double def, double step,
                    void* value, bool isStepped, int type) {
        int id = static_cast<int>(parameter.size());
        Parameter p = {id, name, group, min, max, def, step, value, isStepped, type};
        parameter.push_back(p);
    }

    // reset all parameters to default values
    void reset() {
        uint32_t count = parameter.size();
        for (uint32_t i = 0; i < count; i++) {
            const auto& def = parameter[i];
            setParam(i, def.def);
        }
        // inform the GUI thread that parameters was changed by the host
        paramChanged.store(true, std::memory_order_release);
    }

    // get the parameter value as double
    double getParam(int idx) const {
        if (idx >= static_cast<int>(parameter.size())) return 0.0;
        if (parameter[idx].type == Is_FLOAT) {
            float* pvalue = static_cast<float*>(parameter[idx].value);
            return (double)*pvalue;
        } else if (parameter[idx].type == IS_DOUBLE) {
            double* pvalue = static_cast<double*>(parameter[idx].value);
            return *pvalue;
        } else if (parameter[idx].type == IS_INT) {
            int32_t* pvalue = static_cast<int32_t*>(parameter[idx].value);
            return (double)*pvalue;
        } else if (parameter[idx].type == IS_UINT) {
            uint32_t* pvalue = static_cast<uint32_t*>(parameter[idx].value);
            return (double)*pvalue;
        }
        return 0.0;
    }

    // set the parameter value as double
    void setParam(int idx, double value) {
        if (idx >= static_cast<int>(parameter.size())) return;
        if (parameter[idx].type == Is_FLOAT) {
            float* pvalue = static_cast<float*>(parameter[idx].value);
            *pvalue = static_cast<float>(value);
        } else if (parameter[idx].type == IS_DOUBLE) {
            double* pvalue = static_cast<double*>(parameter[idx].value);
            *pvalue = value;
        } else if (parameter[idx].type == IS_INT) {
            int32_t* pvalue = static_cast<int32_t*>(parameter[idx].value);
            *pvalue = static_cast<int32_t>(value);
        } else if (parameter[idx].type == IS_UINT) {
            uint32_t* pvalue = static_cast<uint32_t*>(parameter[idx].value);
            *pvalue = static_cast<uint32_t>(value);
        }
        // inform the GUI thread that a parameter was changed by the host
        paramChanged.store(true, std::memory_order_release);
    }
};

#endif
