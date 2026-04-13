/**
 * @file    stm32f10x_it.c
 * @brief   Exception and peripheral interrupt handlers.
 */

#include "stm32f10x_it.h"
#include "usart.h"

void NMI_Handler(void)
{
}

void HardFault_Handler(void)
{
	UsartPrintf(USART_DEBUG, "\r\n*** HardFault ***\r\n");
	while(1);
}

void MemManage_Handler(void)
{
	UsartPrintf(USART_DEBUG, "\r\n*** MemManage ***\r\n");
	while(1);
}

void BusFault_Handler(void)
{
	UsartPrintf(USART_DEBUG, "\r\n*** BusFault ***\r\n");
	while(1);
}

void UsageFault_Handler(void)
{
	UsartPrintf(USART_DEBUG, "\r\n*** UsageFault ***\r\n");
	while(1);
}

void SVC_Handler(void)
{
}

void DebugMon_Handler(void)
{
}

void PendSV_Handler(void)
{
}

void SysTick_Handler(void)
{
}

void EXTI0_IRQHandler(void)
{
}

void RTC_IRQHandler(void)
{
}
