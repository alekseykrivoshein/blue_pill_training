/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <cstdio>
#include <numeric>
#include <iterator>

#include "constants.h"
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
ADC_HandleTypeDef hadc1;

I2C_HandleTypeDef hi2c1;

TIM_HandleTypeDef htim1;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_TIM1_Init(void);
static void MX_ADC1_Init(void);
static void MX_I2C1_Init(void);
/* USER CODE BEGIN PFP */
  enum class ButtonPressType {SHORT_PRESS, LONG_PRESS, NO_PRESS};
  ButtonPressType check_button_press(GPIO_TypeDef* port, uint16_t pin, uint32_t time_ms_short, uint32_t time_ms_long);

  thermoregulator::ChargingStatus charging_status();
  float get_battery_voltage(ADC_HandleTypeDef* hadc, int samples_size = 10);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
namespace {
uint8_t adc_tick = 0;
bool adc_ready = false;

class OperatingMode {
  public:
    OperatingMode() :
      params_(thermoregulator::constants::low_mode) {
        HAL_GPIO_WritePin(thermoregulator::constants::mode_led1.port, thermoregulator::constants::mode_led1.pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(thermoregulator::constants::mode_led2.port, thermoregulator::constants::mode_led2.pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(thermoregulator::constants::mode_led3.port, thermoregulator::constants::mode_led3.pin, GPIO_PIN_RESET);
      }

    void change_mode() {
      auto cur_mode = static_cast<int>(params_.mode);
      auto mode_count = static_cast<int>(thermoregulator::OperatingModeType::MODE_COUNT);
      thermoregulator::OperatingModeType next_mode_type = static_cast<thermoregulator::OperatingModeType>((cur_mode + 1) % mode_count);

      switch (next_mode_type)
      {
      case thermoregulator::OperatingModeType::LOW:
        params_ = thermoregulator::constants::low_mode;
        HAL_GPIO_WritePin(thermoregulator::constants::mode_led1.port, thermoregulator::constants::mode_led1.pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(thermoregulator::constants::mode_led2.port, thermoregulator::constants::mode_led2.pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(thermoregulator::constants::mode_led3.port, thermoregulator::constants::mode_led3.pin, GPIO_PIN_RESET);
        printf("low mode, yellow address LED\r\n"); //коммент. Конструкция устройства изменилась, теперь цвет адресного светодиода не зависит от режима, зависит только от уровня батареи
        break;
      case thermoregulator::OperatingModeType::MIDDLE:
        params_ = thermoregulator::constants::middle_mode;
        HAL_GPIO_WritePin(thermoregulator::constants::mode_led1.port, thermoregulator::constants::mode_led1.pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(thermoregulator::constants::mode_led2.port, thermoregulator::constants::mode_led2.pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(thermoregulator::constants::mode_led3.port, thermoregulator::constants::mode_led3.pin, GPIO_PIN_RESET);
        printf("middle mode, orange address LED\r\n");
        break;
      case thermoregulator::OperatingModeType::HIGH:
        params_ = thermoregulator::constants::high_mode;
        HAL_GPIO_WritePin(thermoregulator::constants::mode_led1.port, thermoregulator::constants::mode_led1.pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(thermoregulator::constants::mode_led2.port, thermoregulator::constants::mode_led2.pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(thermoregulator::constants::mode_led3.port, thermoregulator::constants::mode_led3.pin, GPIO_PIN_SET);
        printf("high mode, red address LED\r\n");
        break;
      default:
        printf("unknown operating mode type\r\n");
        break;
      }
    }

    thermoregulator::OperatingModeParams current_mode() const {
      return params_;
    }
  private:
    thermoregulator::OperatingModeParams params_;
};
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
  ++adc_tick;
}

#ifdef __cplusplus
 extern "C" {
#endif
int __io_putchar(int ch) {
  HAL_UART_Transmit(&huart1, reinterpret_cast<uint8_t*>(&ch), 1, 100);
  return ch;
}
#ifdef __cplusplus
}
#endif
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
  MX_USART1_UART_Init();
  MX_TIM1_Init();
  MX_ADC1_Init();
  MX_I2C1_Init();
  /* USER CODE BEGIN 2 */
  HAL_TIM_Base_Start_IT(&htim1);
  HAL_ADCEx_Calibration_Start(&hadc1);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  OperatingMode mode;

  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    auto button_press_state = check_button_press(GPIOA, GPIO_PIN_0, 50, 3000);
    if(button_press_state == ButtonPressType::SHORT_PRESS) {
      printf("short button press\r\n");
      mode.change_mode();
    } else if(button_press_state == ButtonPressType::LONG_PRESS) {
      printf("long button press, powering off\r\n"); 
    }

    if(adc_ready) {
      adc_ready = false;

      auto bat_voltage = get_battery_voltage(&hadc1);
      printf("battery voltage: %f\r\n", bat_voltage); //коммент. Вот здесь можно менять цвет светодиода от зелёного до красного
    }

    if(adc_tick >= 10) {
      adc_tick = 0;
      adc_ready = true;
      HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);

      auto charge_state = charging_status();
      switch (charge_state)
      {
      case thermoregulator::ChargingStatus::DEVICE_CHARGING:
        printf("device is charging\r\n");  //коммент. Добавить мерцание адресного светодиода (плавное нарастание и убывание яркости) синим цветом
        break;
      case thermoregulator::ChargingStatus::DEVICE_CHARGED:
        printf("device is charged\r\n"); //коммент. Добавить свечение адресного светодиода синим цветом
        break;
      case thermoregulator::ChargingStatus::DEVICE_WORKING:
        printf("device is working\r\n"); //коммент. Добавить свечение адресного светодиода цветом в зависимости от уровня батареи
        break;
      default:
        printf("unknown charging status\r\n");
        break;
    }
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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */
  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc1.Init.ContinuousConvMode = ENABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_4;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_13CYCLES_5;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 7199;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 9999;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */

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
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14, GPIO_PIN_RESET);

  /*Configure GPIO pin : PC13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PA0 PA2 PA3 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_2|GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PB12 PB13 PB14 */
  GPIO_InitStruct.Pin = GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */
thermoregulator::ChargingStatus charging_status() {
  bool state1 = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_2);
  bool state2 = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_3);

  thermoregulator::ChargingStatus res;
  if(state1 && state2) {
    res = thermoregulator::ChargingStatus::DEVICE_WORKING;
  } else if(!state1 && state2) {
    res = thermoregulator::ChargingStatus::DEVICE_CHARGING;
  } else if(state1 && !state2) {
    res = thermoregulator::ChargingStatus::DEVICE_CHARGED;
  } else {
    res = thermoregulator::ChargingStatus::UNKNOWN;
  }

  return res;
}

ButtonPressType check_button_press(GPIO_TypeDef* port, uint16_t pin, uint32_t time_ms_short, uint32_t time_ms_long) {
  auto result = ButtonPressType::NO_PRESS;
  auto curr_time = HAL_GetTick();
  auto diff_time = time_ms_long - time_ms_short;

  if(HAL_GPIO_ReadPin(port, pin)) {
    result = ButtonPressType::SHORT_PRESS;
    // debounce time
    HAL_Delay(time_ms_short);

    while(HAL_GPIO_ReadPin(port, pin)) {
      if(HAL_GetTick() - curr_time >= diff_time) {
        result = ButtonPressType::LONG_PRESS;
        break;
      }
    }
  }
  // short delay to counter debounce on release
  HAL_Delay(100);
  return result;
}

float get_battery_voltage(ADC_HandleTypeDef* hadc, int samples_size) {
  uint32_t samples[samples_size];

  HAL_ADC_Start(hadc);
  for(auto i = 0; i < samples_size; ++i) {
    HAL_ADC_PollForConversion(hadc, 1);
    samples[i] = HAL_ADC_GetValue(hadc);
  }
  HAL_ADC_Stop(hadc);

  auto average = std::accumulate(samples, samples + samples_size, 0) / samples_size;
  // TODO: change 4095 to constant 12 bit integer max val
  return thermoregulator::constants::vbat / 4095 * average;
}
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

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
