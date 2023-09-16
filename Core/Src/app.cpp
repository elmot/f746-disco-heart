#include <cstdio>
#include <cmath>
#include <algorithm>
#include "digital_filter.h"
#include "main.h"
#include "stm32746g_discovery_lcd.h"
#include <FreeRTOS.h>
#include "queue.h"
#include "sensor.h"

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

static uint16_t sampleBufferIr[SAMPLE_CAPACITY];
__unused static uint16_t sampleBufferRed[SAMPLE_CAPACITY];

void clearGraph();

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
__unused std::array<float, len> autoConvolution(const std::array<float, len> &srcBuffer) {
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
customConvolution(const std::array<float, len> &srcBufferA, const std::array<float, len> &srcBufferB) {
    const uint bufLen = len + len / 2;
    float buffer[bufLen];
    arm_conv_partial_f32(srcBufferA.data(), (uint32_t) len,
                         srcBufferB.data(), (uint32_t) len,
                         buffer, 0, bufLen);


    std::array<float, len> result = {};
    std::copy_n(std::begin(buffer) + len / 6, len, result.begin());
    return result;
}

template<size_t len>
std::array<float, len>
revert(const std::array<float, len> &srcBuffer) {
    std::array<float, len> result = {};
    std::copy(std::begin(srcBuffer), std::end(srcBuffer), result.rbegin());
    return result;
}

template<size_t len>
std::vector<int> detectPeaks(const std::array<float, len> &src) {
    typedef std::tuple<int, float> point;
    std::vector<point> peaks;
    std::vector<int> result;
    int lastPeak = -1000;
    for (int i = 7; i < src.size() - 7; ++i) {
        auto v = src[i];
        if ((v >= src[i - 1]) && (v > src[i - 7]) && (v > src[i - 3]) &&
            (v > src[i + 1]) && (v > src[i + 7]) && (v > src[i + 3])
            && (i - lastPeak) > 7) {
            peaks.push_back({i, src[i]});
        }
    }
    if (peaks.size() < 4) return {};
    struct {
        bool operator()(point &pointA, point &pointB) const {
            return std::get<float>(pointA) > std::get<float>(pointB);
        }
    } sorValRev;

    std::sort(peaks.begin(), peaks.end(), sorValRev);

    const auto threshold = std::get<float>(peaks[1]) / 2; //skip first peak - it's often noise

    //remove all below threshold
    peaks.erase(std::remove_if(
            peaks.begin(), peaks.end(),
            [threshold](const point &x) {
                return std::get<float>(x) < threshold;
            }), peaks.end());
    if (peaks.size() < 4) return {}; //take into use?
    for (const auto &peak: peaks) {
        result.push_back(std::get<int>(peak));
    }
    std::sort(result.begin(), result.end());
    return result;
}

void drawPeaks(const std::vector<int> &peaks) {
    uint32_t oldTextColor = BSP_LCD_GetTextColor();
    BSP_LCD_SetTextColor(LCD_COLOR_MAGENTA);
    for (const auto &x: peaks) {
        if ((x > 0) && (x < 480)) {
            BSP_LCD_DrawVLine(x, graphTop, graphHeight);
        }
    }
    BSP_LCD_SetTextColor(oldTextColor);
}

void outputHeartRate(const std::vector<int> &peaks) {
    float pulse = -1.f;
    bool valid = false;
    if (peaks.size() > 1) {
        std::vector<int> distances;
        for (int i = 1; i < peaks.size(); ++i) {
            distances.push_back(peaks[i] - peaks[i - 1]);
        }
        std::sort(distances.begin(), distances.end());
        auto median = (float) distances[distances.size() / 2];
        if (!(distances.size() & 1)) {
            median = (median + (float) distances[distances.size() / 2 + 1]) / 2.0f;
        }
        valid =
                median <= (float) distances[0] * 1.1f || median > (float) distances.back() / 1.1f;
        pulse = 60.f * SAMPLES_PER_SEC / median;
    }
    uint32_t oldTextColor = BSP_LCD_GetTextColor();
    BSP_LCD_SetTextColor(valid ? LCD_COLOR_MAGENTA : LCD_COLOR_DARKRED);
    BSP_LCD_ClearStringLine(1);

    static char buf[100];
    snprintf(buf, 100, "Peaks: %2d Pulse: %4.1f(%s)",
             peaks.size(), pulse, valid ? "OK" : "Fail");
    BSP_LCD_DisplayStringAtLine(1, (uint8_t *) buf);
    BSP_LCD_SetTextColor(oldTextColor);
}


_Noreturn void App_Run(void) {
    LCD_Config();
    BSP_LCD_SetTextColor(LCD_COLOR_YELLOW);
    BSP_LCD_SetBackColor(LCD_COLOR_DARKBLUE);
    BSP_LCD_DisplayStringAtLine(0, (uint8_t *) "Heartbeat Demo");
    BSP_LCD_SetFont(&Font24);
    int sampleIndex = 0;
    while (true) {
        SensorSample_t sample;
        bool received = receiveData( &sample, 1000);
        if(received == pdPASS) {
            if (sampleIndex >= 0) {
                sampleBufferIr[sampleIndex] = sample.ir;
                sampleBufferRed[sampleIndex] = sample.red;
                BSP_LCD_DrawPixel(sampleIndex, graphTop, LCD_COLOR_GREEN);
            }
            sampleIndex++;
            if (sampleIndex >= SAMPLE_CAPACITY) {
                clearGraph();
                auto normalized =
                        num_to_float_normalize<uint16_t, SAMPLE_CAPACITY>(sampleBufferIr);
                auto afterLowPass = performFirPass(normalized, LOW_PASS_TAPS);
                auto afterHighPass = performFirPass(afterLowPass, HIGH_PASS_TAPS);
                auto convoluted = customConvolution(afterLowPass, revert(afterLowPass));
                drawGraph(normalized, LCD_COLOR_LIGHTCYAN);
                drawGraph(afterLowPass, LCD_COLOR_LIGHTBLUE);
                drawGraph(afterHighPass, LCD_COLOR_ORANGE);
//                drawGraph(convoluted, LCD_COLOR_WHITE);
                const std::vector<int> &peaks = detectPeaks(afterHighPass);
                drawPeaks(peaks);
                outputHeartRate(peaks);
                sampleIndex = 0;
            }
        }
    }
}


