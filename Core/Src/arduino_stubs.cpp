//
// Created by elmot on 13 Sep 2023.
//
#include <Arduino.h>
#include <cstring>
#include <cstdlib>
#include <string>
#include "stm32746g_discovery.h"

uint32_t millis() {
    return HAL_GetTick();
}

void delay(uint32_t millis) {
    HAL_Delay(millis);
}

#define UART huart1
SerialStub Serial;

extern UART_HandleTypeDef UART;

void SerialStub::begin(unsigned long baud) {
    //ignore for now
}

void SerialStub::print(int i, RadixType radix) {
    itoa(i, buff, radix);
    print(buff);
}

void SerialStub::print(char c) {
    HAL_UART_Transmit(&UART,  (uint8_t*)&c, 1, 1);
}
void SerialStub::print(float f) {
    std::string res = std::to_string(f);
    print(res.c_str());
}

void SerialStub::print(const char *txt) {
    size_t len = strlen(txt);
    HAL_UART_Transmit(&UART, (uint8_t *) txt, len, len);
}

bool SerialStub::available() {
    return __HAL_UART_GET_FLAG(&UART,UART_FLAG_RXNE);
}
