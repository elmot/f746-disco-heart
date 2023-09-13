/*
Arduino-MAX30100 oximetry / heart rate integrated sensor library
Copyright (C) 2016  OXullo Intersecans <x@brainrapers.org>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
/**
 * Modified by elmot to be used with STM32HAL
 */
#include "stm32746g_discovery.h"
#include "MAX30100.h"

#define I2C_HANDLE hi2c1
extern I2C_HandleTypeDef hi2c1;


MAX30100::MAX30100() {
}

bool MAX30100::begin() {
    return true;
}

void MAX30100::setMode(Mode mode) {
    writeRegister(MAX30100_REG_MODE_CONFIGURATION, mode);
}

void MAX30100::setLedsPulseWidth(LEDPulseWidth ledPulseWidth) {
    uint8_t previous = readRegister(MAX30100_REG_SPO2_CONFIGURATION);
    writeRegister(MAX30100_REG_SPO2_CONFIGURATION, (previous & 0xfc) | ledPulseWidth);
}

void MAX30100::setSamplingRate(SamplingRate samplingRate) {
    uint8_t previous = readRegister(MAX30100_REG_SPO2_CONFIGURATION);
    writeRegister(MAX30100_REG_SPO2_CONFIGURATION, (previous & 0xe3) | (samplingRate << 2));
}

void MAX30100::setLedsCurrent(LEDCurrent irLedCurrent, LEDCurrent redLedCurrent) {
    writeRegister(MAX30100_REG_LED_CONFIGURATION, redLedCurrent << 4 | irLedCurrent);
}

void MAX30100::setHighresModeEnabled(bool enabled) {
    uint8_t previous = readRegister(MAX30100_REG_SPO2_CONFIGURATION);
    if (enabled) {
        writeRegister(MAX30100_REG_SPO2_CONFIGURATION, previous | MAX30100_SPC_SPO2_HI_RES_EN);
    } else {
        writeRegister(MAX30100_REG_SPO2_CONFIGURATION, previous & ~MAX30100_SPC_SPO2_HI_RES_EN);
    }
}

void MAX30100::update() {
    readFifoData();
}

bool MAX30100::getRawValues(uint16_t *ir, uint16_t *red) {
    if (!readoutsBuffer.isEmpty()) {
        SensorReadout readout = readoutsBuffer.pop();

        *ir = readout.ir;
        *red = readout.red;

        return true;
    } else {
        return false;
    }
}

void MAX30100::resetFifo() {
    writeRegister(MAX30100_REG_FIFO_WRITE_POINTER, 0);
    writeRegister(MAX30100_REG_FIFO_READ_POINTER, 0);
    writeRegister(MAX30100_REG_FIFO_OVERFLOW_COUNTER, 0);
}

uint8_t MAX30100::readRegister(uint8_t address) {
    uint8_t data;
    HAL_I2C_Mem_Read(&I2C_HANDLE, MAX30100_I2C_ADDRESS<<1, address, 1, &data, 1, 1);
    return data;
}

void MAX30100::writeRegister(uint8_t address, uint8_t data) {
    HAL_I2C_Mem_Write(&I2C_HANDLE, MAX30100_I2C_ADDRESS<<1, address, 1, &data, 1, 1);
}

void MAX30100::burstRead(uint8_t baseAddress, uint8_t *buffer, uint8_t length) {
    HAL_I2C_Mem_Read(&I2C_HANDLE, MAX30100_I2C_ADDRESS<<1, baseAddress, 1, buffer, length, 1);
}

void MAX30100::readFifoData() {
    uint8_t buffer[MAX30100_FIFO_DEPTH * 4];
    uint8_t toRead;

    toRead = (readRegister(MAX30100_REG_FIFO_WRITE_POINTER) - readRegister(MAX30100_REG_FIFO_READ_POINTER)) &
             (MAX30100_FIFO_DEPTH - 1);

    if (toRead) {
        burstRead(MAX30100_REG_FIFO_DATA, buffer, 4 * toRead);

        for (uint8_t i = 0; i < toRead; ++i) {
            // Warning: the values are always left-aligned
            readoutsBuffer.push({
                                        .ir=(uint16_t) ((buffer[i * 4] << 8) | buffer[i * 4 + 1]),
                                        .red=(uint16_t) ((buffer[i * 4 + 2] << 8) | buffer[i * 4 + 3])});
        }
    }
}

void MAX30100::startTemperatureSampling() {
    uint8_t modeConfig = readRegister(MAX30100_REG_MODE_CONFIGURATION);
    modeConfig |= MAX30100_MC_TEMP_EN;

    writeRegister(MAX30100_REG_MODE_CONFIGURATION, modeConfig);
}

bool MAX30100::isTemperatureReady() {
    return !(readRegister(MAX30100_REG_MODE_CONFIGURATION) & MAX30100_MC_TEMP_EN);
}

float MAX30100::retrieveTemperature() {
    int8_t tempInteger = readRegister(MAX30100_REG_TEMPERATURE_DATA_INT);
    float tempFrac = readRegister(MAX30100_REG_TEMPERATURE_DATA_FRAC);

    return tempFrac * 0.0625 + tempInteger;
}

void MAX30100::shutdown() {
    uint8_t modeConfig = readRegister(MAX30100_REG_MODE_CONFIGURATION);
    modeConfig |= MAX30100_MC_SHDN;

    writeRegister(MAX30100_REG_MODE_CONFIGURATION, modeConfig);
}

void MAX30100::resume() {
    uint8_t modeConfig = readRegister(MAX30100_REG_MODE_CONFIGURATION);
    modeConfig &= ~MAX30100_MC_SHDN;

    writeRegister(MAX30100_REG_MODE_CONFIGURATION, modeConfig);
}

uint8_t MAX30100::getPartId() {
    return readRegister(0xff);
}
