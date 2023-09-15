#include <cstdio>
#include "main.h"
#include "stm32746g_discovery_lcd.h"
#include "MAX30100.h"
#include "digital_filter.h"

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

const int sensorSamplingRate = 100;
const int graphTop = 60;
const int graphBottom = 272;
const int graphHeight = graphBottom - graphTop;
const float frequency_resolution = (float) sensorSamplingRate / (float) SAMPLE_CAPACITY;
static MAX30100 sensor;

static uint16_t sampleBufferIr[SAMPLE_CAPACITY];
static uint16_t sampleBufferRed[SAMPLE_CAPACITY];
static uint16_t fftBufferPwr[SAMPLE_CAPACITY_HALF];

void drawGraph(const uint16_t samples[], int valCnt, uint32_t color);

void clearGraph();

 void setupSensor() {

    puts("Initializing MAX30100..");

    // Initialize the sensor
// Failures are generally due to an improper I2C wiring, missing power supply
// or wrong target chip
    if (!sensor.begin()) {
        puts("FAILED");
#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
        for (;;);
#pragma clang diagnostic pop
    } else {
        puts("SUCCESS");
    }

    // Set up the wanted parameters
    sensor.setMode(MAX30100_MODE_SPO2_HR);
//    sensor.setMode(MAX30100_MODE_HRONLY);
    sensor.setLedsCurrent(MAX30100_LED_CURR_11MA, MAX30100_LED_CURR_11MA);
    sensor.setLedsPulseWidth(MAX30100_SPC_PW_400US_14BITS);
    sensor.setSamplingRate(MAX30100_SAMPRATE_50HZ);
    sensor.setHighresModeEnabled(true);
    sensor.resetFifo();
}


void drawGraph(const uint16_t samples[], int valCnt, uint32_t color) {
    int topVal = samples[0];
    int bottomVal = topVal;
    for (int i = 1; i < valCnt; ++i) {
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
    for (int i = 0; i < valCnt; ++i) {
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
            if (sampleIndex >= SAMPLE_CAPACITY) {
                clearGraph();
                performFFT((q15_t *) sampleBufferIr, (q15_t *) fftBufferPwr);
                drawGraph(sampleBufferIr, 480, LCD_COLOR_LIGHTCYAN);
                drawGraph(sampleBufferRed, 480, LCD_COLOR_LIGHTRED);
                drawGraph(fftBufferPwr, SAMPLE_CAPACITY_HALF, LCD_COLOR_ORANGE);
                for (int i = 1; i < SAMPLE_CAPACITY_HALF; i++) {
                    printf("%d\tfrq: %.1f\tenergy %.d\r\n", i, (float) i * frequency_resolution, fftBufferPwr[i]);
                }
                sampleIndex = 0;
                sensor.resetFifo();
            }
        }
    }
}
