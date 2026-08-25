#include "myTimer.h"
#include "rtc.h"
#include "stm32f4xx_hal_rtc.h"
#include "stm32f4xx_hal_tim.h"
#include "tim.h"
#include <stdint.h>
static uint8_t s_current_duty = 50;

void timerInit(void) {
  timerPwmStart();
  timerSetDuty(50);
}

void timerPwmStart(void) {
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1); // PA6
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2); // PA7
  HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1); // PB6
}

void timerPwmStop(void) {
  HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1); // PA6
  HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_2); // PA7
  HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_1); // PB6
}
void timerSetDuty(uint8_t duty_percent) {
  if (duty_percent > 100) {
    duty_percent = 100;
  }
  s_current_duty = duty_percent;

  uint32_t period = __HAL_TIM_GET_AUTORELOAD(&htim3) + 1;
  uint32_t pulse = (period * duty_percent) / 100;

  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, pulse); // PA6
  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, pulse); // PA7
  __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, pulse); // PB6
}

void RTC_Set_Next_10s_Alarm(void) {
  RTC_TimeTypeDef sTime = {0};
  RTC_DateTypeDef sDate = {0};
  RTC_AlarmTypeDef sAlarm = {0};

  /* RTC 레지스터 구조상 Time을 먼저 읽고 Date를 다음에 읽어야 락(Lock)이
   * 풀리며 동기화됨 */
  HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
  HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

  uint8_t next_sec = (sTime.Seconds + 10) % 60;

  sAlarm.AlarmTime.Hours = 0x0;
  sAlarm.AlarmTime.Minutes = 0x0;
  sAlarm.AlarmTime.Seconds = next_sec;
  sAlarm.AlarmTime.SubSeconds = 0x0;
  sAlarm.AlarmTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  sAlarm.AlarmTime.StoreOperation = RTC_STOREOPERATION_RESET;
  sAlarm.AlarmMask =
      RTC_ALARMMASK_DATEWEEKDAY | RTC_ALARMMASK_HOURS | RTC_ALARMMASK_MINUTES;
  sAlarm.AlarmSubSecondMask = RTC_ALARMSUBSECONDMASK_ALL;
  sAlarm.AlarmDateWeekDaySel = RTC_ALARMDATEWEEKDAYSEL_DATE;
  sAlarm.AlarmDateWeekDay = 0x1;
  sAlarm.Alarm = RTC_ALARM_A;

  HAL_RTC_SetAlarm_IT(&hrtc, &sAlarm, RTC_FORMAT_BIN);
}

/* 알람 이벤트 발생 시 호출되는 RTC 전용 콜백 인터럽트 구현 */
void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc) {
  RTC_Set_Next_10s_Alarm();
}