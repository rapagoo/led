#include "apMain.h"
#include "adc.h"
#include "myAdc.h"
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
}

float internal_temp = 0;

void apMain(void) {
  while (1) {
    adcUpdate();
    internal_temp = adcGetTemp();
    HAL_Delay(10);
  }
}