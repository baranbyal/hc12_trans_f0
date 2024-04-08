/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
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

#include "serialLinBuff.h"
#include <stdint.h>

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
TIM_HandleTypeDef htim3;

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM3_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

#define pc_uart &huart1
#define device_uart &huart2

uint8_t uartByteBuff[1];
uint8_t uartByteBuff2[1];
serialLinBuff_t serialLinBuff;
serialLinBuff_t serialLinBuff2;

uint8_t parsedWeightValue[8];
uint8_t parsedWeightValueLength = 0;

void indicateOnLED(uint8_t channelNum);

uint8_t parseTextMessage(char *textMessage, uint8_t textMessageLength, uint8_t *parsedTextMessage);

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if(huart == pc_uart)
  {
    serialLinBuffAddChar(&serialLinBuff, uartByteBuff[0]);
    HAL_UART_Receive_IT(pc_uart, uartByteBuff, 1);
  }
  else if(huart == device_uart)
  {
    serialLinBuffAddChar(&serialLinBuff2, uartByteBuff2[0]);
    HAL_UART_Receive_IT(device_uart, uartByteBuff2, 1);
  }
}


uint8_t AT_Flag = 0;

// Define the maximum channel number
#define MAX_CHANNEL_NUMBER 10

// Global variables
volatile uint32_t buttonTimer = 0;


void plus_one(uint8_t *digits, uint8_t n) {
    int carry = 1; // Start with 1 to add it to the least significant digit

    for (int i = n - 1; i >= 0; i--) {
        // Add carry to the current digit
        digits[i] += carry;

        // Update carry for next iteration
        carry = (digits[i] > '9') ? 1 : 0;

        // Update current digit to the remainder after adding carry
        digits[i] = (digits[i] > '9') ? '0' : digits[i];
    }

    // If carry is still present after processing all digits, handle it accordingly
    if (carry != 0) {
        // You can handle overflow here according to your application requirements
        // For simplicity, let's assume the array has enough space to accommodate the new digit
        digits[0] = '1';
    }
}

void HC12_Set()
{
    // Check if AT_Flag is set to 1
    if (AT_Flag == 1)
    {
        // Set the HC-12 module to AT mode
        HAL_GPIO_WritePin(HC12_SET_GPIO_Port, HC12_SET_Pin, GPIO_PIN_RESET);
        HAL_Delay(1000); // Wait for 1 second for the module to enter AT mode

        

        // Send "AT+RC" command to module to read the channel number
        HAL_UART_Transmit(device_uart, (uint8_t *)"AT+RC\r\n", 7, 1000);

        // Create variables to store channel information
        uint8_t channelNumber[MAX_CHANNEL_NUMBER];
        uint8_t channelNum = 0;
        uint8_t channelNumLength = 0;

        // Wait until serialLinBuffReady(&serialLinBuff2) is set to 1
        while (serialLinBuffReady(&serialLinBuff2) == 0);

        // Extract the channel number from received data
        for (int i = 0; i < serialLinBuff2.currIndex; i++)
        {
            if (serialLinBuff2.buff[i] >= '0' && serialLinBuff2.buff[i] <= '9')
            {
                channelNumber[channelNumLength] = serialLinBuff2.buff[i];
                channelNumLength++;
            }
        }
        serialLinBuffReset(&serialLinBuff2);

        // Convert channelNumber to integer
        for (int i = 0; i < channelNumLength; i++)
        {
            channelNum = channelNum * 10 + (channelNumber[i] - '0');
        }

        // Reset buttonTimer
        buttonTimer = 0;

        // HAL_UART_Transmit(pc_uart, (uint8_t *)"Channel number currently is: ", 28, 1000);
        // HAL_UART_Transmit(pc_uart, (uint8_t *)channelNumber, channelNumLength, 1000);
        // HAL_UART_Transmit(pc_uart, (uint8_t *)"\n", 1, 1000);
        // HAL_UART_Transmit(pc_uart, (uint8_t *)"\r", 1, 1000);


        // Start the button press timer
        HAL_TIM_Base_Start_IT(&htim3);

        // Loop to increase the channel number by 1
        while (AT_Flag == 1)
        {
            // Check if the button is pressed
            if (HAL_GPIO_ReadPin(BUTTON_GPIO_Port, BUTTON_Pin) == GPIO_PIN_SET)
            {
                HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, 1);
                buttonTimer = 0; // Reset the timer
                HAL_Delay(300); // Debounce the button

                // Increase the channel number by 1
                channelNum++;
                if (channelNum > MAX_CHANNEL_NUMBER)
                {
                    /// Assing the channel number to 000. We will increase it by 1 before sending it to the HC-12 module
                    channelNumber[0] = '0';
                    channelNumber[1] = '0';
                    channelNumber[2] = '0';
                    channelNum = 1; // Wrap around if channel number exceeds maximum
                }

                plus_one(channelNumber, channelNumLength);

                HAL_UART_Transmit(device_uart, (uint8_t *)"AT+C", 4, 1000);
                HAL_UART_Transmit(device_uart, channelNumber, channelNumLength, 1000);
                HAL_UART_Transmit(device_uart, (uint8_t *)"\r\n", 2, 1000);

                // HAL_UART_Transmit(pc_uart, (uint8_t *)"AT+C", 4, 1000);
                // HAL_UART_Transmit(pc_uart, channelNumber, channelNumLength, 1000);
                // HAL_UART_Transmit(pc_uart, (uint8_t *)"\r\n", 2, 1000);

                // HAL_UART_Transmit(pc_uart, (uint8_t *)"Channel number is set to: ", 26, 1000);
                // HAL_UART_Transmit(pc_uart, channelNumber, channelNumLength, 1000);
                // HAL_UART_Transmit(pc_uart, (uint8_t *)"\n", 1, 1000);
                // HAL_UART_Transmit(pc_uart, (uint8_t *)"\r", 1, 1000);
                
                HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, 0);

            }

            // Check if buttonTimer exceeds 5000 (5 seconds)
            if (buttonTimer >= 5000)
            {
                // Exit AT mode
                AT_Flag = 0;
                // HAL_UART_Transmit(pc_uart, (uint8_t *)"Stop AT mode\r\n", 14, 1000);
                HAL_GPIO_WritePin(HC12_SET_GPIO_Port, HC12_SET_Pin, GPIO_PIN_SET); // Set the HC-12 module to normal mode
                break;
            }
        }

        indicateOnLED(channelNum);

    }
    else
    {
        HAL_GPIO_WritePin(HC12_SET_GPIO_Port, HC12_SET_Pin, GPIO_PIN_SET); // Set the HC-12 module to normal mode
    }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == htim3.Instance)
    {
        buttonTimer++; // Increment the timer variable
    }
}



void indicateOnLED(uint8_t channelNum)
{
    for (int i = 0; i < channelNum; i++)
    {
        HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, GPIO_PIN_SET);
        HAL_Delay(200);
        HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, GPIO_PIN_RESET);
        HAL_Delay(800);
    }
}



/// Parse the incomming text message
/// Get only the digits and '.' from the text message
/// The parsedTextMessageLength have to be 8 
/// For example textMessage = "ST,GS,01  9.35 kg". We have parsed the text message to "9.35". It is 4 caracters long. Fill the rest with 0 and make it 00009.35
uint8_t parseTextMessage(char *textMessage, uint8_t textMessageLength, uint8_t *parsedTextMessage)
{
    uint8_t parsedTextMessageLength = 8;
    for (int i = textMessageLength - 1; i >= 0; i--)
    {
        if ((textMessage[i] >= '0' && textMessage[i] <= '9') || textMessage[i] == '.')
        {
            parsedTextMessageLength--;
            parsedTextMessage[parsedTextMessageLength] = textMessage[i];
        }
        else if(textMessage[i] == '-')
        {
            /// Sett the parsedTextMessage to '---'
            parsedTextMessage[0] = '-';
            parsedTextMessage[1] = '-';
            parsedTextMessage[2] = '-';
            parsedTextMessage[3] = 0;
            parsedTextMessage[4] = 0;
            parsedTextMessage[5] = 0;
            parsedTextMessage[6] = 0;
            parsedTextMessage[7] = 0;        

            
            return (uint8_t)8;
        }
    }

    for (int i = 0; i < parsedTextMessageLength; i++)
    {
        parsedTextMessage[i] = '0';
    }
    return (uint8_t)8;
}


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
  MX_TIM3_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */

  HAL_UART_Receive_IT(pc_uart, uartByteBuff, 1); // Start reception for PC USART
  HAL_UART_Receive_IT(device_uart, uartByteBuff2, 1); // Start reception for Device USART

  HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, GPIO_PIN_RESET);

  

  /// Transmit 'AT+RC' to the HC-12 module
  HAL_UART_Transmit(device_uart, (uint8_t *)"AT+RC\r\n", 7, 1000);

  /// Create a buffer to store the channel number
  uint8_t channelNumber[10];
  /// Create a variable to store the channel number
  uint8_t channelNum = 0;
  /// Create a variable to store the channel number length
  uint8_t channelNumLength = 0;
  /// Wait until serialLinBuffReady(&serialLinBuff2) is set to 1
  while (serialLinBuffReady(&serialLinBuff2) == 0)
  {
    // HAL_UART_Transmit(pc_uart, (uint8_t *)"Waiting for the channel number\r\n", 32, 1000);
    
  }

  /// If serialLinBuffReady(&serialLinBuff2) is set to 1
  if(serialLinBuffReady(&serialLinBuff2))
  {
    /// Copy the serialLinBuff2.buff to the channelNumber
    for(int i = 0; i < serialLinBuff2.currIndex; i++)
    {
      if(serialLinBuff2.buff[i] >= '0' && serialLinBuff2.buff[i] <= '9')
      {
        channelNumber[channelNumLength] = serialLinBuff2.buff[i];
        channelNumLength++;
      }
    }
    serialLinBuffReset(&serialLinBuff2);
  }

  /// SAve the channel number to the flash memory
  // Flash_Write_Data(HC12_CHANNEL_NUMBER_ADDRESS, (uint32_t *)channelNumber, channelNumLength);

  // HAL_UART_Transmit(pc_uart, (uint8_t *)"Channel number is: ", 19, 1000);
  // HAL_UART_Transmit(pc_uart, (uint8_t *)channelNumber, channelNumLength, 1000);
  // HAL_UART_Transmit(pc_uart, (uint8_t *)"\n", 1, 1000);
  // HAL_UART_Transmit(pc_uart, (uint8_t *)"\r", 1, 1000);

  /// Convert the channelNumber to a number
  for(int i = 0; i < channelNumLength; i++)
  {
    channelNum = channelNum * 10 + (channelNumber[i] - '0');
  }

  indicateOnLED(channelNum);

  HAL_GPIO_WritePin(HC12_SET_GPIO_Port, HC12_SET_Pin, GPIO_PIN_SET);

  // HAL_UART_Transmit(pc_uart, (uint8_t *)"Starting_the_while_loop\r\n", 25, 1000);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    if(HAL_GPIO_ReadPin(BUTTON_GPIO_Port, BUTTON_Pin) == GPIO_PIN_SET)
    {
      /// If the button is pressed for 5 seconds, set the HC-12 module to AT mode
      /// After that, call the AT mode function
      HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, GPIO_PIN_SET);
      /// wait for 5 seconds
      HAL_Delay(5000);
      /// Check the button state again
      if(HAL_GPIO_ReadPin(BUTTON_GPIO_Port, BUTTON_Pin) == GPIO_PIN_SET)
      {
        // /// Set the AT_Flag to 1
        AT_Flag = 1;
        // /// Set the HC-12 module to AT mode
        HAL_GPIO_WritePin(HC12_SET_GPIO_Port, HC12_SET_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, GPIO_PIN_RESET);

        HAL_UART_Transmit(pc_uart, (uint8_t *)"AT mode is in set mode\r\n", 24, 1000);
        HC12_Set();

        HAL_UART_Transmit(pc_uart, (uint8_t *)"AT mode is in normal mode\r\n", 26, 1000);
        // while (1)
        // {
        //   /* code */
        // }
        
      }
      HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, GPIO_PIN_RESET);
    }

    if(serialLinBuffReady(&serialLinBuff))
    {
      parsedWeightValueLength = parseTextMessage(serialLinBuff.buff, serialLinBuff.currIndex, parsedWeightValue);
      HAL_UART_Transmit(device_uart, (uint8_t *)parsedWeightValue, parsedWeightValueLength, 1000);

      // HAL_UART_Transmit(pc_uart, (uint8_t *)"Parsed weight value: ", 21, 1000);
      // HAL_UART_Transmit(pc_uart, (uint8_t *)parsedWeightValue, parsedWeightValueLength, 1000);
      // HAL_UART_Transmit(pc_uart, (uint8_t *)"\n", 1, 1000);
      // HAL_UART_Transmit(pc_uart, (uint8_t *)"\r", 1, 1000);

      serialLinBuffReset(&serialLinBuff);


    }

    if(serialLinBuffReady(&serialLinBuff2))
    {
      HAL_UART_Transmit(pc_uart, (uint8_t *)serialLinBuff2.buff, serialLinBuff2.currIndex, 1000);

      serialLinBuffReset(&serialLinBuff2);
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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1;
  PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK1;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 3999;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 1;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 9600;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

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
  huart2.Init.BaudRate = 9600;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

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
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, HC12_SET_Pin|LED_RED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : BUTTON_Pin */
  GPIO_InitStruct.Pin = BUTTON_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(BUTTON_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : HC12_SET_Pin LED_RED_Pin */
  GPIO_InitStruct.Pin = HC12_SET_Pin|LED_RED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

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

#ifdef  USE_FULL_ASSERT
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
