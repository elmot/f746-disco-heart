#include <cstdio>
#include "main.h"
#include "stm32746g_discovery_lcd.h"
#include "MAX30100.h"
#include "dsp/transform_functions.h"

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

const int sampleCapacity = 512;
const int graphTop = 60;
const int graphBottom = 272;
const int graphHeight = graphBottom - graphTop;

static MAX30100 sensor;

static uint16_t sampleBufferIr[sampleCapacity];
static uint16_t sampleBufferRed[sampleCapacity];
static uint16_t fftBufferIr[sampleCapacity];

void drawGraph(const uint16_t samples[sampleCapacity], uint32_t color);

void clearGraph();

void performFFT();

void setupSensor() {

    puts("Initializing MAX30100..");

    // Initialize the sensor
// Failures are generally due to an improper I2C wiring, missing power supply
// or wrong target chip
    if (!sensor.begin()) {
        puts("FAILED");
        for (;;);
    } else {
        puts("SUCCESS");
    }

    // Set up the wanted parameters
    sensor.setMode(MAX30100_MODE_SPO2_HR);
//    sensor.setMode(MAX30100_MODE_HRONLY);
    sensor.setLedsCurrent(MAX30100_LED_CURR_11MA, MAX30100_LED_CURR_11MA);
    sensor.setLedsPulseWidth(MAX30100_SPC_PW_1600US_16BITS);
    sensor.setSamplingRate(MAX30100_SAMPRATE_400HZ);
    sensor.setHighresModeEnabled(true);
    sensor.resetFifo();
}


void drawGraph(const uint16_t samples[sampleCapacity], uint32_t color) {
    int topVal = samples[0];
    int bottomVal = topVal;
    for (int i = 1; i < 480; ++i) {
        auto v = samples[i];
        if (v > topVal) topVal = v;
        if (v < bottomVal) bottomVal = v;
    }
    if (topVal == bottomVal) {
        topVal += 10;
        bottomVal -= 10;
    } else {
        auto d = (topVal - bottomVal) / 10;
        topVal += d;
        bottomVal -= d;
    }
    for (int i = 0; i < 480; ++i) {
        auto v = samples[i];
        int y = graphBottom - graphHeight * (v - bottomVal) / (topVal - bottomVal);
        BSP_LCD_DrawPixel(i, y, color);
    }
}

void clearGraph() {
    BSP_LCD_SetTextColor(LCD_COLOR_DARKGREEN);
    BSP_LCD_FillRect(0, graphTop, 480, graphHeight);
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
            if (sampleIndex >= sampleCapacity) {
                clearGraph();
                drawGraph(sampleBufferIr, LCD_COLOR_LIGHTCYAN);
                drawGraph(sampleBufferRed, LCD_COLOR_LIGHTRED);
                performFFT();
                drawGraph(fftBufferIr, LCD_COLOR_ORANGE);
                sampleIndex = 0;
                sensor.resetFifo();
            }
        }
    }
}

void performFFT() {
    arm_rfft_instance_q15 fft_inst_q15;
    arm_rfft_init_q15(&fft_inst_q15, 512, 0, 0);
    arm_rfft_q15(&fft_inst_q15, (q15_t *) sampleBufferIr, (q15_t *) fftBufferIr);
}
