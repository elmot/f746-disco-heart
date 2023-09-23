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
#include "dsp/transform_functions.h"
#include "dsp/filtering_functions.h"


const int SAMPLE_CAPACITY = (480);
static const int BLOCK_SIZE = 64;

template<typename NUM, int len>
std::vector<float> num_to_float_normalize(const std::array<NUM, len> &srcBuf, int srcIndexShift = 0) {
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
    std::vector<float> result(len);
    for (int i = 0; i < len; ++i) {
        double v = (srcBuf[(i + srcIndexShift) % len] - bottomVal) / magnitudeScale;
        if (v > 1.0) v = 1.0; else if (v < -1.0) v = -1.0;
        result[i] = (float) v;
    }
    return result;
}

std::vector<float> performFirPass(const std::vector<float> &srcBuffer, const std::vector<float> &fir_taps);

std::vector<float> revert(const std::vector<float> &srcBuffer);

#endif //F746_DISCO_HEART_DIGITAL_FILTER_H
