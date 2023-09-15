#include "digital_filter.h"

static float fftBufferIrSrc[SAMPLE_CAPACITY];
static float fftBufferIr[SAMPLE_CAPACITY];

void performFFT(uint16_t const srcBuffer[SAMPLE_CAPACITY], float dstBuffer[SAMPLE_CAPACITY_HALF]) {
    arm_rfft_instance_f32 fft_inst_f32;
    arm_cfft_radix4_instance_f32 cfft_inst_f32;
    arm_q15_to_float((q15_t *) srcBuffer, fftBufferIrSrc, SAMPLE_CAPACITY);

    arm_rfft_init_f32(&fft_inst_f32, &cfft_inst_f32, SAMPLE_CAPACITY, 0, 1);
    arm_rfft_f32(&fft_inst_f32, fftBufferIrSrc, fftBufferIr);
    arm_cmplx_mag_f32(fftBufferIr, dstBuffer, SAMPLE_CAPACITY_HALF);
}

const int FIR_PARAMS_CNT = 22;
static float FIR_PARAMS_F32[FIR_PARAMS_CNT] = {
        0.047619047619047644, 0.047619047619047644, 0.047619047619047644, 0.047619047619047644, 0.047619047619047644,
        0.047619047619047644, 0.047619047619047644, 0.047619047619047644, 0.047619047619047644, 0.047619047619047644,
        0.047619047619047644, 0.047619047619047644, 0.047619047619047644, 0.047619047619047644, 0.047619047619047644,
        0.047619047619047644, 0.047619047619047644, 0.047619047619047644, 0.047619047619047644, 0.047619047619047644,
        0.047619047619047644,
        /*Filling zero*/ 0
};

static const int BLOCK_SIZE = 64;
static float firStateF32[2 * BLOCK_SIZE + FIR_PARAMS_CNT - 1];

void performBandPass(const uint16_t srcBuffer[SAMPLE_CAPACITY], float dstBuffer[SAMPLE_CAPACITY_HALF]) {
    float workingData[SAMPLE_CAPACITY];
    int topVal = srcBuffer[0];
    int bottomVal = topVal;
    for (int i = 1; i < SAMPLE_CAPACITY; ++i) {
        auto v = srcBuffer[i];
        if (v > topVal) topVal = v;
        if (v < bottomVal) bottomVal = v;
    }
    auto midValue = (topVal + bottomVal) / 2;
    for (int i = 0; i < SAMPLE_CAPACITY; ++i) {
        float v = srcBuffer[i];
        workingData[i] = v - (float) midValue;
    }


    arm_fir_instance_f32 fir_instance;

    arm_fir_init_f32(&fir_instance, FIR_PARAMS_CNT, FIR_PARAMS_F32, firStateF32, BLOCK_SIZE);


    /* ----------------------------------------------------------------------
    ** Call the FIR process function for every blockSize samples
    ** ------------------------------------------------------------------- */

    for (int idx = 0; idx < SAMPLE_CAPACITY;) {
        int blockSize = SAMPLE_CAPACITY - idx;
        if (blockSize > BLOCK_SIZE) blockSize = BLOCK_SIZE;
        arm_fir_f32(&fir_instance, workingData + idx, dstBuffer + idx, blockSize);
        idx += blockSize;
    }

}

