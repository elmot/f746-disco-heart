#include "digital_filter.h"

static q15_t fftBufferIrSrc[SAMPLE_CAPACITY];
static q15_t fftBufferIr[SAMPLE_CAPACITY];

void performFFT(q15_t srcBuffer[SAMPLE_CAPACITY], q15_t dstBuffer[SAMPLE_CAPACITY_HALF]) {
    arm_rfft_instance_q15 fft_inst_q15;
    arm_copy_q15(srcBuffer,fftBufferIrSrc, SAMPLE_CAPACITY);
    arm_rfft_init_q15(&fft_inst_q15, SAMPLE_CAPACITY, 0, 1);
    arm_rfft_q15(&fft_inst_q15, fftBufferIrSrc, fftBufferIr);
    arm_cmplx_mag_q15(fftBufferIr, dstBuffer, SAMPLE_CAPACITY_HALF);
}
