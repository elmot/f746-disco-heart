//
// Created by elmot on 15 Sep 2023.
//

#ifndef F746_DISCO_HEART_DIGITAL_FILTER_H
#define F746_DISCO_HEART_DIGITAL_FILTER_H

#include "dsp/transform_functions.h"
#include "dsp/filtering_functions.h"
#include "dsp/support_functions.h"

const int SAMPLE_CAPACITY = (512);
const int SAMPLE_CAPACITY_HALF = (SAMPLE_CAPACITY / 2);

void performFFT(uint16_t const srcBuffer[SAMPLE_CAPACITY], float dstBuffer[SAMPLE_CAPACITY]);

void performBandPass(uint16_t const srcBuffer[SAMPLE_CAPACITY], float dstBuffer[SAMPLE_CAPACITY_HALF]);

#endif //F746_DISCO_HEART_DIGITAL_FILTER_H
