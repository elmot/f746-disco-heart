
#ifndef MAX30100_H
#define MAX30100_H

#include <cstdint>

class MAX30100 {
private:
    uint8_t readRegister(uint8_t address);

    bool writeRegister(uint8_t address, uint8_t data);

    bool (*readI2CRegisters)(uint8_t i2CAddress, uint8_t address, uint8_t len, uint8_t *data);

    bool (*writeI2CRegister)(uint8_t i2CAddress, uint8_t address, uint8_t data);

public:
    enum [[maybe_unused]] PULSE_WIDTH_t : uint8_t {
        PULSE_200US = 0x00,
        PULSE_400US = 0x01,
        PULSE_800US = 0x02,
        PULSE_1600US = 0x03,
        PULSE_MASK = 0xfc,
    };
    enum [[maybe_unused]] SAMPLING_RATE_t : uint8_t {
        RATE_50HZ = 0x00,
        RATE_100HZ = 0x01 << 2,
        RATE_167HZ = 0x02 << 2,
        RATE_200HZ = 0x03 << 2,
        RATE_400HZ = 0x04 << 2,
        RATE_600HZ = 0x05 << 2,
        RATE_800HZ = 0x06 << 2,
        RATE_1000HZ = 0x07 << 2,
        RATE_MASK = 0xe3
    };
    typedef enum [[maybe_unused]] LED_CURRENT_t : uint8_t {
        CURR_0MA = 0x00,
        CURR_4_4MA = 0x01,
        CURR_7_6MA = 0x02,
        CURR_11MA = 0x03,
        CURR_14_2MA = 0x04,
        CURR_17_4MA = 0x05,
        CURR_20_8MA = 0x06,
        CURR_24MA = 0x07,
        CURR_27_1MA = 0x08,
        CURR_30_6MA = 0x09,
        CURR_33_8MA = 0x0a,
        CURR_37MA = 0x0b,
        CURR_40_2MA = 0x0c,
        CURR_43_6MA = 0x0d,
        CURR_46_8MA = 0x0e,
        CURR_50MA = 0x0f
    } LEDCurrent;

    explicit MAX30100(
            bool (*readI2CRegisters)(uint8_t i2CAddress, uint8_t address, uint8_t len, uint8_t *data),
            bool (*writeI2CRegister)(uint8_t i2CAddress, uint8_t address, uint8_t data)) {
        this->readI2CRegisters = readI2CRegisters;
        this->writeI2CRegister = writeI2CRegister;
    }

    bool setSpo2Mode(bool enabled);

    bool setLedsPulseWidth(PULSE_WIDTH_t ledPulseWidth);

    bool setSamplingRate(SAMPLING_RATE_t samplingRate);

    bool setLedsCurrent(LED_CURRENT_t irLedCurrent, LED_CURRENT_t redLedCurrent);

    bool setHighresMode(bool enabled);


    bool readValues(void (*accept)(uint16_t irValue, uint16_t redValue),
                    void (*overflow)(),
                    uint8_t maxBatchCnt);

    bool resetFifo();

    bool startTemperatureSampling();

    bool isTemperatureReady();

    float retrieveTemperature();

    bool shutdown();

    bool resume();

    uint8_t getPartId();

    uint8_t getRevId();

};

#endif //MAX30100_H
