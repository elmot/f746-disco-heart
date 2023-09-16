#include "digital_filter.h"
#include <cstdio>
#include <cmath>
#include <algorithm>
#include "main.h"
#include "stm32746g_discovery_lcd.h"
#include "MAX30100.h"
#include "dsp/transform_functions.h"
#include "dsp/filtering_functions.h"

static void LCD_Config() {
    BSP_SDRAM_Init();
    /* LCD Initialization */
    BSP_LCD_Init();

    /* LCD Initialization */
    BSP_LCD_LayerDefaultInit(0, LCD_FB_START_ADDRESS);

    /* Enable the LCD */
    BSP_LCD_DisplayOn();

    /* Clear the Background Layer */
    BSP_LCD_Clear(LCD_COLOR_DARKBLUE);

    BSP_LCD_SetTransparency(0, 255);
}

const int graphTop = 60;
const uint graphBottom = 272;
const int graphHeight = graphBottom - graphTop;
static MAX30100 sensor;

static uint16_t sampleBufferIr[SAMPLE_CAPACITY];
static uint16_t sampleBufferRed[SAMPLE_CAPACITY];

void clearGraph();

void setupSensor() {

    puts("Initializing MAX30100..");

    // Initialize the sensor
// Failures are generally due to an improper I2C wiring, missing power supply
// or wrong target chip
    if (!sensor.begin()) {
        puts("FAILED");
        Error_Handler();
    } else {
        puts("SUCCESS");
    }

    // Set up the wanted parameters
    sensor.setMode(MAX30100_MODE_SPO2_HR);
    sensor.setLedsCurrent(MAX30100_LED_CURR_20_8MA, MAX30100_LED_CURR_20_8MA);
    sensor.setLedsPulseWidth(MAX30100_SPC_PW_400US_14BITS);
    sensor.setSamplingRate(MAX30100_SAMPRATE_100HZ);
    sensor.setHighresModeEnabled(true);
    sensor.resetFifo();
}

template<size_t len>
void drawGraph(const std::array<float, len> &samples, uint32_t color) {
    float topVal = samples[0];
    float bottomVal = topVal;
    uint maxX = len < 480 ? len : 480;
    for (int i = 1; i < maxX; ++i) {
        float v = samples[i];
        if (v > topVal) topVal = v; else if (v < bottomVal) bottomVal = v;
    }
    float d;
    if (topVal == bottomVal) {
        d = 100;
    } else {
        d = (topVal - bottomVal) * 1.1f;
    }
    bottomVal -= d / 22;
    uint oldY;
    uint32_t oldTextColor = BSP_LCD_GetTextColor();
    BSP_LCD_SetTextColor(color);
    for (int i = 0; i < maxX; ++i) {
        float v = samples[i];
        float normV = (v - bottomVal) / (float) d;
        if (normV < 0.0f) normV = 0.f;
        if (normV > 1.0f) normV = 1.f;
        uint y = graphBottom - (uint) round(graphHeight * normV);
        if (i > 0) {
            BSP_LCD_DrawLine(i - 1, oldY, i, y);
        }

        oldY = y;
    }
    BSP_LCD_SetTextColor(oldTextColor);

}

void clearGraph() {
    BSP_LCD_SetTextColor(LCD_COLOR_DARKGREEN);
    BSP_LCD_FillRect(0, graphTop, 480, graphHeight);
}

#include "low_pass.inc"
#include "high_pass.inc"

template<size_t len>
std::array<float, len>
autoConvolution(const std::array<float, len> &srcBuffer) {
    const uint bufLen = len + len / 2;
    float buffer[bufLen];
    arm_conv_partial_f32(srcBuffer.data(), (uint32_t) len,
                         srcBuffer.data(), (uint32_t) len,
                         buffer, 0, bufLen);


    std::array<float, len> result = {};
    std::copy_n(std::begin(buffer) + len / 2, len, result.begin());
    return result;
}

template<size_t len>
std::array<float, len>
centerConvolution(const std::array<float, len> &srcBufferA, const std::array<float, len> &srcBufferB) {
    const uint bufLen = len + len / 2;
    float buffer[bufLen];
    arm_conv_partial_f32(srcBufferA.data(), (uint32_t) len,
                         srcBufferB.data(), (uint32_t) len,
                         buffer, 0, bufLen);


    std::array<float, len> result = {};
    std::copy_n(std::begin(buffer) + len / 2, len, result.begin());
    return result;
}

template<size_t len>
std::array<float, len>
revert(const std::array<float, len> &srcBuffer) {
    std::array<float, len> result = {};
    std::copy(std::begin(srcBuffer), std::end(srcBuffer), result.rbegin());
    return result;
}

_Noreturn void App_Run(void) {
    LCD_Config();
    BSP_LCD_SetTextColor(LCD_COLOR_YELLOW);
    BSP_LCD_SetBackColor(LCD_COLOR_DARKBLUE);
    BSP_LCD_DisplayStringAtLine(0, (uint8_t *) "MAX30100 Demo");
    BSP_LCD_SetFont(&Font24);
    setupSensor();
    int sampleIndex = 0;
    while (true) {
        uint16_t ir, red;

        sensor.update();

        while (sensor.getRawValues(&ir, &red)) {
            if (sampleIndex >= 0) {
                sampleBufferIr[sampleIndex] = ir;
                sampleBufferRed[sampleIndex] = red;
                BSP_LCD_DrawPixel(sampleIndex, graphTop, LCD_COLOR_GREEN);
            }
            sampleIndex++;
            if (sampleIndex >= SAMPLE_CAPACITY) {
                clearGraph();
                auto normalized =
                        num_to_float_normalize<uint16_t, SAMPLE_CAPACITY>(sampleBufferIr);
                auto afterLowPass = performFirPass(normalized, LOW_PASS_TAPS);
                auto afterHighPass = performFirPass(afterLowPass, HIGH_PASS_TAPS);
                auto convoluted = centerConvolution(afterLowPass, revert(afterLowPass));
                drawGraph(normalized, LCD_COLOR_LIGHTCYAN);
                drawGraph(afterLowPass, LCD_COLOR_LIGHTBLUE);
                drawGraph(afterHighPass, LCD_COLOR_ORANGE);
//                drawGraph(convoluted, LCD_COLOR_WHITE);
                sampleIndex = 0;
                sensor.resetFifo();
            }
        }
    }
}


