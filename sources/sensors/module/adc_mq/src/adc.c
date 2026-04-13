
#include "stm32f10x.h"
#include "adc.h"

__IO uint16_t ADC_ConvertedValue[NOFCHANEL] = {0};

/**
  * @brief  ADC GPIO 初始化
  *
  * Only PA0, PA1, PA4, PA5, PA6 are used.
  * PA2/PA3 are USART2 TX/RX (ESP8266) and must not be touched here.
  */
static void ADCx_GPIO_Config(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	ADC_GPIO_APBxClock_FUN(ADC_GPIO_CLK, ENABLE);

	GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_0 | GPIO_Pin_1 |
	                               GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(ADC_PORT, &GPIO_InitStructure);
}

/**
  * @brief  配置ADC + DMA，循环扫描5个通道
  *
  * DMA buffer layout (ADC_ConvertedValue[]):
  *   [ADC_IDX_MQ3]   = PA0  MQ3  Alcohol
  *   [ADC_IDX_MQ4]   = PA1  MQ4  Methane
  *   [ADC_IDX_LIGHT] = PA4  Light sensor
  *   [ADC_IDX_MQ2]   = PA5  MQ2  Smoke/CO
  *   [ADC_IDX_MQ7]   = PA6  MQ7  Carbon Monoxide
  *
  * PA2/PA3 (MQ5/MQ6) are NOT sampled — they belong to USART2.
  */
static void ADCx_Mode_Config(void)
{
	DMA_InitTypeDef DMA_InitStructure;
	ADC_InitTypeDef ADC_InitStructure;

	RCC_AHBPeriphClockCmd(ADC_DMA_CLK, ENABLE);
	ADC_APBxClock_FUN(ADC_CLK, ENABLE);

	DMA_DeInit(ADC_DMA_CHANNEL);

	DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)(&(ADC_x->DR));
	DMA_InitStructure.DMA_MemoryBaseAddr     = (u32)ADC_ConvertedValue;
	DMA_InitStructure.DMA_DIR                = DMA_DIR_PeripheralSRC;
	DMA_InitStructure.DMA_BufferSize         = NOFCHANEL;
	DMA_InitStructure.DMA_PeripheralInc      = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc          = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
	DMA_InitStructure.DMA_MemoryDataSize     = DMA_MemoryDataSize_HalfWord;
	DMA_InitStructure.DMA_Mode               = DMA_Mode_Circular;
	DMA_InitStructure.DMA_Priority           = DMA_Priority_High;
	DMA_InitStructure.DMA_M2M                = DMA_M2M_Disable;
	DMA_Init(ADC_DMA_CHANNEL, &DMA_InitStructure);
	DMA_Cmd(ADC_DMA_CHANNEL, ENABLE);

	ADC_InitStructure.ADC_Mode               = ADC_Mode_Independent;
	ADC_InitStructure.ADC_ScanConvMode       = ENABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
	ADC_InitStructure.ADC_ExternalTrigConv   = ADC_ExternalTrigConv_None;
	ADC_InitStructure.ADC_DataAlign          = ADC_DataAlign_Right;
	ADC_InitStructure.ADC_NbrOfChannel       = NOFCHANEL;

	RCC_ADCCLKConfig(RCC_PCLK2_Div8);  /* ADC clock = 72MHz/8 = 9MHz — must be set before ADC_Init */
	ADC_Init(ADC_x, &ADC_InitStructure);

	/* rank = ADC_IDX + 1, ensuring DMA index matches buffer definition */
	ADC_RegularChannelConfig(ADC_x, ADC_Channel_0, ADC_IDX_MQ3   + 1, ADC_SampleTime_55Cycles5); /* PA0 MQ3  */
	ADC_RegularChannelConfig(ADC_x, ADC_Channel_1, ADC_IDX_MQ4   + 1, ADC_SampleTime_55Cycles5); /* PA1 MQ4  */
	ADC_RegularChannelConfig(ADC_x, ADC_Channel_4, ADC_IDX_LIGHT + 1, ADC_SampleTime_55Cycles5); /* PA4 光照 */
	ADC_RegularChannelConfig(ADC_x, ADC_Channel_5, ADC_IDX_MQ2   + 1, ADC_SampleTime_55Cycles5); /* PA5 MQ2  */
	ADC_RegularChannelConfig(ADC_x, ADC_Channel_6, ADC_IDX_MQ7   + 1, ADC_SampleTime_55Cycles5); /* PA6 MQ7  */

	ADC_DMACmd(ADC_x, ENABLE);
	ADC_Cmd(ADC_x, ENABLE);

	ADC_ResetCalibration(ADC_x);
	while(ADC_GetResetCalibrationStatus(ADC_x));
	ADC_StartCalibration(ADC_x);
	while(ADC_GetCalibrationStatus(ADC_x));

	ADC_SoftwareStartConvCmd(ADC_x, ENABLE);
}

/**
  * @brief  ADC初始化入口
  */
void ADCx_Init(void)
{
	ADCx_GPIO_Config();
	ADCx_Mode_Config();
}
