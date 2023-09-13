//
// Created by elmot on 13 Sep 2023.
//

#ifndef F746_DISCO_HEART_ARDUINO_H
#define F746_DISCO_HEART_ARDUINO_H
#include "stm32746g_discovery.h"

enum RadixType {
    HEX = 16,
    DEC = 10
};

uint32_t millis();
void delay(uint32_t millis);

class SerialStub {
public:
    void begin(uint32_t baud);
    void print(const char *);
    void print(float);
    void print(char);
    void print(int, RadixType radix = DEC);
    bool available();

    void println() {
        print("\r\n");
    }

    void println(int value, RadixType radix = DEC) {
        print(value, radix);
        println();
    }

    template <typename T>
    void println(T value) {
        print(value);
        println();
    }

private:
    char buff [100];
};
extern SerialStub Serial;
#endif //F746_DISCO_HEART_ARDUINO_H
