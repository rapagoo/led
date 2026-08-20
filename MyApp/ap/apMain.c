#include "apMain.h"
#include "adc.h"
#include "myAdc.h"
#include "myDht11.h"
#include "myI2c.h"
#include "myLcd1602.h"
#include "myUart.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_adc.h"
#include "stm32f4xx_hal_def.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

extern ADC_HandleTypeDef hadc1;
void apInit(void) {
  uartInit();
  adcInit();
  dht11Init();
  lcd1602Init();
  i2cScan();
}

float internal_temp = 0;
dht11Data_t dht_data = {0};
bool dht_status = false;

void apMain(void) {
  lcd1602Clear();
  lcd1602Cursor(0, 0);
  lcd1602Print("Hello LCD");
  lcd1602Cursor(1, 5);
  lcd1602Print("World!");

  while (1) {
    adcUpdate();
    dht_status = dht11Read(&dht_data);
    internal_temp = adcGetTemp();
    HAL_Delay(10);
  }
}