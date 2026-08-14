/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2026 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dma.h"
#include "gpio.h"
#include "usart.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#ifdef __GNUC__
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif

PUTCHAR_PROTOTYPE {
  /* Polling 방식으로 1바이트 전송 (전송 완료될 때까지 대기) */
  HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, 0xFFFF);
  return ch;
}
#define RX_BUF_SIZE 64
#define LINE_BUF_SIZE 64
uint8_t usart1_rx_buf[RX_BUF_SIZE];
uint8_t usart2_rx_buf[RX_BUF_SIZE];

uint8_t line_buf[LINE_BUF_SIZE];
uint16_t line_len = 0;

uint8_t rx_data;
uint8_t rx_buf[RX_BUF_SIZE];
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
  if (GPIO_Pin == GPIO_PIN_13) {
    HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
  } else if (GPIO_Pin == GPIO_PIN_0) {
    HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_0);
  }
}

// void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
//   if (huart->Instance == USART2) {
//     if (rx_data == 'a')
//       printf("Hello STM32 Cortex-M4 USART Polling!\r\n");
//     else
//       HAL_UART_Transmit(&huart2, rx_buf, RX_BUF_SIZE, 100);

//     HAL_UART_Receive_DMA(&huart2, rx_buf, RX_BUF_SIZE);
//   }
//     else if (huart->Instance == USART1) {
//     if (rx_data == 'a')
//       printf("Hello STM32 Cortex-M4 USART Polling!\r\n");
//     else
//       HAL_UART_Transmit(&huart1, rx_buf, RX_BUF_SIZE, 100);

//     HAL_UART_Receive_DMA(&huart1, rx_buf, RX_BUF_SIZE);
//   }
// }

static void ProcessKeyboardInput(uint8_t key) {
  const uint8_t newline[] = "\r\n";
  const uint8_t erase[] = "\b \b";

  // Enter: 저장한 한 줄을 상대에게 전송
  if (key == '\r' || key == '\n') {
    if (line_len > 0) {
      HAL_UART_Transmit(&huart1, line_buf, line_len, 100);
    }
    HAL_UART_Transmit(&huart1, (uint8_t *)newline, 2, 100);
    HAL_UART_Transmit(&huart2, (uint8_t *)newline, 2, 100);
    line_len = 0;
  }
  // Backspace: 버퍼와 내 화면에서 마지막 문자 삭제
  else if ((key == '\b') || (key == 0x7F)) {
    if (line_len > 0) {
      line_len--;
      HAL_UART_Transmit(&huart2, (uint8_t *)erase, 3, 100);
    }
  }
  // 일반 문자: 버퍼에 저장하고 내 화면에만 표시
  else if ((key >= 0x20) && (key <= 0x7E) && (line_len < LINE_BUF_SIZE)) {
    line_buf[line_len++] = key;
    HAL_UART_Transmit(&huart2, &key, 1, 100);
  }
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size) {
  if (huart->Instance == USART2) {
    /*
     * PC에서 받은 키를 먼저 처리
     * 이 작업이 끝날 때까지 DMA 버퍼는 재사용 X
     */
    for (uint16_t i = 0; i < Size; i++) {
      ProcessKeyboardInput(usart2_rx_buf[i]);
    }

    /* 처리가 끝난 다음 USART2 수신 재시작 */
    HAL_UARTEx_ReceiveToIdle_DMA(&huart2, usart2_rx_buf, RX_BUF_SIZE);

    __HAL_DMA_DISABLE_IT(huart2.hdmarx, DMA_IT_HT);
  } else if (huart->Instance == USART1) {
    /* 상대가 보낸 데이터를 내 PC에 출력 */
    HAL_UART_Transmit(&huart2, usart1_rx_buf, Size, 100);

    /* 출력이 끝난 다음 USART1 수신 재시작 */
    HAL_UARTEx_ReceiveToIdle_DMA(&huart1, usart1_rx_buf, RX_BUF_SIZE);

    __HAL_DMA_DISABLE_IT(huart1.hdmarx, DMA_IT_HT);
  }
}

// void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size){
//   if (huart->Instance == USART2) {
//     if (rx_data == 'a')
//       printf("Hello STM32 Cortex-M4 USART Polling!\r\n");
//     else
//       HAL_UART_Transmit(&huart2, rx_buf, Size, 100);
//     HAL_UART_DMAStop(&huart2);
//     memset(rx_buf,0,Size);
//   }
//   HAL_UARTEx_ReceiveToIdle_DMA(&huart2, rx_buf, RX_BUF_SIZE);
// }

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
  if (huart->Instance == USART1) {
    HAL_UART_DMAStop(&huart1);
    HAL_UARTEx_ReceiveToIdle_DMA(&huart1, usart1_rx_buf, RX_BUF_SIZE);
    __HAL_DMA_DISABLE_IT(huart1.hdmarx, DMA_IT_HT);
  } else if (huart->Instance == USART2) {
    HAL_UART_DMAStop(&huart2);
    HAL_UARTEx_ReceiveToIdle_DMA(&huart2, usart2_rx_buf, RX_BUF_SIZE);
    __HAL_DMA_DISABLE_IT(huart2.hdmarx, DMA_IT_HT);
  }
}

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick.
   */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART2_UART_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  HAL_UARTEx_ReceiveToIdle_DMA(&huart2, usart2_rx_buf, RX_BUF_SIZE);
  HAL_UARTEx_ReceiveToIdle_DMA(&huart1, usart1_rx_buf, RX_BUF_SIZE);
  __HAL_DMA_DISABLE_IT(huart2.hdmarx, DMA_IT_HT);
  __HAL_DMA_DISABLE_IT(huart1.hdmarx, DMA_IT_HT);
  // HAL_UART_Receive_DMA(&huart2, rx_buf, RX_BUF_SIZE);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1) {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
   */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
   * in the RCC_OscInitTypeDef structure.
   */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
   */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1) {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line) {
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line
     number, ex: printf("Wrong parameters value: file %s on line %d\r\n", file,
     line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
