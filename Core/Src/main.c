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
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "usbd_cdc_if.h"
#include "ism330dhcx_reg.h"
#include "NanoEdgeAI.h"
#include <stdio.h>
#include <math.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define SENSOR_BUS 		hspi3	/* SPI3 connected to the ISM330DHCX */
#define LEARN_NB		50	/* Number of learns before entering in detect mode */
#define NEAI_MODE		0	/* 0: Datalogging mode | 1: NEAI functions mode */
#define NEAI_NB_VERIFS	3	/* Number of inferences done before calculating a mean result */
#define THRESHOLD		90	/* NEAI threshold between nominal & abnormal */
#define COM_MODE		0	/* 0: HUART (USB on STLink) | 1: USB_CDC (USB on STWIN) */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
#define	BOOT_TIME	100 //ms
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi3;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
static stmdev_ctx_t dev_ctx;
static int16_t data_raw_acceleration[3];
static uint8_t whoamI, rst;
static uint8_t tx_buffer[1000];
float input_user_buffer[DATA_INPUT_USER * AXIS_NUMBER];	// Buffer of input values
char input_user_buffer_uint8[7 * DATA_INPUT_USER * AXIS_NUMBER + 1];
uint8_t learn_cpt = 0, signals = 0, neai_result = 0;
uint32_t nb_inferences = 0, mean_neai_result = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI3_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */
static int32_t platform_write(void *handle, uint8_t reg, const uint8_t *bufp, uint16_t len);
static int32_t platform_read(void *handle, uint8_t reg, uint8_t *bufp, uint16_t len);
static void tx_com( uint8_t *tx_buffer, uint16_t len );
static void platform_delay(uint32_t ms);
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
  /* Initialize mems driver interface */
  dev_ctx.write_reg = platform_write;
  dev_ctx.read_reg = platform_read;
  dev_ctx.handle = &SENSOR_BUS;
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
  enum neai_state error_code = neai_anomalydetection_init();	// Anomaly Detection init
  if (error_code != NEAI_OK) {								// Checking NEAI error code
	  printf("NEAI ERROR %d\r\n", error_code);
  } else {
	  printf("NEAI OK\r\n");
  }
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USB_DEVICE_Init();
  MX_SPI3_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */

  /* Set SPI3 on TX/RX STMOD+ pins */
  HAL_GPIO_WritePin(GPIOG, SEL1_2_Pin, GPIO_PIN_SET);

  /* Restore default configuration */
  ism330dhcx_reset_set(&dev_ctx, PROPERTY_ENABLE);
  /* Wait sensor boot time */
  platform_delay(BOOT_TIME);

  do {
	  /* Restore default configuration */
      ism330dhcx_reset_set(&dev_ctx, PROPERTY_ENABLE);
      /* Wait sensor boot time */
      platform_delay(BOOT_TIME);
      /* Check device ID */
      ism330dhcx_device_id_get(&dev_ctx, &whoamI);
  }
  while (whoamI != ISM330DHCX_ID);

  do {
	  /* Check sensor reset */
      ism330dhcx_reset_get(&dev_ctx, &rst);
  }
  while (rst);

  /* Start device configuration. */
  ism330dhcx_device_conf_set(&dev_ctx, PROPERTY_ENABLE);
  /* Enable Block Data Update */
  ism330dhcx_block_data_update_set(&dev_ctx, PROPERTY_ENABLE);
  /* Set Output Data Rate */
  ism330dhcx_xl_data_rate_set(&dev_ctx, ISM330DHCX_XL_ODR_833Hz);
  /* Set full scale */
  ism330dhcx_xl_full_scale_set(&dev_ctx, ISM330DHCX_2g);
  /* Configure filtering chain(No aux interface)
   *
   * Accelerometer - LPF1 + LPF2 path
   */
  //ism330dhcx_xl_hp_path_on_out_set(&dev_ctx, ISM330DHCX_LP_ODR_DIV_100);
  ism330dhcx_xl_hp_path_on_out_set(&dev_ctx, ISM330DHCX_HP_PATH_DISABLE_ON_OUT);
  //ism330dhcx_xl_filter_lp2_set(&dev_ctx, PROPERTY_ENABLE);

  /* Fill accelerometer buffer */
  HAL_UART_TxCpltCallback(&huart2);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    if (NEAI_MODE) {
		if (learn_cpt < LEARN_NB) {
			HAL_GPIO_WritePin(GPIOE, LED2_Pin, GPIO_PIN_SET);
			/* Increase learn counter */
			learn_cpt++;
			/* Call NEAI learn function */
			error_code = neai_anomalydetection_learn(input_user_buffer);
			/* Print infos on learning */
			sprintf((char *)tx_buffer, "Learn: %d/%d.\r\n", learn_cpt, LEARN_NB);
			if (COM_MODE) {
				tx_com(tx_buffer, strlen((char const *)tx_buffer));
			}
			else {
				HAL_UART_Transmit_IT(&huart2, tx_buffer, strlen((char const *)tx_buffer));
			}
		}
		else {
			/* Learning is done, first detect */
			if(learn_cpt == LEARN_NB) {
				HAL_Delay(2200);
				learn_cpt++;
			}
			//HAL_GPIO_WritePin(GPIOE, LED1_Pin, GPIO_PIN_SET);
	    	/* Fill accelerometer buffer */
	    	HAL_UART_TxCpltCallback(&huart2);
			/* Detect */
			if(nb_inferences < NEAI_NB_VERIFS) {
				error_code = neai_anomalydetection_detect(input_user_buffer, &neai_result);
				mean_neai_result += neai_result;
				nb_inferences++;
			} else {
				neai_result = mean_neai_result / NEAI_NB_VERIFS;
				nb_inferences = 0;
				mean_neai_result = 0;

				/* Print infos on detect */
				sprintf((char *)tx_buffer, "Result: %d/100.\r\n", neai_result);
				if (COM_MODE) {
					tx_com(tx_buffer, strlen((char const *)tx_buffer));
				}
				else {
					HAL_UART_Transmit_IT(&huart2, tx_buffer, strlen((char const *)tx_buffer));
				}
				if (neai_result < THRESHOLD) {
					/* Switch on orange led */
					HAL_GPIO_WritePin(GPIOE, LED1_Pin, GPIO_PIN_RESET);
					HAL_GPIO_WritePin(GPIOD, LED2_Pin, GPIO_PIN_SET);
				}
				else {
					/* Switch on green led */
					HAL_GPIO_WritePin(GPIOE, LED1_Pin, GPIO_PIN_SET);
					HAL_GPIO_WritePin(GPIOD, LED2_Pin, GPIO_PIN_RESET);
				}
				HAL_Delay(800);
			}
		}
	}
    else {
    	if (COM_MODE) {
    		/* Fill accelerometer buffer */
    		HAL_UART_TxCpltCallback(&huart2);
    		/* Formatting the whole buffer to be sent through usb_cdc */
        	for(uint32_t buf_index = 0 ; buf_index < DATA_INPUT_USER * AXIS_NUMBER ; buf_index++) {
        		snprintf(input_user_buffer_uint8 + 7*buf_index, 7, "%.4f", input_user_buffer[buf_index]);
        		*(input_user_buffer_uint8 + 7*buf_index + 6) = ' ';
        	}
        	*(input_user_buffer_uint8 + 7*(DATA_INPUT_USER * AXIS_NUMBER)) = '\n';
    		/* Print the whole buffer to the serial over usb_cdc */
    		tx_com((uint8_t *)input_user_buffer_uint8, strlen((char const *)input_user_buffer_uint8));
    	}
    	else {
    		/* Formatting the whole buffer to be sent through uart */
        	for(uint32_t buf_index = 0 ; buf_index < DATA_INPUT_USER * AXIS_NUMBER ; buf_index++) {
        		snprintf(input_user_buffer_uint8 + 7*buf_index, 7, "%.4f", input_user_buffer[buf_index]);
        		*(input_user_buffer_uint8 + 7*buf_index + 6) = ' ';
        	}
        	*(input_user_buffer_uint8 + 7*(DATA_INPUT_USER * AXIS_NUMBER)) = '\n';
        	/* Print the whole buffer to the serial */
        	HAL_UART_Transmit_IT(&huart2, (uint8_t *)input_user_buffer_uint8, sizeof(input_user_buffer_uint8));
    	}
		signals++;
    }
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
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48|RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 2;
  RCC_OscInitStruct.PLL.PLLN = 30;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief SPI3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI3_Init(void)
{

  /* USER CODE BEGIN SPI3_Init 0 */

  /* USER CODE END SPI3_Init 0 */

  /* USER CODE BEGIN SPI3_Init 1 */

  /* USER CODE END SPI3_Init 1 */
  /* SPI3 parameter configuration*/
  hspi3.Instance = SPI3;
  hspi3.Init.Mode = SPI_MODE_MASTER;
  hspi3.Init.Direction = SPI_DIRECTION_2LINES;
  hspi3.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi3.Init.CLKPolarity = SPI_POLARITY_HIGH;
  hspi3.Init.CLKPhase = SPI_PHASE_2EDGE;
  hspi3.Init.NSS = SPI_NSS_SOFT;
  hspi3.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
  hspi3.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi3.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi3.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi3.Init.CRCPolynomial = 7;
  hspi3.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi3.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
  if (HAL_SPI_Init(&hspi3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI3_Init 2 */

  /* USER CODE END SPI3_Init 2 */

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
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart2, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart2, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart2) != HAL_OK)
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

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();
  HAL_PWREx_EnableVddIO2();
  __HAL_RCC_GPIOF_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOE, LED1_Pin|DCDC_2_EN_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, LED2_Pin|WIFI_WAKEUP_Pin|EX_RESET_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(BLE_RST_GPIO_Port, BLE_RST_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(WIFI_RST_GPIO_Port, WIFI_RST_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOF, CS_WIFI_Pin|CS_ADWB_Pin|CS_DHC_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOF, C_EN_Pin|STSAFE_RESET_Pin|WIFI_BOOT0_Pin|SEL3_4_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(BLE_SPI_CS_GPIO_Port, BLE_SPI_CS_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(CS_DH_GPIO_Port, CS_DH_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, SPI2_MOSI_p2_Pin|PB11_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(SEL1_2_GPIO_Port, SEL1_2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : BOOT0_PE0_Pin BLE_TEST8_Pin */
  GPIO_InitStruct.Pin = BOOT0_PE0_Pin|BLE_TEST8_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : PB9_Pin PB8_Pin PB14_Pin CHRGB0_Pin */
  GPIO_InitStruct.Pin = PB9_Pin|PB8_Pin|PB14_Pin|CHRGB0_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : BOOT0_PE0H3_Pin */
  GPIO_InitStruct.Pin = BOOT0_PE0H3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(BOOT0_PE0H3_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : SDMMC_D3_Pin SDMMC_D2_Pin SDMMC_D1_Pin SDMMC_CK_Pin
                           SDMMC_D0_Pin */
  GPIO_InitStruct.Pin = SDMMC_D3_Pin|SDMMC_D2_Pin|SDMMC_D1_Pin|SDMMC_CK_Pin
                          |SDMMC_D0_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF12_SDMMC1;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : BLE_TEST9_Pin WIFI_DRDY_Pin INT1_DHC_Pin INT_STT_Pin
                           INT1_ADWB_Pin */
  GPIO_InitStruct.Pin = BLE_TEST9_Pin|WIFI_DRDY_Pin|INT1_DHC_Pin|INT_STT_Pin
                          |INT1_ADWB_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pin : SPI2_CLK_Pin */
  GPIO_InitStruct.Pin = SPI2_CLK_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
  HAL_GPIO_Init(SPI2_CLK_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : EX_PWM_Pin */
  GPIO_InitStruct.Pin = EX_PWM_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF1_TIM2;
  HAL_GPIO_Init(EX_PWM_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : SAI1_SCK_A_Pin SAI1_MCLK_A_Pin SAI1_FS_A_DFSDM_D3_Pin SAI1_SD_A_Pin
                           SAI1_SD_B_Pin */
  GPIO_InitStruct.Pin = SAI1_SCK_A_Pin|SAI1_MCLK_A_Pin|SAI1_FS_A_DFSDM_D3_Pin|SAI1_SD_A_Pin
                          |SAI1_SD_B_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF13_SAI1;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : LED1_Pin DCDC_2_EN_Pin */
  GPIO_InitStruct.Pin = LED1_Pin|DCDC_2_EN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : LED2_Pin WIFI_WAKEUP_Pin CS_DH_Pin EX_RESET_Pin */
  GPIO_InitStruct.Pin = LED2_Pin|WIFI_WAKEUP_Pin|CS_DH_Pin|EX_RESET_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pins : PA10_Pin PA9_Pin PA0_Pin DAC1_OUT1_Pin
                           PA1_Pin */
  GPIO_InitStruct.Pin = PA10_Pin|PA9_Pin|PA0_Pin|DAC1_OUT1_Pin
                          |PA1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : DFSDM1_DATIN5_Pin DFSDM1_D7_Pin */
  GPIO_InitStruct.Pin = DFSDM1_DATIN5_Pin|DFSDM1_D7_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF6_DFSDM1;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PG12_Pin PG10_Pin PG9_Pin */
  GPIO_InitStruct.Pin = PG12_Pin|PG10_Pin|PG9_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

  /*Configure GPIO pin : SDMMC_CMD_Pin */
  GPIO_InitStruct.Pin = SDMMC_CMD_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF12_SDMMC1;
  HAL_GPIO_Init(SDMMC_CMD_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : BLE_RST_Pin */
  GPIO_InitStruct.Pin = BLE_RST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(BLE_RST_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : WIFI_RST_Pin */
  GPIO_InitStruct.Pin = WIFI_RST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(WIFI_RST_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : I2C2_SMBA_Pin I2C2_SDA_Pin I2C2_SDAF0_Pin */
  GPIO_InitStruct.Pin = I2C2_SMBA_Pin|I2C2_SDA_Pin|I2C2_SDAF0_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF4_I2C2;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

  /*Configure GPIO pins : CS_WIFI_Pin C_EN_Pin CS_ADWB_Pin STSAFE_RESET_Pin
                           WIFI_BOOT0_Pin SEL3_4_Pin */
  GPIO_InitStruct.Pin = CS_WIFI_Pin|C_EN_Pin|CS_ADWB_Pin|STSAFE_RESET_Pin
                          |WIFI_BOOT0_Pin|SEL3_4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

  /*Configure GPIO pins : I2C3_SDA_Pin I2C3_SCL_Pin */
  GPIO_InitStruct.Pin = I2C3_SDA_Pin|I2C3_SCL_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF4_I2C3;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

  /*Configure GPIO pin : SW_SEL_Pin */
  GPIO_InitStruct.Pin = SW_SEL_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF2_TIM5;
  HAL_GPIO_Init(SW_SEL_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : INT2_DHC_Pin PGOOD_Pin INT_M_Pin */
  GPIO_InitStruct.Pin = INT2_DHC_Pin|PGOOD_Pin|INT_M_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

  /*Configure GPIO pins : SPI1_MISO_Pin SPI1_MOSI_Pin SPI1_CLK_Pin */
  GPIO_InitStruct.Pin = SPI1_MISO_Pin|SPI1_MOSI_Pin|SPI1_CLK_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

  /*Configure GPIO pins : BLE_SPI_CS_Pin SEL1_2_Pin */
  GPIO_InitStruct.Pin = BLE_SPI_CS_Pin|SEL1_2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

  /*Configure GPIO pins : INT_HTS_Pin BLE_INT_Pin */
  GPIO_InitStruct.Pin = INT_HTS_Pin|BLE_INT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

  /*Configure GPIO pins : I2C4_SCL_Pin I2C4_SDA_Pin */
  GPIO_InitStruct.Pin = I2C4_SCL_Pin|I2C4_SDA_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF4_I2C4;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pins : ADC1_IN1_Pin ADC1_IN2_Pin uC_ADC_BATT_Pin */
  GPIO_InitStruct.Pin = ADC1_IN1_Pin|ADC1_IN2_Pin|uC_ADC_BATT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG_ADC_CONTROL;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : SPI2_MISO_Pin SPI2_MOSI_Pin */
  GPIO_InitStruct.Pin = SPI2_MISO_Pin|SPI2_MOSI_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : INT2_ADWB_Pin SD_DETECT_Pin */
  GPIO_InitStruct.Pin = INT2_ADWB_Pin|SD_DETECT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : CHRG_Pin */
  GPIO_InitStruct.Pin = CHRG_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(CHRG_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : BUTTON_PWR_Pin */
  GPIO_InitStruct.Pin = BUTTON_PWR_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(BUTTON_PWR_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : USART3_RX_Pin USART3_TX_Pin */
  GPIO_InitStruct.Pin = USART3_RX_Pin|USART3_TX_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pins : USART3_RTS_Pin USART3_CTS_Pin */
  GPIO_InitStruct.Pin = USART3_RTS_Pin|USART3_CTS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : CS_DHC_Pin */
  GPIO_InitStruct.Pin = CS_DHC_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(CS_DHC_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : DFSDM1_CKOUT_Pin */
  GPIO_InitStruct.Pin = DFSDM1_CKOUT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF6_DFSDM1;
  HAL_GPIO_Init(DFSDM1_CKOUT_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : SPI2_MOSI_p2_Pin PB11_Pin */
  GPIO_InitStruct.Pin = SPI2_MOSI_p2_Pin|PB11_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : INT2_DH_Pin */
  GPIO_InitStruct.Pin = INT2_DH_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(INT2_DH_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : EX_ADC_Pin */
  GPIO_InitStruct.Pin = EX_ADC_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG_ADC_CONTROL;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(EX_ADC_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PE12_Pin */
  GPIO_InitStruct.Pin = PE12_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(PE12_GPIO_Port, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */

/* ----------- Redirecting stdout to UART2 -------------- */
int __io_putchar(int ch)
{
 uint8_t c[1];
 c[0] = ch & 0x00FF;
 HAL_UART_Transmit(&huart2, &*c, 1, 10);
 return ch;
}

/*
 * @brief  Write generic device register (platform dependent)
 *
 * @param  handle    customizable argument. In this examples is used in
 *                   order to select the correct sensor bus handler.
 * @param  reg       register to write
 * @param  bufp      pointer to data to write in register reg
 * @param  len       number of consecutive register to write
 *
 */
static int32_t platform_write(void *handle, uint8_t reg, const uint8_t *bufp,
                              uint16_t len)
{
  HAL_GPIO_WritePin(CS_DHC_GPIO_Port, CS_DHC_Pin, GPIO_PIN_RESET);
  HAL_SPI_Transmit(handle, &reg, 1, 1000);
  HAL_SPI_Transmit(handle, (uint8_t*) bufp, len, 1000);
  HAL_GPIO_WritePin(CS_DHC_GPIO_Port, CS_DHC_Pin, GPIO_PIN_SET);
  return 0;
}

/*
 * @brief  Read generic device register (platform dependent)
 *
 * @param  handle    customizable argument. In this examples is used in
 *                   order to select the correct sensor bus handler.
 * @param  reg       register to read
 * @param  bufp      pointer to buffer that store the data read
 * @param  len       number of consecutive register to read
 *
 */
static int32_t platform_read(void *handle, uint8_t reg, uint8_t *bufp,
                             uint16_t len)
{
  reg |= 0x80;
  HAL_GPIO_WritePin(CS_DHC_GPIO_Port, CS_DHC_Pin, GPIO_PIN_RESET);
  HAL_SPI_Transmit(handle, &reg, 1, 1000);
  HAL_SPI_Receive(handle, bufp, len, 1000);
  HAL_GPIO_WritePin(CS_DHC_GPIO_Port, CS_DHC_Pin, GPIO_PIN_SET);
  return 0;
}

/*
 * @brief  Send buffer to console (platform dependent)
 *
 * @param  tx_buffer     buffer to transmit
 * @param  len           number of byte to send
 *
 */
static void tx_com(uint8_t *tx_buffer, uint16_t len)
{
  CDC_Transmit_FS(tx_buffer, len);
}

/*
 * @brief  platform specific delay (platform dependent)
 *
 * @param  ms        delay in ms
 *
 */
static void platform_delay(uint32_t ms)
{
  HAL_Delay(ms);
}


void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
	if(huart == &huart2) {
		uint8_t reg;
		for (uint16_t samples = 0 ; samples < DATA_INPUT_USER; samples++) {
			do {
				// Read output only if new xl value is available
				ism330dhcx_xl_flag_data_ready_get(&dev_ctx, &reg);
			}
			while(!reg);
			// When data ready, read acceleration field data
			memset(data_raw_acceleration, 0x00, 3 * sizeof(int16_t));
			ism330dhcx_acceleration_raw_get(&dev_ctx, data_raw_acceleration);
			// Fill buffer with accelerometer x, y & z values
			input_user_buffer[AXIS_NUMBER * samples] = ism330dhcx_from_fs2g_to_mg(data_raw_acceleration[0]);
			input_user_buffer[(AXIS_NUMBER * samples) + 1] = ism330dhcx_from_fs2g_to_mg(data_raw_acceleration[1]);
			input_user_buffer[(AXIS_NUMBER * samples) + 2] = ism330dhcx_from_fs2g_to_mg(data_raw_acceleration[2]);
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

