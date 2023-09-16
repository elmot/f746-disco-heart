//
// Created by elmot on 15 Sep 2023.
//

#ifndef F746_DISCO_HEART_DIGITAL_FILTER_H
#define F746_DISCO_HEART_DIGITAL_FILTER_H

#include <array>
#include <memory>
#include <cstdio>
#include <cmath>
#include <algorithm>
#include "main.h"
#include "stm32746g_discovery_lcd.h"
#include "MAX30100.h"
#include "dsp/transform_functions.h"
#include "dsp/filtering_functions.h"


const int SAMPLE_CAPACITY = (480);
static const int BLOCK_SIZE = 64;

template<typename NUM, int len>
std::array<float, len> num_to_float_normalize(const NUM srcBuf[len]) {
    NUM topVal = srcBuf[0];
    NUM bottomVal = topVal;
    for (int i = 1; i < len; ++i) {
        auto v = srcBuf[i];
        if (v > topVal) {
            topVal = v;
        } else if (v < bottomVal) bottomVal = v;
    }
    double magnitudeScale = (topVal - bottomVal);
    NUM mid = (topVal + bottomVal) / 2;
    std::array<float, len> result = {};
    for (int i = 0; i < len; ++i) {
        double v = (srcBuf[i] - bottomVal) / magnitudeScale;
        if (v > 1.0) v = 1.0; else if (v < -1.0) v = -1.0;
        result[i] = v;
    }
    return result;
}

template<size_t len, size_t num_taps>
std::array<float, len>
performFirPass(const std::array<float, len> &srcBuffer, const std::array<float, num_taps> &fir_taps) {
    static float firStateF32[2 * BLOCK_SIZE + num_taps - 1];

    arm_fir_instance_f32 fir_instance;

    arm_fir_init_f32(&fir_instance, num_taps, fir_taps.data(), firStateF32, BLOCK_SIZE);

    /* ----------------------------------------------------------------------
    ** Call the FIR process function for every blockSize samples
    ** ------------------------------------------------------------------- */
    std::array<float, len> result = {};
    for (int idx = 0; idx < SAMPLE_CAPACITY;) {
        int blockSize = SAMPLE_CAPACITY - idx;
        if (blockSize > BLOCK_SIZE) blockSize = BLOCK_SIZE;
        arm_fir_f32(&fir_instance, srcBuffer.data() + idx, result.data() + idx, blockSize);
        idx += blockSize;
    }
    return result;
}


#endif //F746_DISCO_HEART_DIGITAL_FILTER_H
