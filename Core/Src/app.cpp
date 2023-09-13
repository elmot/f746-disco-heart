#include <cstdio>
#include "main.h"
#include "stm32746g_discovery_lcd.h"
#include "Arduino.h"
#include "MAX30100.h"
//
// Created by Ilia.Motornyi on 27-Nov-20.
//

static void LCD_Config() {
    BSP_SDRAM_Init();
    /* LCD Initialization */
    BSP_LCD_Init();

    /* LCD Initialization */
    BSP_LCD_LayerDefaultInit(0, LCD_FB_START_ADDRESS);
    BSP_LCD_LayerDefaultInit(1, LCD_FB_START_ADDRESS + (BSP_LCD_GetXSize() * BSP_LCD_GetYSize() * 4));

    /* Enable the LCD */
    BSP_LCD_DisplayOn();

    /* Select the LCD Background Layer  */
    BSP_LCD_SelectLayer(0);

    /* Clear the Background Layer */
    BSP_LCD_Clear(LCD_COLOR_DARKBLUE);

    /* Select the LCD Foreground Layer  */
    BSP_LCD_SelectLayer(1);

    /* Clear the Foreground Layer */
    BSP_LCD_Clear(LCD_COLOR_DARKBLUE);

    /* Configure the transparency for foreground and background :
       Increase the transparency */
    BSP_LCD_SetTransparency(0, 0);
    BSP_LCD_SetTransparency(1, 255);
}
#include "../examples/MAX30100_RawData/MAX30100_RawData.ino"

_Noreturn void App_Run(void) {
    setup();
    sensor.resume();
    sensor.resetFifo();
    while (true)
    {
        loop();
    }
}
_Noreturn void _App_Run(void) {
    LCD_Config();
    BSP_LCD_SetTextColor(LCD_COLOR_YELLOW);
    BSP_LCD_SetBackColor(LCD_COLOR_DARKBLUE);
    BSP_LCD_Clear(LCD_COLOR_TRANSPARENT);
    BSP_LCD_DisplayStringAtLine(0, (uint8_t *) "MAX30100 Demo");
    float temp;
    uint8_t buff[100];
    BSP_LCD_SetFont(&Font24);
    MAX30100 sensor;
    sensor.startTemperatureSampling();
    sensor.setMode(MAX30100_MODE_SPO2_HR);
    sensor.setSamplingRate(MAX30100_SAMPRATE_100HZ);
    sensor.setLedsCurrent(MAX30100_LED_CURR_24MA, MAX30100_LED_CURR_24MA);
    sensor.setLedsPulseWidth(MAX30100_SPC_PW_200US_13BITS);
    sensor.resetFifo();
    while (true){

    }
    while (true) {
        sensor.update();
        uint8_t partId = sensor.getPartId();
//        if (sensor.isTemperatureReady()) {
//            temp = sensor.retrieveTemperature();
//            sensor.startTemperatureSampling();
//        }
        snprintf((char *) buff, sizeof buff, "Part ID: %x; Temp: %6.1fC", partId, temp);
        BSP_LCD_DisplayStringAtLine(1, buff);
        for(int i = 2; i<8;++i) {
            uint16_t ir = -1, red = -1;
            sensor.getRawValues(&ir, &red);
            snprintf((char *) buff, sizeof buff, "IR: %d; RED: %d", ir, red);
            BSP_LCD_DisplayStringAtLine(i, buff);
        }
        sensor.resetFifo();
        HAL_Delay(120);
    }

}
