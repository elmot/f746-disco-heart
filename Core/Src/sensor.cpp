#include <cstdio>
#include "sensor.h"
#include "MAX30100.h"
#include "FreeRTOS.h"
#include "task.h"

const SamplingRate rate =
#if SAMPLES_PER_SEC == 50
        MAX30100_SAMPRATE_50HZ;
#elif SAMPLES_PER_SEC == 100
        MAX30100_SAMPRATE_100HZ;
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

extern "C" void Error_Handler();
static MAX30100 sensor;

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
    sensor.setSamplingRate(rate);
    sensor.setHighresModeEnabled(true);
    sensor.resetFifo();
}

extern "C" [[noreturn]] void sensor_start(__unused void *argument) {
    setupSensor();
    SensorSample_t sample;
    while (true) {
        sensor.update();
        while (sensor.getRawValues(&sample.ir, &sample.red)) {
            sendData(&sample);
        }
        vTaskDelay(10);
    }
}
