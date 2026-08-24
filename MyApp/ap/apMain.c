#include "apMain.h"
#include "adc.h"
#include "myAdc.h"
#include "myDht11.h"
#include "myDs1302.h"
#include "myHcSr04.h"
#include "myI2c.h"
#include "myLcd1602.h"
#include "myMpu6050.h"
#include "mySsd1306.h"
#include "myUart.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_adc.h"
#include "stm32f4xx_hal_def.h"
#include "stm32f4xx_hal_tim.h"
#include "tim.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static ds1302Time_t rtc_time = {0};
static mpu6050Data_t mpu_data = {0};

extern ADC_HandleTypeDef hadc1;
void apInit(void) {
  uartInit();
  hcSr04Init();
  adcInit();
  dht11Init();
  lcd1602Init();
  i2cScan();
  mpu6050Init();
  ssd1306Init();
  ds1302Init();
}

float internal_temp = 0;
dht11Data_t dht_data = {0};
bool dht_status = false;

void apMain(void) {
  uint32_t tick_1000 = 0;
  uint32_t tick_250 = 0;
  uint32_t tick_100 = 0;
  uint32_t tick_50 = 0;
  uint32_t current_tick = 0;

  ssd1306Clear();
  ssd1306DrawRect(0, 0, SSD1306_WIDTH, SSD1306_HEIGHT, SSD1306_COLOR_WHITE);
  ssd1306DrawString(8, 3, "STM32 MULTI-SENSOR", SSD1306_COLOR_WHITE);
  ssd1306DrawLine(4, 13, 124, 13, SSD1306_COLOR_WHITE);
  ssd1306Update();
  HAL_TIM_Base_Start_IT(&htim2);

  while (1) {
    current_tick = HAL_GetTick();

    if (current_tick - tick_1000 >= 1000) {
      tick_1000 = current_tick;

      ds1302GetDateTime(&rtc_time);
      printf("sec: %02d\r\n", rtc_time.sec);
    }

    if (current_tick - tick_250 >= 250) {
      tick_250 = current_tick;
      adcUpdate();
      dht_status = dht11Read(&dht_data);
      internal_temp = adcGetTemp();

      float distance_cm;

      if (hcSr04Read(&distance_cm)) {
        printf("Distance: %.2f cm\r\n", distance_cm);
      } else {
        printf("HC-SR04 read failed\r\n");
      }

      // lcd1602Clear();
      // lcd1602Cursor(0, 0);
      // lcd1602Printf("Temp %.2f/%.2f", internal_temp, dht_data.temperature);
      // lcd1602Cursor(1, 0);
      // lcd1602Printf("Humi %.2f", dht_data.humidity);
    }

    if (current_tick - tick_100 >= 100) {
      tick_100 = current_tick;
      if (mpu6050Read(&mpu_data)) {
        // printf(">acc_x:%.3f\r\n>acc_y:%.3f\r\n>acc_z:%.3f\r\n>gyro_x:"
        //        "%.3f\r\n>gyro_y: %.3f\r\n>gyro_z: %.3f\r\n",
        //        mpu_data.accel_x, mpu_data.accel_y, mpu_data.accel_z,
        //        mpu_data.gyro_x, mpu_data.gyro_y, mpu_data.gyro_z);

        char str[32];
        ssd1306FillRect(2, 15, 124, 47, SSD1306_COLOR_BLACK);
        ssd1306DrawChar(4, 16, '~' + 1, SSD1306_COLOR_WHITE);
        ssd1306DrawChar(10, 16, '~' + 2, SSD1306_COLOR_WHITE);
        ssd1306DrawChar(16, 16, '~' + 3, SSD1306_COLOR_WHITE);
        ssd1306DrawChar(22, 16, '~' + 4, SSD1306_COLOR_WHITE);
        ssd1306DrawChar(28, 16, '~' + 5, SSD1306_COLOR_WHITE);
        ssd1306DrawChar(34, 16, '~' + 6, SSD1306_COLOR_WHITE);
        ssd1306DrawChar(40, 16, '~' + 7, SSD1306_COLOR_WHITE);
        ssd1306DrawChar(46, 16, '~' + 8, SSD1306_COLOR_WHITE);
        ssd1306DrawChar(52, 16, '~' + 9, SSD1306_COLOR_WHITE);
        ssd1306DrawChar(58, 16, '~' + 10, SSD1306_COLOR_WHITE);
        snprintf(str, sizeof(str), "GyroX: %5.2f", mpu_data.gyro_x);
        ssd1306DrawString(5, 25, str, SSD1306_COLOR_WHITE);
        snprintf(str, sizeof(str), "GyroY: %5.2f", mpu_data.gyro_y);
        ssd1306DrawString(5, 35, str, SSD1306_COLOR_WHITE);
        snprintf(str, sizeof(str), "GyroZ: %5.2f", mpu_data.gyro_z);
        ssd1306DrawString(5, 45, str, SSD1306_COLOR_WHITE);

        ssd1306Update();
      }
    }

    if (current_tick - tick_50 >= 50) {
    }

    HAL_Delay(10);
  }
}