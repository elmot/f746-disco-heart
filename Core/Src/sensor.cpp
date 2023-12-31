#include <cstdio>
#include "sensor.h"
#include "FreeRTOS.h"
#include "task.h"
#include "stm32f7xx_hal.h"
#include "MAX30100.h"

const MAX30100::SAMPLING_RATE_t rate =
#if SAMPLES_PER_SEC == 50
        MAX30100_SAMPRATE_50HZ;
#elif SAMPLES_PER_SEC == 100
        MAX30100::RATE_100HZ;
#elif SAMPLES_PER_SEC == 167
MAX30100_SAMPRATE_167HZ;
#elif SAMPLES_PER_SEC == 200
MAX30100_SAMPRATE_200HZ;
#elif SAMPLES_PER_SEC == 400
MAX30100_SAMPRATE_400HZ;
#elif SAMPLES_PER_SEC == 600
MAX30100_SAMPRATE_600HZ;
#elif SAMPLES_PER_SEC == 800
MAX30100_SAMPRATE_800HZ;
#elif SAMPLES_PER_SEC == 1000
MAX30100_SAMPRATE_1000HZ;
#else

#error Incorrect SAMPLES_PER_SEC value

#endif

extern I2C_HandleTypeDef hi2c1;
extern "C" void Error_Handler();
static MAX30100 sensor (
        [](uint8_t i2CAddress, uint8_t address, uint8_t len, uint8_t *data) {
            return HAL_OK ==
                   HAL_I2C_Mem_Read(&hi2c1, i2CAddress << 1, address, 1, data, len, len);
        },
        [](uint8_t i2CAddress, uint8_t address, uint8_t data) {
            return HAL_OK ==
                   HAL_I2C_Mem_Write(&hi2c1, i2CAddress << 1, address, 1, &data, 1, 1);
        }
);

void setupSensor() {

    puts("Initializing MAX30100..");

    bool initResult = sensor.setSpo2Mode(true);
    initResult &= sensor.setLedsCurrent(MAX30100::CURR_20_8MA, MAX30100::CURR_20_8MA);
    initResult &= sensor.setLedsPulseWidth(MAX30100::MAX30100::PULSE_400US);
    initResult &= sensor.setSamplingRate(rate);
    initResult &= sensor.setHighresMode(true);
    initResult &= sensor.resetFifo();
    if (initResult) {
        puts("SUCCESS");
    } else {
        puts("FAILED");
        Error_Handler();
    }
    uint8_t partId = sensor.getPartId();
    uint8_t revId = sensor.getRevId();
    printf("Device: 0x%02x:0x%02x\r\n", partId, revId);
    puts("Temperature measurement...");
    if(!sensor.startTemperatureSampling()) {
        puts("FAILED");
        Error_Handler();
    }
    while (!sensor.isTemperatureReady()) {}
    printf("T = %.1fC\r\n", sensor.retrieveTemperature());

}

extern "C" [[noreturn]] void sensor_start(__unused void *argument) {
    setupSensor();

    while (true) {
        sensor.readValues(
                [](uint16_t irValue, uint16_t redValue) {
                    SensorSample_t sample = {irValue, redValue};
                    sendData(&sample);
                }, [] {}, 10);
        vTaskDelay(10);
    }
}
