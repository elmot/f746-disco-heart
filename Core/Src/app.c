#include <stdio.h>
#include "main.h"
#include "stm32746g_discovery_lcd.h"
//
// Created by Ilia.Motornyi on 27-Nov-20.
//

static void LCD_Config(void) {
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
    BSP_LCD_Clear(LCD_COLOR_BLUE);

    /* Select the LCD Foreground Layer  */
    BSP_LCD_SelectLayer(1);

    /* Clear the Foreground Layer */
    BSP_LCD_Clear(LCD_COLOR_BLUE);

    /* Configure the transparency for foreground and background :
       Increase the transparency */
    BSP_LCD_SetTransparency(0, 0);
    BSP_LCD_SetTransparency(1, 255);
}

_Noreturn void App_Run(void) {
    LCD_Config();
    BSP_LCD_SetTextColor(LCD_COLOR_YELLOW);
    BSP_LCD_SetBackColor(LCD_COLOR_BLUE);
    BSP_LCD_DisplayStringAtLine(1, "HTS221 Demo");
//    int32_t status = CUSTOM_ENV_SENSOR_Init(CUSTOM_HTS221_0, ENV_TEMPERATURE | ENV_HUMIDITY);
//    printf("Sensor Status: %ld\n", status);
//    status = CUSTOM_ENV_SENSOR_Enable(CUSTOM_HTS221_0, ENV_TEMPERATURE);
uint status=12;
    printf("Sensor Status: %ld\n", status);
//    status = CUSTOM_ENV_SENSOR_Enable(CUSTOM_HTS221_0, ENV_HUMIDITY);
    printf("Sensor Status: %ld\n", status);
    float temp;
    float hum;
    char buff[100];
    BSP_LCD_SetFont(&Font24);
    while(1) {
//        CUSTOM_ENV_SENSOR_GetValue(CUSTOM_HTS221_0, ENV_TEMPERATURE, &temp);
//        CUSTOM_ENV_SENSOR_GetValue(CUSTOM_HTS221_0, ENV_HUMIDITY, &hum);
        snprintf(buff, sizeof buff, "Temp: %6.1f; Hum: %6.1f", temp, hum);
        BSP_LCD_DisplayStringAtLine(4, buff);
        HAL_Delay(1000);
    }

}
