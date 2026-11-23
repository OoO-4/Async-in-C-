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
#include "tim.h"
#include "wwdg.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "Async.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
//extern DMA_HandleTypeDef hdma_tim7_up;
//extern TIM_HandleTypeDef htim7; // @suppress("Unused variable declaration in file scope")
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MPU_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void(*OS_Timer_Irq)(void);
void TimerConfig(size_t Reload, void(*OS_Irq)(void))//, size_t DMA_Count, uint8_t* DMA_Src, uint8_t* DMA_Dest)
{
	htim7.Instance->ARR = Reload;
//	HAL_DMA_Init(&hdma_tim7_up);
//	htim7.hdma[0] = &hdma_tim7_up;
//	hdma_tim7_up.XferCpltCallback = (void(*)(struct __DMA_HandleTypeDef * hdma))OS_Irq;
//	HAL_DMA_Start_IT(&hdma_tim7_up, (uint32_t)DMA_Src, (uint32_t)DMA_Dest, DMA_Count);
//	__HAL_TIM_ENABLE_DMA(&htim7, TIM_DMA_UPDATE);
//	__HAL_TIM_ENABLE(&htim7);
	OS_Timer_Irq = OS_Irq;
	HAL_TIM_Base_Start_IT(&htim7);
}
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if(htim == &htim7)
	{
		OS_Timer_Irq();
	}
}

enum btn{
    BTN_IDLE = 0,
    BTN_DOWN,              // 单击确认中（等长按或释放）
    BTN_SECOND_DOWN,       // 第二次按下确认中
    BTN_WAIT_DOUBLE,       // 等待第二次按下或释放
    BTN_LONG_HOLD,         // 长按持续
    BTN_SECOND_LONG_HOLD,  // 双击长按持续
	BTN_TO_IDLE,		   // 枚举值分割，会释放但需要跟IDLE区分的类别
	BTN_CLICK,			   // 单击确认，通知一次后恢复为IDLE
	BTN_DOUBLE_CLICK,	   // 双击双击，通知一次后恢复为IDLE
};


/*测试专区*/

bool RetrunTrue(size_t index)
{
	return 0 > index;
}

//按键事件分发，test1识别按键事件，更新时通过btnListener通知挂起了监听的任务
Subject(enum btn) btnListener = {
		.Observers.Tail = (void*)&btnListener.Observers.Head,
};
asyncFast(void, test1, ()){
	const int BTN_DEBOUNCE_MS    = 20;
	const int BTN_LONG_MS        = 600;
	const int BTN_DOUBLE_GAP_MS  = 200;
	TimeStamp time = 16;
	TimeStamp Time_ms = 0;
	bool key_status = 0;
	char gpio;
	enum btn btnStatus = BTN_IDLE;
	AsyncWaitable_NewVal(Timer, 0);
	SetWaitObj(Timer, RetrunTrue, );

	btnListener.Data = &btnStatus;
//	AsyncWaitAtFast(Timer, (1000 << 16));

	while(true)
	{
		bool isUpdata = false;
		time += 1 << 16;//10ms定时
		Time_ms += 1;
		AsyncWaitAtFast(Timer, time);
		gpio = !HAL_GPIO_ReadPin(KeyOnBoard_GPIO_Port, KeyOnBoard_Pin);
		if(btnStatus > BTN_TO_IDLE)
		{
			btnStatus = BTN_IDLE;
		}
		if(key_status != gpio)
		{
			Time_ms = 0;
			key_status = gpio;   // 保存本次电平
		}
		if (gpio)   // gpio == 1 表示按下
		{
		    switch (btnStatus)
		    {
		    case BTN_IDLE:
		        if (Time_ms > BTN_DEBOUNCE_MS)
		        {
		            btnStatus = BTN_DOWN;
		            isUpdata  = true;
		        }
		        break;
		    case BTN_DOWN:
		        if (Time_ms > BTN_LONG_MS)
		        {
		            btnStatus = BTN_LONG_HOLD;
		            isUpdata  = true;
		        }
		        break;
		    case BTN_LONG_HOLD:
		        break;
		    case BTN_WAIT_DOUBLE:
		        if (Time_ms > BTN_DEBOUNCE_MS)
		        {
		            btnStatus = BTN_SECOND_DOWN;
		            isUpdata  = true;
		        }
		        break;
		    case BTN_SECOND_DOWN:
		        if (Time_ms > BTN_LONG_MS)
		        {
		            btnStatus = BTN_SECOND_LONG_HOLD;
		            isUpdata  = true;
		        }
		        break;
		    case BTN_SECOND_LONG_HOLD:
		    	if(Time_ms > BTN_LONG_MS)
		    	{
		    		isUpdata = 1;
		    		Time_ms -= 100;
		    	}
		        break;
		    default:
//		        btnStatus = BTN_IDLE;
		        break;
		    }
		}
		else   // gpio == 0 表示松开
		{
		    switch (btnStatus)
		    {
		    case BTN_DOWN:
		    	if(Time_ms > BTN_DEBOUNCE_MS)
		    	{
					btnStatus = BTN_WAIT_DOUBLE;
					isUpdata = true;
		    	}
		        break;
		    case BTN_LONG_HOLD:
		    	if(Time_ms > BTN_DEBOUNCE_MS)
		    	{
					btnStatus = BTN_IDLE;
					isUpdata = true;
		    	}
		        break;
		    case BTN_SECOND_DOWN:
		    	if(Time_ms > BTN_DEBOUNCE_MS)
		    	{
					btnStatus = BTN_DOUBLE_CLICK;
					isUpdata  = true;
		    	}
		        break;
		    case BTN_SECOND_LONG_HOLD:
		    	if(Time_ms > BTN_DEBOUNCE_MS)
		    	{
					btnStatus = BTN_IDLE;
					isUpdata = true;
		    	}
		        break;
		    case BTN_WAIT_DOUBLE:
		        if (Time_ms > BTN_DOUBLE_GAP_MS)
		        {
		            btnStatus = BTN_CLICK;
		            isUpdata  = true;
		        }
		        break;
		    default:
		        break;
		    }
		}
		if(isUpdata)
		{
			SubjectNotify(&btnListener.Observers);
		}
	}
}
async_end

int i;
int j = 0;
async(void, test2, ()){
	AsyncWaitable_NewVal(Waitable, 1);
	enum btn btnStatus = BTN_IDLE;
	int asdfghjkl = 0;
	bool LoadBtnStatus(size_t index)
	{
		if(index == 1)
		{
			btnStatus = *(btnListener.Data);
			return true;
		}
		return false;
	}

	//内嵌函数修正
	asyncNestedFunction(LoadBtnStatus);
	//初始化等待对象
	SetWaitObj(Waitable, LoadBtnStatus, &btnListener);

	for(; asdfghjkl < 20; asdfghjkl++)
	{
		AsyncWaitForFast(Waitable, (50<<16));
		HAL_GPIO_TogglePin(LEDonBoard_GPIO_Port, LEDonBoard_Pin);
	}
#define yieldtest 0
#if yieldtest == 1
	volatile __attribute__((used)) TimeStamp cost = GetTimeStamp();
	for(i = 0; i < (1 << 25); i++){
		YieldFast();
		HAL_GPIO_TogglePin(LEDonBoard_GPIO_Port, LEDonBoard_Pin);
	}
	cost = GetTimeStamp() - cost;
	__asm__ volatile("bkpt");
#elif yieldtest == 2
	volatile __attribute__((used)) int64_t max;
	volatile __attribute__((used)) int64_t current, least;
restart:
	__asm__ volatile ("":::"memory");
	j++;
	max = 0;
	least = GetTimeStamp();
	for(i = 0; i < (1 << 25); i++){
		HAL_GPIO_TogglePin(LEDonBoard_GPIO_Port, LEDonBoard_Pin);
		YieldFast();
		current = GetTimeStamp();
		if(j < 3 && i > 10)
			goto restart;
		if(current - least > max)
		{
			max = current - least;
		}
		least = current;
	}
	__asm__ volatile("bkpt");
#endif

	while(true)
	{
		AsyncWaitFast(Waitable);
		switch(btnStatus)
		{
		case BTN_CLICK:
			HAL_GPIO_WritePin(LEDonBoard_GPIO_Port, LEDonBoard_Pin, GPIO_PIN_RESET);
			break;
		case BTN_DOUBLE_CLICK:
			HAL_GPIO_WritePin(LEDonBoard_GPIO_Port, LEDonBoard_Pin, GPIO_PIN_SET);
			break;
		case BTN_LONG_HOLD://没有连续触发
		case BTN_SECOND_LONG_HOLD:
			HAL_GPIO_TogglePin(LEDonBoard_GPIO_Port, LEDonBoard_Pin);
			break;
		default:
			break;
		}
	}
}
async_end

void test3()
{
	while(1)
	{
		Yield();
	}
}

//启动任务test1、test2后退出
void Entry(void){
	Async_RunTask(test1);
	Async_RunTask(test2);
//	Async_RunTask(test3);
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

  /* MPU Configuration--------------------------------------------------------*/
  MPU_Config();

  /* Enable the CPU Cache */

  /* Enable I-Cache---------------------------------------------------------*/
  SCB_EnableICache();

  /* Enable D-Cache---------------------------------------------------------*/
  SCB_EnableDCache();

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
  //防止以外跑飞救不回来
  HAL_Delay(2000);
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM7_Init();
  MX_WWDG1_Init();
  /* USER CODE BEGIN 2 */
  //调试冻结看门狗、系统时钟TIM7
  DBGMCU->APB3FZ1 = 64;
  DBGMCU->APB1LFZ1 = 16;
  //使能低功耗模式调试
  HAL_DBGMCU_EnableDBGSleepMode();
  HAL_DBGMCU_EnableDBGStandbyMode();
  HAL_DBGMCU_EnableDBGStopMode();

  //配置系统时钟
  Async_SetOSTimer(60000, (uint16_t*)&(htim7.Instance->CNT), TimerConfig, NULL);
  //开始调度（正常情况不退出）
  Async_Run(Entry);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
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

  /** Supply configuration update enable
  */
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  __HAL_RCC_SYSCFG_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 5;
  RCC_OscInitStruct.PLL.PLLN = 192;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_2;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

 /* MPU Configuration */

void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct = {0};

  /* Disables the MPU */
  HAL_MPU_Disable();

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.BaseAddress = 0x0;
  MPU_InitStruct.Size = MPU_REGION_SIZE_4GB;
  MPU_InitStruct.SubRegionDisable = 0x87;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /* Enables the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

}

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
