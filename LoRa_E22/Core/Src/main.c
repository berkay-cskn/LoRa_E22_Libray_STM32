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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "LoRa_E22.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

typedef struct {
	float latitude;
	float longitude;
	float altitude;
} Position;

typedef struct {
	float x;
	float y;
	float z;
} Vector3;

typedef struct __attribute__((__packed__)) {
	float altitude;
	Position gps;
	Vector3 accelerometer;
	Vector3 gyroscope;
	Vector3 magnetometer;
	float angle;
	uint8_t status;
} RocketDataPack;

typedef struct __attribute__((__packed__)){
	Position gps;
	float strainX;
	float strainY;
} PayloadDataPack;

typedef struct __attribute__((__packed__)) {
	uint8_t prefix;
	RocketDataPack rocketData;
	PayloadDataPack payloadData;
	Position comGPS;
	uint8_t ukb_rssi;
	uint8_t gy_rssi;
	uint8_t suffix;
} DataPack;

Lora_t lora;
Lora_t lora2;

DataPack dataPack;

Lora_Config_t loraConfig;

UartHandler_t uart4;
UartHandler_t uart1;
UartHandler_t uart2;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define PREFIX 0xAB
#define SUFFIX 0xDC
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart4;
UART_HandleTypeDef huart2;
UART_HandleTypeDef huart3;

/* USER CODE BEGIN PV */

uint8_t response[10];
uint8_t data[10];

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_UART4_Init(void);
static void MX_USART3_UART_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_UART4_Init();
  MX_USART3_UART_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */

  data[0] = 1;
  data[1] = 2;
  data[2] = 3;
  data[3] = 4;
  data[4] = 5;
  data[5] = 6;
  data[6] = 7;
  data[7] = 8;
  data[8] = 9;
  data[9] = 10;

  memset(&dataPack, 0, sizeof(dataPack));
  dataPack.prefix = PREFIX;
  dataPack.suffix = SUFFIX;

  RingBuffer_t rb;
  uart4.ringBuffer = &rb;
  uart4.uart = &huart4;
  Uart_Init(&uart4);

  Lora_Init_t loraInit;
  loraInit.auxPin = E22_AUX_Pin;
  loraInit.auxPort = E22_AUX_GPIO_Port;
  loraInit.m0Pin = E22_M0_Pin;
  loraInit.m0Port = E22_M0_GPIO_Port;
  loraInit.m1Pin = E22_M1_Pin;
  loraInit.m1Port = E22_M1_GPIO_Port;
  loraInit.baudRate = 9600;
  loraInit.bufferSize = 240;
  loraInit.prefix = 10;
  loraInit.suffix = 20;

  Lora_Init(&lora, &loraInit, &uart4);


  RingBuffer_t rb2;
  uart1.ringBuffer = &rb2;
  uart1.uart = &huart3;
  Uart_Init(&uart1);

  Lora_Init_t loraInit2;
  loraInit2.auxPin = E22_2_AUX_Pin;
  loraInit2.auxPort = E22_2_AUX_GPIO_Port;
  loraInit2.m0Pin = E22_2_M0_Pin;
  loraInit2.m0Port = E22_2_M0_GPIO_Port;
  loraInit2.m1Pin = E22_2_M1_Pin;
  loraInit2.m1Port = E22_2_M1_GPIO_Port;
  loraInit2.baudRate = 9600;
  loraInit2.bufferSize = 240;
  loraInit2.prefix = 10;
  loraInit2.suffix = 20;

  Lora_Init(&lora2, &loraInit2, &uart1);

  RingBuffer_t rb3;
  uart2.ringBuffer = &rb3;
  uart2.uart = &huart2;
  Uart_Init(&uart2);

/*
  Lora_SetMode(&lora, LORA_MODE_CONFIGURATION);
  HAL_Delay(100);
  loraConfig.ADDH = 0;
  loraConfig.ADDL = 101;
  loraConfig.AirDataRate = LORA_AIR_DATA_RATE_4_8;
  loraConfig.AmbientRSSI = 0;
  loraConfig.BaudRate = LORA_UART_BAUD_RATE_9600;
  loraConfig.Channel = 18;
  loraConfig.FixedPointTransmission = 1;
  loraConfig.LBTEnabled = 0;
  loraConfig.ParityBit = 0;
  loraConfig.NETID = 0;
  loraConfig.RSSIEnabled = 1;
  loraConfig.RepeaterEnabled = 0;
  loraConfig.SubPacketSize = LORA_SUB_PACKET_SIZE_64;
  loraConfig.TransmittingPower = LORA_TRANSMITTING_POWER_30;
  loraConfig.WORCycle = 0;
  loraConfig.WORTransceiverControl = 0;
*/
  Lora_SetMode(&lora, LORA_MODE_CONFIGURATION);
  HAL_Delay(100);
  Lora_GetConfig(&lora);
  HAL_Delay(100);
  Lora_SetMode(&lora, LORA_MODE_NORMAL);

  HAL_Delay(100);

  Lora_SetMode(&lora2, LORA_MODE_CONFIGURATION);
  HAL_Delay(100);
  Lora_GetConfig(&lora2);
  HAL_Delay(100);
  Lora_SetMode(&lora2, LORA_MODE_NORMAL);

  int startTime = HAL_GetTick();
  int dataReady = 0;
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

	  Lora_Process(&lora2);
	  if(Lora_IsDataReady(&lora2)) {
		  Lora_Read(&lora2, response, 10);
		  dataReady = 1;
	  }

	  if(HAL_GetTick() - startTime > 200) {
		  startTime = HAL_GetTick();
		  Lora_Write(&lora, 101, 18, data, 10);
		  dataPack.ukb_rssi = lora2.RSSI;
		  //if(dataReady) {
			  Uart_Write(&uart2, (uint8_t *)&dataPack, sizeof(dataPack));
			  dataReady = 0;
		  //}
	  }

  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
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
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief UART4 Initialization Function
  * @param None
  * @retval None
  */
static void MX_UART4_Init(void)
{

  /* USER CODE BEGIN UART4_Init 0 */

  /* USER CODE END UART4_Init 0 */

  /* USER CODE BEGIN UART4_Init 1 */

  /* USER CODE END UART4_Init 1 */
  huart4.Instance = UART4;
  huart4.Init.BaudRate = 9600;
  huart4.Init.WordLength = UART_WORDLENGTH_8B;
  huart4.Init.StopBits = UART_STOPBITS_1;
  huart4.Init.Parity = UART_PARITY_NONE;
  huart4.Init.Mode = UART_MODE_TX_RX;
  huart4.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart4.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart4) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN UART4_Init 2 */

  /* USER CODE END UART4_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 19200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 9600;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(E22_M1_GPIO_Port, E22_M1_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, E22_M0_Pin|E22_2_M1_Pin|E22_2_M0_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : E22_M1_Pin */
  GPIO_InitStruct.Pin = E22_M1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(E22_M1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : E22_AUX_Pin */
  GPIO_InitStruct.Pin = E22_AUX_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(E22_AUX_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : E22_M0_Pin E22_2_M1_Pin E22_2_M0_Pin */
  GPIO_InitStruct.Pin = E22_M0_Pin|E22_2_M1_Pin|E22_2_M0_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : E22_2_AUX_Pin */
  GPIO_InitStruct.Pin = E22_2_AUX_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(E22_2_AUX_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
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
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
