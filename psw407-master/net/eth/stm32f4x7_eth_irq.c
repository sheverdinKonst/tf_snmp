#include "stm32f4xx.h"
#include "stm32f4x7_eth.h"
#include "stm32f4x7_eth_bsp.h"

/* Scheduler includes */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_exti.h"
/* lwip includes */
//#include "sys.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
extern xSemaphoreHandle s_xSemaphore;

/**
  * @brief  This function handles External line 10 interrupt request.
  * @param  None
  * @retval None
  */
/*
void EXTI0_IRQHandler(void)
{

  if(EXTI_GetITStatus(ETH_LINK_EXTI_LINE) != RESET)
  {
    Eth_Link_ITHandler(KSZ8051_PHY_ADDRESS);
    
    EXTI_ClearITPendingBit(ETH_LINK_EXTI_LINE);
  }
}
*/
/**
  * @brief  This function handles ethernet DMA interrupt request.
  * @param  None
  * @retval None
  */
void ETH_IRQHandler(void)
{
  //GPIOD->ODR ^= GPIO_Pin_13;
  portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

  /* Frame received */
  if ( ETH_GetDMAFlagStatus(ETH_DMA_FLAG_R) == SET) 
  {
    /* Give the semaphore to wakeup LwIP task */
	  if(s_xSemaphore!=NULL){
		  xSemaphoreGiveFromISR( s_xSemaphore, &xHigherPriorityTaskWoken );

	  }
  }
  
  /* Clear the interrupt flags. */
  /* Clear the Eth DMA Rx IT pending bits */
  ETH_DMAClearITPendingBit(ETH_DMA_IT_R);
  ETH_DMAClearITPendingBit(ETH_DMA_IT_NIS);
  
  /* Switch tasks if necessary. */  
  if( xHigherPriorityTaskWoken != pdFALSE )
  {
    portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
  }
}
