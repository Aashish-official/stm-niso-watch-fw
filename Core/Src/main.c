/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body for the Primary Watch MCU
  ******************************************************************************
  * @attention
  *
  * This version implements a robust master/slave protocol. It initiates
  * communication by sending a "WAKEUP_CMD" to the coprocessor and then
  * waits for its acknowledgement before proceeding. The data packet
  * transmission is now sent in one atomic block for reliability.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "lcd.h"
#include "pic.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum {
    STATE_ACTIVE,
    STATE_COMMUNICATE,
    STATE_SLEEP
} AppState_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
// --- General Timing ---
#define ACTIVE_MODE_DURATION_MS 60000
#define SLEEP_MODE_DURATION_S   60
#define PERIODIC_COMMS_INTERVAL_MS 15000

// --- Thermistor Calculation Constants ---
#define SERIES_RESISTANCE   10000.0f
#define NOMINAL_RESISTANCE  10000.0f
#define NOMINAL_TEMPERATURE 298.15f
#define B_COEFFICIENT       3950.0f
#define ADC_MAX             4095.0f

// --- Fall Detection (ISM330) Constants ---
#define ISM330_I2C_ADDR             0xD6
#define WHO_AM_I_REG                0x0F
#define CTRL1_XL_REG                0x10
#define CTRL10_C_REG                0x19
#define FREE_FALL_REG               0x5D
#define MD1_CFG_REG                 0x5E
#define FUNC_CFG_ACCESS_REG         0x01

// --- Coprocessor Communication Protocol ---
#define WAKEUP_CMD                  0xAA
#define COPRO_TIMEOUT_MS            5000
#define PACKET_START_BYTE           0x7E
#define PACKET_END_BYTE             0x7F
#define ACK_READY                   "ACK_READY"
#define ACK_PACKET_OK               "ACK_PACKET_OK"
#define ACK_HTTP_OK                 "ACK_HTTP_OK"
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
I2C_HandleTypeDef hi2c3;
UART_HandleTypeDef hlpuart1;
UART_HandleTypeDef huart1;
RTC_HandleTypeDef hrtc;
SPI_HandleTypeDef hspi1;

/* USER CODE BEGIN PV */
volatile uint8_t fall_detected_flag = 0;
volatile float g_temperature_celsius = -99.0f;
AppState_t g_app_state = STATE_ACTIVE;
uint32_t active_mode_start_tick = 0;
uint32_t last_comms_tick = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void PeriphCommonClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C3_Init(void);
static void MX_SPI1_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_ADC1_Init(void);
static void MX_RTC_Init(void);
static void MX_LPUART1_UART_Init(void);
/* USER CODE BEGIN PFP */
void ISM330_FallDetection_Init(void);
void Update_Display(float temp, int wifi_status);
void Enter_Sleep_Mode(void);
void ReInit_Peripherals_After_Wakeup(void);
void DeInit_Peripherals_For_Sleep(void);
float Read_Temperature(void);
int Communicate_With_Coprocessor(uint8_t* data, uint8_t len);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
extern void LCD_Init(void);
extern void LCD_Fill(uint16_t xsta, uint16_t ysta, uint16_t xend, uint16_t yend, uint16_t color);
extern void LCD_StrCenter(uint16_t x, uint16_t y,const char *s, uint16_t fc, uint16_t bc, uint8_t sizey);
extern void LCD_ShowPicture(uint16_t x, uint16_t y, uint16_t length, uint16_t width, const uint8_t pic[]);
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
  uint8_t sensor_data_packet[10];
  int comms_success = 0;
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/
  HAL_Init();

  SystemClock_Config();
  PeriphCommonClock_Config();
  MX_GPIO_Init();
  MX_I2C3_Init();
  MX_SPI1_Init();
  MX_USART1_UART_Init();
  MX_ADC1_Init();
  MX_RTC_Init();
  MX_LPUART1_UART_Init();

  /* USER CODE BEGIN 2 */
  printf("\r\n\r\n--- Primary MCU: System Booted ---\r\n");

  if (HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED) != HAL_OK)
  {
      Error_Handler();
  }
  printf("[DEBUG] ADC Calibrated.\r\n");

  LCD_Init();
  LCD_Fill(0, 0, 120, 240, BLACK);
  ISM330_FallDetection_Init();

  g_app_state = STATE_ACTIVE;
  active_mode_start_tick = HAL_GetTick();
  last_comms_tick = active_mode_start_tick;
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    switch(g_app_state)
    {
        case STATE_ACTIVE:
            g_temperature_celsius = Read_Temperature();
            Update_Display(g_temperature_celsius, comms_success);

            if (fall_detected_flag) {
                printf("[EVENT] Fall Detected! Preparing to communicate...\r\n");
                g_app_state = STATE_COMMUNICATE;
            }
            else if (HAL_GetTick() - last_comms_tick > PERIODIC_COMMS_INTERVAL_MS) {
                printf("[EVENT] Periodic timer expired! Preparing to communicate...\r\n");
                g_app_state = STATE_COMMUNICATE;
            }
            else if (HAL_GetTick() - active_mode_start_tick > ACTIVE_MODE_DURATION_MS) {
                printf("[SYSTEM] Active time (%lu ms) elapsed. Entering sleep.\r\n", ACTIVE_MODE_DURATION_MS);
                g_app_state = STATE_SLEEP;
            }
            HAL_Delay(1000);
            break;

        case STATE_COMMUNICATE:
            printf("\r\n[STATE] Communication Mode\r\n");
            sensor_data_packet[0] = fall_detected_flag ? 0x01 : 0x00;
            memcpy(&sensor_data_packet[1], &g_temperature_celsius, sizeof(float));
            printf("[COMMS] Packet created: Event=0x%02X, Temp=%.1f\r\n", sensor_data_packet[0], g_temperature_celsius);

            comms_success = Communicate_With_Coprocessor(sensor_data_packet, 5);
            if (comms_success) {
                printf("[COMMS] ✅ Coprocessor communication sequence SUCCESSFUL.\r\n");
            } else {
                printf("[COMMS] ❌ Coprocessor communication sequence FAILED.\r\n");
            }
            Update_Display(g_temperature_celsius, comms_success);

            fall_detected_flag = 0;
            last_comms_tick = HAL_GetTick();
            printf("[SYSTEM] Returning to Active Mode.\r\n");
            g_app_state = STATE_ACTIVE;
            break;

        case STATE_SLEEP:
            printf("\r\n[STATE] Sleep Mode\r\n");
            Enter_Sleep_Mode();

            ReInit_Peripherals_After_Wakeup();

            printf("\r\n[SYSTEM] Wakeup complete. Returning to Active Mode.\r\n");
            comms_success = 0;
            g_app_state = STATE_ACTIVE;
            active_mode_start_tick = HAL_GetTick();
            last_comms_tick = active_mode_start_tick;
            break;

        default:
            g_app_state = STATE_ACTIVE;
            break;
    }
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_LSI1
                              |RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV1;
  RCC_OscInitStruct.PLL.PLLN = 32;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK4|RCC_CLOCKTYPE_HCLK2
                              |RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.AHBCLK2Divider = RCC_SYSCLK_DIV2;
  RCC_ClkInitStruct.AHBCLK4Divider = RCC_SYSCLK_DIV1;
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief Peripherals Common Clock Configuration
  */
void PeriphCommonClock_Config(void)
{
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_SMPS|RCC_PERIPHCLK_USART1
                              |RCC_PERIPHCLK_LPUART1|RCC_PERIPHCLK_I2C3
                              |RCC_PERIPHCLK_ADC;
  PeriphClkInitStruct.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2;
  PeriphClkInitStruct.Lpuart1ClockSelection = RCC_LPUART1CLKSOURCE_PCLK1;
  PeriphClkInitStruct.I2c3ClockSelection = RCC_I2C3CLKSOURCE_PCLK1;
  PeriphClkInitStruct.AdcClockSelection = RCC_ADCCLKSOURCE_PLLSAI1;
  PeriphClkInitStruct.SmpsClockSelection = RCC_SMPSCLKSOURCE_HSI;
  PeriphClkInitStruct.SmpsDivSelection = RCC_SMPSCLKDIV_RANGE1;
  PeriphClkInitStruct.PLLSAI1.PLLN = 16;
  PeriphClkInitStruct.PLLSAI1.PLLP = RCC_PLLP_DIV2;
  PeriphClkInitStruct.PLLSAI1.PLLQ = RCC_PLLQ_DIV2;
  PeriphClkInitStruct.PLLSAI1.PLLR = RCC_PLLR_DIV2;
  PeriphClkInitStruct.PLLSAI1.PLLSAI1ClockOut = RCC_PLLSAI1_ADCCLK;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  */
static void MX_ADC1_Init(void)
{
  ADC_ChannelConfTypeDef sConfig = {0};
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  hadc1.Init.LowPowerAutoWait = DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.Overrun = ADC_OVR_DATA_PRESERVED;
  hadc1.Init.OversamplingMode = DISABLE;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }
  sConfig.Channel = ADC_CHANNEL_2;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_2CYCLES_5;
  sConfig.SingleDiff = ADC_SINGLE_ENDED;
  sConfig.OffsetNumber = ADC_OFFSET_NONE;
  sConfig.Offset = 0;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C3 Initialization Function
  */
static void MX_I2C3_Init(void)
{
  hi2c3.Instance = I2C3;
  hi2c3.Init.Timing = 0x00702991;
  hi2c3.Init.OwnAddress1 = 0;
  hi2c3.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c3.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c3.Init.OwnAddress2 = 0;
  hi2c3.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c3.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c3.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c3) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c3, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c3, 0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief LPUART1 Initialization Function
  */
static void MX_LPUART1_UART_Init(void)
{
  hlpuart1.Instance = LPUART1;
  hlpuart1.Init.BaudRate = 115200;
  hlpuart1.Init.WordLength = UART_WORDLENGTH_8B;
  hlpuart1.Init.StopBits = UART_STOPBITS_1;
  hlpuart1.Init.Parity = UART_PARITY_NONE;
  hlpuart1.Init.Mode = UART_MODE_TX_RX;
  hlpuart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  hlpuart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  hlpuart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&hlpuart1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief RTC Initialization Function
  */
static void MX_RTC_Init(void)
{
  RTC_TimeTypeDef sTime = {0};
  RTC_DateTypeDef sDate = {0};
  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = 127;
  hrtc.Init.SynchPrediv = 255;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
  hrtc.Init.OutPutRemap = RTC_OUTPUT_REMAP_NONE;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }
  sTime.Hours = 0x0;
  sTime.Minutes = 0x0;
  sTime.Seconds = 0x0;
  sTime.SubSeconds = 0x0;
  sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  sTime.StoreOperation = RTC_STOREOPERATION_RESET;
  if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BCD) != HAL_OK)
  {
    Error_Handler();
  }
  sDate.WeekDay = RTC_WEEKDAY_MONDAY;
  sDate.Month = RTC_MONTH_JANUARY;
  sDate.Date = 0x1;
  sDate.Year = 0x0;
  if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BCD) != HAL_OK)
  {
    Error_Handler();
  }
  HAL_RTCEx_DeactivateWakeUpTimer(&hrtc);
}

/**
  * @brief SPI1 Initialization Function
  */
static void MX_SPI1_Init(void)
{
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 7;
  hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief USART1 Initialization Function
  */
static void MX_USART1_UART_Init(void)
{
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
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
}

/**
  * @brief GPIO Initialization Function
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();

  HAL_GPIO_WritePin(GPIOH, CS_DISP_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(GPIOC, D_C_DISP_Pin|RST_DISP_Pin, GPIO_PIN_RESET);

  GPIO_InitStruct.Pin = D_C_DISP_Pin|RST_DISP_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = CS_DISP_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(CS_DISP_GPIO_Port, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = INT1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  HAL_NVIC_SetPriority(EXTI2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI2_IRQn);
}

/* USER CODE BEGIN 4 */
#ifdef __GNUC__
int _write(int file, char *ptr, int len)
{
  (void)file;
  HAL_UART_Transmit(&huart1, (uint8_t*)ptr, len, HAL_MAX_DELAY);
  return len;
}
#endif

int Communicate_With_Coprocessor(uint8_t* data, uint8_t len)
{
    uint8_t rx_buffer[32] = {0};
    uint8_t packet_buffer[32];
    HAL_StatusTypeDef status;
    const uint8_t wakeup_cmd = WAKEUP_CMD;

    printf("\r\n--- Starting Coprocessor Handshake ---\r\n");

    // Step 1: Send the wakeup command
    printf("[STEP 1] Sending Wakeup Command (0x%02X)...\r\n", wakeup_cmd);
    HAL_UART_Transmit(&hlpuart1, &wakeup_cmd, 1, 200);

    // Step 2: Wait for the "ready" acknowledgement
    printf("[STEP 2] Waiting for '%s' from coprocessor...\r\n", ACK_READY);
    status = HAL_UART_Receive(&hlpuart1, rx_buffer, strlen(ACK_READY), COPRO_TIMEOUT_MS);
    if (status != HAL_OK) {
        printf("   [FAIL] Timeout waiting for ready signal. Status: %d\r\n", status);
        return 0;
    }
    if (strncmp((char*)rx_buffer, ACK_READY, strlen(ACK_READY)) != 0) {
        printf("   [FAIL] Invalid response received. Expected '%s', got '%.*s'.\r\n", ACK_READY, (int)strlen(ACK_READY), rx_buffer);
        return 0;
    }
    printf("   [OK] Coprocessor is ready. Received '%s'.\r\n", ACK_READY);

    // Step 3: Construct and send the entire data packet in one go
    uint8_t packet_len = len + 3; // Start + Len Byte + Payload + End
    packet_buffer[0] = PACKET_START_BYTE;
    packet_buffer[1] = len;
    memcpy(&packet_buffer[2], data, len);
    packet_buffer[2 + len] = PACKET_END_BYTE;

    printf("[STEP 3] Sending data packet (%d total bytes)...\r\n", packet_len);
    HAL_UART_Transmit(&hlpuart1, packet_buffer, packet_len, 500);

    // Step 4: Wait for packet acknowledgement
    memset(rx_buffer, 0, sizeof(rx_buffer));
    printf("[STEP 4] Waiting for '%s' confirmation...\r\n", ACK_PACKET_OK);
    status = HAL_UART_Receive(&hlpuart1, rx_buffer, strlen(ACK_PACKET_OK), COPRO_TIMEOUT_MS);
    if (status != HAL_OK) {
        printf("   [FAIL] Timeout waiting for packet ACK. Status: %d\r\n", status);
        return 0;
    }
     if (strncmp((char*)rx_buffer, ACK_PACKET_OK, strlen(ACK_PACKET_OK)) != 0) {
        printf("   [FAIL] Invalid response. Expected '%s', got '%.*s'.\r\n", ACK_PACKET_OK, (int)strlen(ACK_PACKET_OK), rx_buffer);
        return 0;
    }
    printf("   [OK] Received '%s'.\r\n", ACK_PACKET_OK);

    // Step 5: Wait for final HTTP acknowledgement
    memset(rx_buffer, 0, sizeof(rx_buffer));
    printf("[STEP 5] Waiting for final HTTP confirmation '%s'...\r\n", ACK_HTTP_OK);
    status = HAL_UART_Receive(&hlpuart1, rx_buffer, strlen(ACK_HTTP_OK), COPRO_TIMEOUT_MS * 3);
    if (status != HAL_OK) {
        printf("   [FAIL] Timeout waiting for HTTP ACK. Status: %d\r\n", status);
        return 0;
    }
    if (strncmp((char*)rx_buffer, ACK_HTTP_OK, strlen(ACK_HTTP_OK)) != 0) {
        printf("   [FAIL] Invalid final response. Expected '%s', got '%.*s'.\n", ACK_HTTP_OK, (int)strlen(ACK_HTTP_OK), rx_buffer);
        return 0;
    }
    printf("   [OK] Received '%s'. Handshake complete.\r\n", ACK_HTTP_OK);
    return 1;
}

float Read_Temperature(void)
{
    float temp_c = -99.0f;
    uint32_t adc_value = 0;
    printf("[SENSOR] Reading temperature...\r\n");
    HAL_ADC_Start(&hadc1);
    if (HAL_ADC_PollForConversion(&hadc1, 100) == HAL_OK)
    {
         adc_value = HAL_ADC_GetValue(&hadc1);
         if (adc_value > 10 && adc_value < (ADC_MAX - 10)) {
             float thermistor_resistance = SERIES_RESISTANCE * (adc_value / (ADC_MAX - (float)adc_value));
             float steinhart = logf(thermistor_resistance / NOMINAL_RESISTANCE) / B_COEFFICIENT;
             steinhart += 1.0f / NOMINAL_TEMPERATURE;
             temp_c = (1.0f / steinhart) - 273.15f;
             printf("   [OK] Temp: %.2f C (Raw ADC: %lu)\r\n", temp_c, adc_value);
         } else {
             printf("   [FAIL] ADC value out of range: %lu\r\n", adc_value);
         }
    } else {
        printf("   [FAIL] ADC Poll for conversion failed.\r\n");
    }
    HAL_ADC_Stop(&hadc1);
    return temp_c;
}

void Enter_Sleep_Mode(void) {
    printf("[SYSTEM] Preparing to enter STOP2 mode for %d seconds.\r\n", SLEEP_MODE_DURATION_S);
    LCD_Fill(0, 0, 120, 240, BLACK);
    LCD_StrCenter(0, 110, "Sleeping...", WHITE, BLACK, 16);
    HAL_Delay(100);
    DeInit_Peripherals_For_Sleep();
    printf("[SYSTEM] Setting RTC Wakeup Timer.\r\n");
    HAL_RTCEx_DeactivateWakeUpTimer(&hrtc);
    if (HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, SLEEP_MODE_DURATION_S, RTC_WAKEUPCLOCK_CK_SPRE_16BITS) != HAL_OK)
    {
        Error_Handler();
    }
    printf("[SYSTEM] Entering STOP2 Mode now. Bye!\r\n\r\n");
    HAL_PWREx_EnterSTOP2Mode(PWR_STOPENTRY_WFI);
}

void DeInit_Peripherals_For_Sleep(void)
{
    printf("[DEBUG] De-initializing peripherals for sleep...\r\n");
    HAL_SPI_DeInit(&hspi1);
    HAL_I2C_DeInit(&hi2c3);
    HAL_ADC_DeInit(&hadc1);
    HAL_UART_DeInit(&huart1);
    HAL_UART_DeInit(&hlpuart1);
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();
    GPIO_InitStruct.Pin = GPIO_PIN_All;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
    HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);
    __HAL_RCC_GPIOA_CLK_DISABLE();
    __HAL_RCC_GPIOB_CLK_DISABLE();
    __HAL_RCC_GPIOC_CLK_DISABLE();
    __HAL_RCC_GPIOD_CLK_DISABLE();
    __HAL_RCC_GPIOE_CLK_DISABLE();
    __HAL_RCC_GPIOH_CLK_DISABLE();
}

void ReInit_Peripherals_After_Wakeup(void)
{
    printf("[DEBUG] Re-initializing system after wake-up...\r\n");
    SystemClock_Config();
    PeriphCommonClock_Config();
    MX_GPIO_Init();
    MX_I2C3_Init();
    MX_SPI1_Init();
    MX_LPUART1_UART_Init();
    MX_USART1_UART_Init();
    MX_ADC1_Init();
    MX_RTC_Init();
    if (HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED) != HAL_OK)
    {
        Error_Handler();
    }
    printf("[DEBUG] ADC Re-calibrated.\r\n");
    LCD_Init();
    ISM330_FallDetection_Init();
}

void Update_Display(float temp, int wifi_status) {
    char temp_str[20];
    LCD_Fill(0, 0, 120, 240, BLACK);
    LCD_ShowPicture(20, 0, 70, 240, fun_image_50x50);
    if (temp > -99.0f) {
        sprintf(temp_str, "%.1f C", temp);
    } else {
        sprintf(temp_str, "--.- C");
    }
    LCD_StrCenter(0, 220, temp_str, WHITE, BLACK, 16);
    if (wifi_status) {
        LCD_ShowPicture(90, 210, 25, 25, wifi);
    } else {
        LCD_ShowPicture(90, 210, 25, 25, no_wifi);
    }
}

void ISM330_FallDetection_Init(void) {
  uint8_t val = 0;
  printf("[SENSOR] Initializing ISM330 Fall Detection...\r\n");
  if(HAL_I2C_Mem_Read(&hi2c3, ISM330_I2C_ADDR, WHO_AM_I_REG, 1, &val, 1, 100) != HAL_OK || val != 0x6B) {
      printf("   [FAIL] ISM330 Not Found! WHO_AM_I = 0x%02X\r\n", val);
      Error_Handler();
  }
  printf("   [OK] ISM330 Found.\r\n");
  val = 0x80; HAL_I2C_Mem_Write(&hi2c3, ISM330_I2C_ADDR, FUNC_CFG_ACCESS_REG, 1, &val, 1, 100);
  val = 0x60; HAL_I2C_Mem_Write(&hi2c3, ISM330_I2C_ADDR, CTRL1_XL_REG, 1, &val, 1, 100);
  val = 0x01; HAL_I2C_Mem_Write(&hi2c3, ISM330_I2C_ADDR, CTRL10_C_REG, 1, &val, 1, 100);
  val = 0x33; HAL_I2C_Mem_Write(&hi2c3, ISM330_I2C_ADDR, FREE_FALL_REG, 1, &val, 1, 100);
  val = 0x08; HAL_I2C_Mem_Write(&hi2c3, ISM330_I2C_ADDR, MD1_CFG_REG, 1, &val, 1, 100);
  val = 0x00; HAL_I2C_Mem_Write(&hi2c3, ISM330_I2C_ADDR, FUNC_CFG_ACCESS_REG, 1, &val, 1, 100);
  printf("   [OK] ISM330 Configured.\r\n");
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if(GPIO_Pin == INT1_Pin)
  {
    fall_detected_flag = 1;
  }
}

void HAL_RTCEx_WakeUpTimerEventCallback(RTC_HandleTypeDef *hrtc)
{
  printf("[INTERRUPT] RTC Wake-up event received.\r\n");
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  printf("---!!! PRIMARY MCU SYSTEM ERROR !!!---\r\n");
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
  printf("Wrong parameters value: file %s on line %lu\r\n", file, line);
}
#endif /* USE_FULL_ASSERT */
