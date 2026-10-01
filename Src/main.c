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
#include "usb_device.h"
#include "usbd_cdc_if.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "icm42688.h"
#include "ssd1306.h"
#include "ssd1306_fonts.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* Axis orientation glyph: bottom-right corner, bottom 16 rows of the screen. */
#define AXIS_INDICATOR_X      (SSD1306_WIDTH - AXIS_INDICATOR_WIDTH)
#define AXIS_INDICATOR_Y      (SSD1306_HEIGHT - AXIS_INDICATOR_HEIGHT)
#define AXIS_INDICATOR_WIDTH  23U
#define AXIS_INDICATOR_HEIGHT 16U
#define AXIS_INDICATOR_BYTES  ((AXIS_INDICATOR_WIDTH + 7U) / 8U)

/* Clamp OLED counters to 8 digits so the status lines stay clear of the glyph. */
#define CLAMP_COUNTER(value)                                                 \
  ((uint32_t)((value) > 99999999U ? 99999999UL : (uint32_t)(value)))

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

I2C_HandleTypeDef hi2c1;
I2C_HandleTypeDef hi2c2;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */

#define ACCEL_SAMPLE_QUEUE_SIZE 8192U
#define CDC_PACKET_SIZE         256U
#define CDC_MAX_SAMPLES         16U
#define RMS_AVERAGE_WINDOW_COUNT 20U

static ICM42688_AccelSample accel_sample_queue[ACCEL_SAMPLE_QUEUE_SIZE];
static uint32_t accel_sequence_queue[ACCEL_SAMPLE_QUEUE_SIZE];
static volatile uint16_t accel_queue_head = 0U;
static volatile uint16_t accel_queue_tail = 0U;
static volatile uint32_t accel_queue_overruns = 0U;
static volatile uint32_t accel_i2c_failures = 0U;
static volatile uint32_t accel_samples_discarded = 0U;
static volatile uint32_t usb_packets_transmitted = 0U;
static volatile uint32_t usb_samples_transmitted = 0U;
static volatile uint32_t usb_busy_events = 0U;
static volatile uint32_t accel_sequence = 0U;
static uint8_t cdc_stream_buffers[2][CDC_PACKET_SIZE];
static uint8_t cdc_buffer_index = 0U;
static int64_t rms_sum[3] = {0, 0, 0};
static int64_t rms_square_sum[3] = {0, 0, 0};
static uint16_t rms_sample_count = 0U;
static float rms_history[RMS_AVERAGE_WINDOW_COUNT][3];
static float rms_history_sum[3] = {0.0f, 0.0f, 0.0f};
static uint16_t rms_history_count = 0U;
static uint16_t rms_history_index = 0U;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC1_Init(void);
static void MX_I2C1_Init(void);
static void MX_I2C2_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */

static void TransmitAccelSamples(void);
static void UpdateRmsDisplay(const ICM42688_AccelSample *samples,
                             uint16_t sample_count);
static void DrawAxisIndicator(void);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/**
  * @brief  Draw the sensor axis orientation glyph in the bottom-right corner.
  *
  * Tells the user how the displayed axes map onto the physical device: the X
  * axis runs along the long (128 px) edge of the display, the Y axis along the
  * short (64 px) edge. The two arrows cross at the sensor origin and point in
  * the positive direction of each axis.
  *
  *  ..............#.......   Y arrow tip
  *  .............###......
  *  ..............#.......
  *  ..............#..#...#   Y label
  *  ..............#..#...#
  *  ..............#...#.#.
  *  ....#...#.....#....#..
  *  .....#.#......#....#..
  *  ......#.......#.......   X label
  *  .....#.#......#.......
  *  ....#...#.....#.......
  *  ..............#.......
  *  ..#...........#.......   X arrow head
  *  .#################....   X axis line
  *  ..#...........#.......   origin (axis crossing)
  */
static void DrawAxisIndicator(void)
{
  /* Row-major, 1 bit per pixel; MSB of byte N is column N * 8. */
  static const uint8_t glyph[AXIS_INDICATOR_HEIGHT][AXIS_INDICATOR_BYTES] = {
      {0x00, 0x02, 0x00}, {0x00, 0x07, 0x00}, {0x00, 0x02, 0x00},
      {0x00, 0x02, 0x44}, {0x00, 0x02, 0x44}, {0x00, 0x02, 0x28},
      {0x08, 0x82, 0x10}, {0x05, 0x02, 0x10}, {0x02, 0x02, 0x00},
      {0x05, 0x02, 0x00}, {0x08, 0x82, 0x00}, {0x00, 0x02, 0x00},
      {0x20, 0x02, 0x00}, {0x7F, 0xFF, 0xC0}, {0x20, 0x02, 0x00},
      {0x00, 0x02, 0x00},
  };

  ssd1306_DrawBitmap(AXIS_INDICATOR_X, AXIS_INDICATOR_Y,
                     (const unsigned char *)&glyph[0][0], AXIS_INDICATOR_WIDTH,
                     AXIS_INDICATOR_HEIGHT, White);
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
  MX_ADC1_Init();
  MX_I2C1_Init();
  MX_I2C2_Init();
  MX_USART1_UART_Init();
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN 2 */

  ssd1306_Init();
  ssd1306_Fill(Black);
  ssd1306_SetCursor(0, 0);
  ssd1306_WriteString("Starting...", Font_6x8, White);
  ssd1306_UpdateScreen();

  if (ICM42688_Init(&hi2c1) != HAL_OK)
  {
    ssd1306_Fill(Black);
    ssd1306_SetCursor(0, 0);
    ssd1306_WriteString("IMU NO CONNECTION", Font_6x8, White);
    ssd1306_UpdateScreen();
    Error_Handler();
  }

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    TransmitAccelSamples();
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
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

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_8;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
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
  hi2c1.Init.ClockSpeed = 400000;
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
  * @brief I2C2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C2_Init(void)
{

  /* USER CODE BEGIN I2C2_Init 0 */

  /* USER CODE END I2C2_Init 0 */

  /* USER CODE BEGIN I2C2_Init 1 */

  /* USER CODE END I2C2_Init 1 */
  hi2c2.Instance = I2C2;
  hi2c2.Init.ClockSpeed = 100000;
  hi2c2.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c2.Init.OwnAddress1 = 0;
  hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c2.Init.OwnAddress2 = 0;
  hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C2_Init 2 */

  /* USER CODE END I2C2_Init 2 */

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
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_RESET);

  /*Configure GPIO pin : PA15 */
  GPIO_InitStruct.Pin = GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PB5 */
  GPIO_InitStruct.Pin = GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* Enable the EXTI9_5 interrupt line so the ICM-42688 data-ready
     interrupt on PB5 can wake the main loop. */
  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

static void TransmitAccelSamples(void)
{
  if (CDC_IsHostReady() == 0U)
  {
    ICM42688_AccelSample rms_samples[CDC_MAX_SAMPLES];
    uint16_t sample_count = 0U;

    while (accel_queue_tail != accel_queue_head &&
           sample_count < CDC_MAX_SAMPLES)
    {
      rms_samples[sample_count] =
          accel_sample_queue[accel_queue_tail &
                             (ACCEL_SAMPLE_QUEUE_SIZE - 1U)];
      __disable_irq();
      accel_queue_tail = (uint16_t)((accel_queue_tail + 1U) &
                                    (ACCEL_SAMPLE_QUEUE_SIZE - 1U));
      __enable_irq();
      sample_count++;
    }

    if (sample_count != 0U)
    {
      accel_samples_discarded += sample_count;
      UpdateRmsDisplay(rms_samples, sample_count);
    }
    return;
  }

  while (accel_queue_tail != accel_queue_head)
  {
    uint16_t tail = accel_queue_tail;
    uint16_t head = accel_queue_head;
    uint16_t sample_count = 0U;
    uint16_t buffer_length = 0U;
    char line[40];
    ICM42688_AccelSample packet_samples[CDC_MAX_SAMPLES];
    uint8_t *cdc_stream_buffer = cdc_stream_buffers[cdc_buffer_index];

    while (tail != head && sample_count < CDC_MAX_SAMPLES)
    {
      ICM42688_AccelSample sample;
      uint16_t sample_tail = tail;
      sample = accel_sample_queue[sample_tail & (ACCEL_SAMPLE_QUEUE_SIZE - 1U)];
      uint32_t sequence = accel_sequence_queue[
          sample_tail & (ACCEL_SAMPLE_QUEUE_SIZE - 1U)];

      int written = snprintf(line, sizeof(line),
                             "S0,%lu,%d,%d,%d\r\n",
                             (unsigned long)sequence,
                             (int)sample.x,
                             (int)sample.y,
                             (int)sample.z);
      if (written <= 0 || (uint32_t)written >= CDC_PACKET_SIZE - buffer_length)
      {
        break;
      }
      packet_samples[sample_count] = sample;
      memcpy(&cdc_stream_buffer[buffer_length], line, (size_t)written);
      buffer_length = (uint16_t)(buffer_length + (uint16_t)written);
      sample_count = (uint16_t)(sample_count + 1U);
      tail = (uint16_t)((sample_tail + 1U) & (ACCEL_SAMPLE_QUEUE_SIZE - 1U));
    }

    if (sample_count == 0U)
    {
      break;
    }

    if (CDC_Transmit_FS(cdc_stream_buffer, buffer_length) == USBD_OK)
    {
      __disable_irq();
      accel_queue_tail = tail;
      __enable_irq();
      usb_packets_transmitted++;
      usb_samples_transmitted += sample_count;
      UpdateRmsDisplay(packet_samples, sample_count);
      cdc_buffer_index ^= 1U;
    }
    else
    {
      usb_busy_events++;
      break;
    }
  }
}

static void FormatRmsValue(char *buffer, uint16_t value_centi)
{
  if (value_centi >= 10000U)
  {
    snprintf(buffer, 6U, "%u", (unsigned int)((value_centi + 50U) / 100U));
  }
  else
  {
    snprintf(buffer, 6U, "%u.%02u",
             (unsigned int)(value_centi / 100U),
             (unsigned int)(value_centi % 100U));
  }
}

static void UpdateRmsDisplay(const ICM42688_AccelSample *samples,
                             uint16_t sample_count)
{
  char line[32];
  char current_text[6];
  char average_text[6];
  char peak_text[6];

  for (uint16_t index = 0U; index < sample_count; ++index)
  {
    const ICM42688_AccelSample *sample = &samples[index];
    int32_t values[3] = {sample->x, sample->y, sample->z};
    for (uint32_t axis = 0U; axis < 3U; ++axis)
    {
      rms_sum[axis] += values[axis];
      rms_square_sum[axis] += (int64_t)values[axis] * values[axis];
    }
    rms_sample_count = (uint16_t)(rms_sample_count + 1U);

    if (rms_sample_count == 500U)
    {
      float rms[3];
      float average[3];
      const float acceleration_scale = 9.80665f / 2048.0f;
      for (uint32_t axis = 0U; axis < 3U; ++axis)
      {
        float mean = (float)rms_sum[axis] / 500.0f;
        float variance = ((float)rms_square_sum[axis] / 500.0f) - (mean * mean);
        rms[axis] = sqrtf(fmaxf(variance, 0.0f)) * acceleration_scale;
        rms_history_sum[axis] -= rms_history[rms_history_index][axis];
        rms_history[rms_history_index][axis] = rms[axis];
        rms_history_sum[axis] += rms[axis];
        average[axis] = rms_history_sum[axis] /
            (float)((rms_history_count < RMS_AVERAGE_WINDOW_COUNT)
                        ? (rms_history_count + 1U)
                        : RMS_AVERAGE_WINDOW_COUNT);
        rms_sum[axis] = 0;
        rms_square_sum[axis] = 0;
      }
      if (rms_history_count < RMS_AVERAGE_WINDOW_COUNT)
      {
        rms_history_count++;
      }
      rms_history_index = (uint16_t)((rms_history_index + 1U) %
                                     RMS_AVERAGE_WINDOW_COUNT);
      rms_sample_count = 0U;

      ssd1306_Fill(Black);
      ssd1306_SetCursor(0, 0);
      ssd1306_WriteString("AX NOW AVG PEAK m/s2", Font_6x8, White);
      for (uint32_t axis = 0U; axis < 3U; ++axis)
      {
        const char axis_name = "XYZ"[axis];
        float peak = 0.0f;
        for (uint16_t history_index = 0U;
             history_index < rms_history_count;
             ++history_index)
        {
          if (rms_history[history_index][axis] > peak)
          {
            peak = rms_history[history_index][axis];
          }
        }
        uint16_t current_centi = (uint16_t)(rms[axis] * 100.0f + 0.5f);
        uint16_t peak_centi = (uint16_t)(peak * 100.0f + 0.5f);
        FormatRmsValue(current_text, current_centi);
        FormatRmsValue(peak_text, peak_centi);
        if (rms_history_count == RMS_AVERAGE_WINDOW_COUNT)
        {
          uint16_t average_centi =
              (uint16_t)(average[axis] * 100.0f + 0.5f);
          FormatRmsValue(average_text, average_centi);
          snprintf(line, sizeof(line), "%c:%s %s %s",
                   axis_name, current_text, average_text, peak_text);
        }
        else
        {
          snprintf(line, sizeof(line), "%c:%s --.-- %s",
                   axis_name, current_text, peak_text);
        }
        ssd1306_SetCursor(0, (uint8_t)((axis + 1U) * 8U));
        ssd1306_WriteString(line, Font_6x8, White);
      }
      snprintf(line, sizeof(line), "TX:%lu D:%lu",
               (unsigned long)CLAMP_COUNTER(usb_samples_transmitted),
               (unsigned long)CLAMP_COUNTER(accel_samples_discarded));
      ssd1306_SetCursor(0, 32);
      ssd1306_WriteString(line, Font_6x8, White);
      snprintf(line, sizeof(line), "Q:%lu I:%lu",
               (unsigned long)CLAMP_COUNTER(accel_queue_overruns),
               (unsigned long)CLAMP_COUNTER(accel_i2c_failures));
      ssd1306_SetCursor(0, 40);
      ssd1306_WriteString(line, Font_6x8, White);
      DrawAxisIndicator();
      ssd1306_UpdateScreen();
    }
  }
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if (GPIO_Pin == GPIO_PIN_5)
  {
    uint16_t head = accel_queue_head;
    uint16_t next_head = (uint16_t)((head + 1U) &
                                    (ACCEL_SAMPLE_QUEUE_SIZE - 1U));
    ICM42688_AccelSample sample;

    if (ICM42688_ReadAccel(&hi2c1, &sample) == HAL_OK)
    {
      uint32_t sequence = accel_sequence++;
      if (next_head == accel_queue_tail)
      {
        accel_queue_overruns++;
      }
      else
      {
        accel_sample_queue[head] = sample;
        accel_sequence_queue[head] = sequence;
        __DMB();
        accel_queue_head = next_head;
      }
    }
    else
    {
      accel_i2c_failures++;
    }
  }
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
