/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum {
    CMD_OPEN,                           // FIX: đổi tên enum
    CMD_PING,
    CMD_UNKNOWN,
    CMD_COUNT
} esp_command;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define STR_LEN   16

// �?ịnh nghĩa chân đi�?u khiển khóa và reed switch
#define LOCK_PIN         GPIO_PIN_12
#define LOCK_GPIO_PORT   GPIOB

#define REED_PIN         GPIO_PIN_13
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart1;
DMA_HandleTypeDef hdma_usart1_rx;
DMA_HandleTypeDef hdma_usart1_tx;

/* USER CODE BEGIN PV */
static char     rx_buf[STR_LEN + 1];
static char     tx_buf[STR_LEN + 1];

static volatile uint8_t is_busy = 0;    // FIX: tách bạch "bận/rảnh"
static volatile uint8_t is_open = 1;    // FIX: 1 = đang khóa (giả lập mặc định khoá)

const char* command_strings[CMD_COUNT] = {
    "open",
    "ping"
};
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static void strip_crlf(char *s)
{       // FIX: loại \r\n để so khớp
    for (char *p = s; *p; ++p) {
        if (*p == '\r' || *p == '\n') *p = '\0';
    }
}

static esp_command get_command_type(const char *input)
{  // FIX: nhận const char*
    for (int i = 0; i < CMD_COUNT; i++) {
        if (strcmp(input, command_strings[i]) == 0) {
            return (esp_command)i;
        }
    }
    return CMD_UNKNOWN;
}

// �?�?c reed switch: 1=đóng (nam châm gần), 0=mở
static uint8_t read_reed_switch(void)
{
    GPIO_PinState state = HAL_GPIO_ReadPin(LOCK_GPIO_PORT, REED_PIN);
    return (state == GPIO_PIN_RESET) ? 1 : 0;
}

// �?i�?u khiển khoá (PB12): open=1 -> mở, lock=1 -> khoá
static void actuate_lock(uint8_t lock, uint8_t open) // FIX: tên rõ nghĩa
{
    if (lock == 1 && open == 0)
    {
        HAL_GPIO_WritePin(LOCK_GPIO_PORT, LOCK_PIN, GPIO_PIN_RESET);
        is_open = 0; // FIX: tuỳ wiring, ở đây giả sử RESET = �?ÓNG then chốt, cửa KHÔNG mở
    }
    else if (open == 1 && lock == 0)
    {
        HAL_GPIO_WritePin(LOCK_GPIO_PORT, LOCK_PIN, GPIO_PIN_SET);
        is_open = 1; // FIX: giả sử SET = nhả chốt, cửa M�?
    }
}

static const char* handle_ping_cmd(void)
{
    return (is_busy == 0) ? "ready\n" : "busy\n";  // FIX: logic rành mạch
}

static const char* handle_open_cmd(void)
{
    // Nếu đã mở, báo success luôn
    if (is_open == 1)
    {
        return "success\n";
    }
    // Thử mở
    actuate_lock(0, 1);                           // mở
    // Giả sử thao tác tức th�?i; thực tế có thể ch�? reed/timeout
    return (is_open == 1) ? "success\n" : "fail\n";
}

/**
 *  TX xong
 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        // có thể đặt flag nếu cần
    	is_busy = 0;
    }
}

/**
 * RX theo cơ chế ReceiveToIdle DMA: biết chính xác Size
 */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)  // FIX: dùng RxEvent
{
    if (huart->Instance != USART1) return;

    // �?ảm bảo chuỗi kết thúc
    is_busy = 1;
    if (Size > STR_LEN) Size = STR_LEN;           // FIX: chặn tràn
    rx_buf[Size] = '\0';
    strip_crlf(rx_buf);

    const char* response = "unknown\n";
    esp_command cmd = get_command_type(rx_buf);

    if (cmd == CMD_PING)
    {
        response = handle_ping_cmd();
    }
    else if (cmd == CMD_OPEN)
    {
        response = handle_open_cmd();
    }

    size_t len = strlen(response);                // FIX: lấy độ dài từ chuỗi nguồn
    if (len > STR_LEN) len = STR_LEN;
    memcpy(tx_buf, response, len);

    HAL_UART_Transmit_DMA(&huart1, (uint8_t*)tx_buf, (uint16_t)len);

    // Nhận tiếp lệnh mới
    HAL_UARTEx_ReceiveToIdle_DMA(&huart1, (uint8_t*)rx_buf, STR_LEN);      // FIX: restart nhận
    __HAL_DMA_DISABLE_IT(huart1.hdmarx, DMA_IT_HT);                        // FIX: tắt HalfTransfer
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
  MX_DMA_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  // FIX: trạng thái ban đầu
    is_busy = 0;
    // Khóa khi mới khởi tạo
    actuate_lock(0,1);
    // FIX: bắt đầu nhận theo Idle (nhận liên tục, không cần đủ STR_LEN)
    HAL_UARTEx_ReceiveToIdle_DMA(&huart1, (uint8_t*)rx_buf, STR_LEN);
    __HAL_DMA_DISABLE_IT(huart1.hdmarx, DMA_IT_HT);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

	  // Có thể cập nhật LED theo reed switch mỗi 100ms
	      if (read_reed_switch())
	      {
	        HAL_GPIO_WritePin(GPIOC, REED_PIN, GPIO_PIN_RESET);
	      }
	      else
	      {
	        HAL_GPIO_WritePin(GPIOC, REED_PIN, GPIO_PIN_SET);
	      }
	      HAL_Delay(10);
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
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel4_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel4_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel4_IRQn);
  /* DMA1_Channel5_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel5_IRQn);

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
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);

  /*Configure GPIO pin : PB12 */
  GPIO_InitStruct.Pin = GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : PB13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

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
