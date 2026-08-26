#include "apMain.h"
#include "adc.h"
#include "cmsis_os2.h"
#include "freeRTOS.h"
#include "main.h"
#include "myAdc.h"
#include "myDht11.h"
#include "myDs1302.h"
#include "myHcSr04.h"
#include "myI2c.h"
#include "myLcd1602.h"
#include "myMpu6050.h"
#include "mySsd1306.h"
#include "myTimer.h"
#include "myUart.h"
#include "rtc.h"
#include "stm32f411xe.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_adc.h"
#include "stm32f4xx_hal_def.h"
#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal_rtc.h"
#include "stm32f4xx_hal_tim.h"
#include "tim.h"

#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define TARGET_COUNT 1000000
#define HALF_COUNT (TARGET_COUNT / 2)

/* 공유 전역 변수 */
volatile uint32_t g_shared_counter = 0;
volatile uint8_t task1_done = 0;
volatile uint8_t task2_done = 0;

osMutexId_t counterMutexHandle;
const osMutexAttr_t counterMutex_attributes = {
  .name = "counterMutex"
};

static ds1302Time_t rtc_time = {0};
static mpu6050Data_t mpu_data = {0};

extern ADC_HandleTypeDef hadc1;
void apInit(void) {
  uartInit();
  hcSr04Init();
  adcInit();
  lcd1602Init();
  i2cScan();
  mpu6050Init();
  ssd1306Init();
  ds1302Init();
  dht11Init();
  timerInit();

  bool lcd_status = lcd1602Init();
  printf("LCD init: %s\r\n", lcd_status ? "OK" : "FAIL");
}

float distance_cm = 0.0f;

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
  HAL_TIM_Base_Start_IT(&htim3);
  HAL_TIM_Base_Start_IT(&htim4);

  RTC_TimeTypeDef sTime;
  RTC_DateTypeDef sDate;

  while (1) {
    current_tick = HAL_GetTick();
    timerLedUpdate();

    if (current_tick - tick_1000 >= 1000) {
      tick_1000 = current_tick;

      ds1302GetDateTime(&rtc_time);
      printf("sec: %02d\r\n", rtc_time.sec);

      /* RTC 레지스터 구조상 Time을 먼저 읽고 Date를 다음에 읽어야 락(Lock)이
       * 풀리며 동기화됨 */
      HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
      HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

      /* 시리얼 터미널로 실시간 날짜와 시간 출력 */
    }

    // float distance_cm;
    if (current_tick - tick_250 >= 250) {
      tick_250 = current_tick;
      adcUpdate();
      internal_temp = adcGetTemp();

      // if (hcSr04Read(&distance_cm)) {
      //   printf("Distance: %.2f cm\r\n", distance_cm);
      // } else {
      //   printf("HC-SR04 read failed\r\n");
      // }

      lcd1602Clear();
      lcd1602Cursor(0, 0);
      lcd1602Printf("Temp %.2f/%.2f", internal_temp, dht_data.temperature);
      lcd1602Cursor(1, 0);
      lcd1602Printf("Humi %.2f", dht_data.humidity);
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

void StartDefaultTask(void *argument) {
  apInit();

  counterMutexHandle = osMutexNew(&counterMutex_attributes);
  if (counterMutexHandle == NULL) {
    Error_Handler();
  }

  while (1) {
    if (task1_done && task2_done) {
      printf("\r\n==================================\r\n");
      printf("Expected Target : %lu\r\n", (long unsigned int)TARGET_COUNT);
      printf("Actual Result   : %lu\r\n", g_shared_counter);
      printf("Loss Count      : %lu\r\n", TARGET_COUNT - g_shared_counter);
      printf("==================================\r\n");

      osThreadExit();
    }
    osDelay(1000);
  }
}

void StartTaskLED(void *argument) {
  for (;;) {
    HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
    osDelay(500); // HAL_Delay 대신 반드시 osDelay 사용 (태스크 양보)
  }
}

void StartTaskCLI(void *argument) { apMain(); }

void StartTaskHCS04(void *argument) {
  while (1) {
    hcSr04Read(&distance_cm);
    osDelay(1000);
  }
}

void StartTaskDHT11(void *argument) {
  while (1) {
    dht11Read(&dht_data);
    osDelay(1000);
  }
}

/* Task 1: 50만 번 증가 */
void StartTask01(void *argument) {
  for (uint32_t i = 0; i < HALF_COUNT; i++) {
    // osMutexAcquire(counterMutexHandle, osWaitForever);
    // g_shared_counter++; // [비원자적 연산] Read -> Modify -> Write
    // osMutexRelease(counterMutexHandle);
    atomic_fetch_add(&g_shared_counter, 1);
  }
  task1_done = 1;
  osThreadExit();
}

/* Task 2: 50만 번 증가 */
void StartTask02(void *argument) {
  for (uint32_t i = 0; i < HALF_COUNT; i++) {
    // osMutexAcquire(counterMutexHandle, osWaitForever);
    // g_shared_counter++; // [비원자적 연산] Read -> Modify -> Write
    // osMutexRelease(counterMutexHandle);
    atomic_fetch_add(&g_shared_counter, 1);
  }
  task2_done = 1;
  osThreadExit();
}
