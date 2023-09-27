//
// Created by elmot on 23 Sep 2023.
//
#include "digital_filter.h"
std::vector<float> performFirPass(const std::vector<float> &srcBuffer, const std::vector<float> &fir_taps) {
    std::vector<float> firStateF32(2 * BLOCK_SIZE + fir_taps.size() - 1);

    arm_fir_instance_f32 fir_instance;

    arm_fir_init_f32(&fir_instance, fir_taps.size(), fir_taps.data(), firStateF32.data(), BLOCK_SIZE);

    /* ----------------------------------------------------------------------
    ** Call the FIR process function for every blockSize samples
    ** ------------------------------------------------------------------- */
    std::vector<float> result(srcBuffer.size());
    for (int idx = 0; idx < SAMPLE_CAPACITY;) {
        int blockSize = SAMPLE_CAPACITY - idx;
        if (blockSize > BLOCK_SIZE) blockSize = BLOCK_SIZE;
        arm_fir_f32(&fir_instance, srcBuffer.data() + idx, result.data() + idx, blockSize);
        idx += blockSize;
    }
    return result;
}

__unused std::vector<float> revert(const std::vector<float> &srcBuffer) {
    std::vector<float> result(srcBuffer.size());
    std::copy(std::begin(srcBuffer), std::end(srcBuffer), result.rbegin());
    return result;
}
