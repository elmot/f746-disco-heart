#include "MAX30100.h"

[[maybe_unused]] static const uint8_t MAX30100_I2C_ADDRESS = 0x57;

typedef enum : uint8_t {
    MODE_HRONLY = 0x02,
    MODE_SPO2_HR = 0x03
} MODE_t;

// Interrupt status register (RO)
[[maybe_unused]] static const uint8_t REG_INTERRUPT_STATUS = 0x00;
typedef enum [[maybe_unused]] : uint8_t {
    IS_PWR_RDY = (1 << 0),
    IS_SPO2_RDY = (1 << 4),
    IS_HR_RDY = (1 << 5),
    IS_TEMP_RDY = (1 << 6),
    IS_A_FULL = (1 << 7)
} INTERRUPT_STATUS_t;

// Interrupt enable register
[[maybe_unused]] static const uint8_t REG_INTERRUPT_ENABLE = 0x01;
typedef enum [[maybe_unused]] : uint8_t {
    IE_ENB_SPO2_RDY = (1 << 4),
    IE_ENB_HR_RDY = (1 << 5),
    IE_ENB_TEMP_RDY = (1 << 6),
    IE_ENB_A_FULL = (1 << 7)
} INTERRUPT_ENABLE_t;

// FIFO control and data registers
static const uint8_t REG_FIFO_WRITE_POINTER = 0x02;
static const uint8_t REG_FIFO_OVERFLOW_COUNTER = 0x03;
static const uint8_t REG_FIFO_READ_POINTER = 0x04;
static const uint8_t REG_FIFO_DATA = 0x05;  // Burst read does not autoincrement addr

// Mode Configuration register
static const uint8_t REG_MODE_CONFIGURATION = 0x06;
typedef enum : uint8_t {
    MC_TEMP_EN = (1 << 3),
    MC_RESET = (1 << 6),
    MC_SHDN = (1 << 7)
} MODE_CONFIGURATION_t;

static const uint8_t REG_REVISION_ID = 0xfe;
static const uint8_t REG_PART_ID = 0xff;
static const uint8_t REG_TEMPERATURE_DATA_INT = 0x16;
static const uint8_t REG_TEMPERATURE_DATA_FRAC = 0x17;

static const uint8_t REG_SPO2_CONFIGURATION = 0x07;
static const uint8_t SPC_SPO2_HI_RES_EN = 1 << 6;

static const uint8_t FIFO_DEPTH = 32;

static const uint8_t REG_LED_CONFIGURATION = 0x09;

bool MAX30100::setSpo2Mode(bool enabled) {
    return writeRegister(REG_MODE_CONFIGURATION, enabled ? MODE_SPO2_HR : MODE_HRONLY);
}

bool MAX30100::setLedsPulseWidth(PULSE_WIDTH_t ledPulseWidth) {
    uint8_t spo2Configuration = readRegister(REG_SPO2_CONFIGURATION);
    return writeRegister(REG_SPO2_CONFIGURATION, (spo2Configuration & PULSE_MASK) | ledPulseWidth);
}

bool MAX30100::setSamplingRate(SAMPLING_RATE_t samplingRate) {
    uint8_t spo2Configuration = readRegister(REG_SPO2_CONFIGURATION);
    return writeRegister(REG_SPO2_CONFIGURATION, (spo2Configuration & RATE_MASK) | samplingRate);
}

bool MAX30100::setLedsCurrent(LED_CURRENT_t irLedCurrent, LED_CURRENT_t redLedCurrent) {
    return writeRegister(REG_LED_CONFIGURATION, redLedCurrent << 4 | irLedCurrent);
}

bool MAX30100::setHighresMode(bool enabled) {
    uint8_t spo2Configuration = readRegister(REG_SPO2_CONFIGURATION);
    if (enabled) {
        spo2Configuration |= SPC_SPO2_HI_RES_EN;
    } else {
        spo2Configuration &= ~SPC_SPO2_HI_RES_EN;
    }
    return writeRegister(REG_SPO2_CONFIGURATION, spo2Configuration);
}

bool MAX30100::resetFifo() {
    bool result = writeRegister(REG_FIFO_WRITE_POINTER, 0);
    result &= writeRegister(REG_FIFO_READ_POINTER, 0);
    result &= writeRegister(REG_FIFO_OVERFLOW_COUNTER, 0);
    readRegister(REG_FIFO_DATA);
    return result;
}


uint8_t MAX30100::readRegister(uint8_t address) {
    uint8_t data;
    readI2CRegisters(MAX30100_I2C_ADDRESS, address, 1, &data);
    return data;
}

bool MAX30100::writeRegister(uint8_t address, uint8_t data) {
    return writeI2CRegister(MAX30100_I2C_ADDRESS, address, data);
}


bool MAX30100::startTemperatureSampling() {
    uint8_t modeConfig = readRegister(REG_MODE_CONFIGURATION);
    modeConfig |= MC_TEMP_EN;
    return writeRegister(REG_MODE_CONFIGURATION, modeConfig);
}

bool MAX30100::isTemperatureReady() {
    return !(readRegister(REG_MODE_CONFIGURATION) & MC_TEMP_EN);
}

float MAX30100::retrieveTemperature() {
    auto tempInteger = (int8_t) readRegister(REG_TEMPERATURE_DATA_INT);
    float tempFrac = readRegister(REG_TEMPERATURE_DATA_FRAC);

    return tempFrac * 0.0625f + (float) tempInteger;
}


bool MAX30100::shutdown() {
    uint8_t modeConfig = readRegister(REG_MODE_CONFIGURATION);
    modeConfig |= MC_SHDN;

    return writeRegister(REG_MODE_CONFIGURATION, modeConfig);
}

bool MAX30100::resume() {
    uint8_t modeConfig = readRegister(REG_MODE_CONFIGURATION);
    modeConfig &= ~MC_SHDN;
    return writeRegister(REG_MODE_CONFIGURATION, modeConfig);
}


uint8_t MAX30100::getPartId() {
    return readRegister(REG_PART_ID);
}

uint8_t MAX30100::getRevId() {
    return readRegister(REG_REVISION_ID);
}

bool MAX30100::readValues(void (*accept)(uint16_t irValue, uint16_t redValue),
                          void (*overflow)(),
                          uint8_t maxBatchCnt) {
    static uint8_t buffer[FIFO_DEPTH * 4];
    uint8_t overflowCounter = readRegister(REG_FIFO_OVERFLOW_COUNTER);
    if (overflowCounter != 0) {
        overflow();
        resetFifo();
    }
    int toRead = (int) readRegister(REG_FIFO_WRITE_POINTER) - readRegister(REG_FIFO_READ_POINTER);
    if (toRead < 0) toRead += 0xF;
    if (toRead > FIFO_DEPTH) toRead = FIFO_DEPTH;
    if (toRead > maxBatchCnt) toRead = maxBatchCnt;
    if (toRead) {
        if (!readI2CRegisters(MAX30100_I2C_ADDRESS, REG_FIFO_DATA, 4 * toRead, buffer)) {
            return false;
        }

        for (int i = 0; i < toRead; ++i) {
            auto ir = (uint16_t) ((buffer[i * 4] << 8u) | buffer[i * 4 + 1]);
            auto red = (uint16_t) ((buffer[i * 4 + 2] << 8u) | buffer[i * 4 + 3]);
            accept(ir, red);
        }
    }
    return true;
}
