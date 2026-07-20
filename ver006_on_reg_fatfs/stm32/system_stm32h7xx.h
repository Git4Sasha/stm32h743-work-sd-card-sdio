/**
  ******************************************************************************
  * @file    system_stm32h7xx.h
  * @author  MCD Application Team
  * @brief   CMSIS Cortex-Mx Device System Source File for STM32H7xx devices.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2017 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */

/** @addtogroup CMSIS
  * @{
  */

/** @addtogroup stm32h7xx_system
  * @{
  */

/**
  * @brief Define to prevent recursive inclusion
  */
#ifndef SYSTEM_STM32H7XX_H
#define SYSTEM_STM32H7XX_H


/** @addtogroup STM32H7xx_System_Exported_types
  * @{
  */
  /* This variable is updated in three ways:
      1) by calling CMSIS function SystemCoreClockUpdate()
      2) by calling HAL API function HAL_RCC_GetSysClockFreq()
      3) each time HAL_RCC_ClockConfig() is called to configure the system clock frequency
         Note: If you use this function to configure the system clock; then there
               is no need to call the 2 first functions listed above, since SystemCoreClock
               variable is updated automatically.
  */
extern uint32_t SystemCoreClock;             // !< System Domain1 Clock Frequency
extern uint32_t SystemD2Clock;               // !< System Domain2 Clock Frequency
extern const  uint8_t D1CorePrescTable[16];  // !< D1CorePrescTable prescalers table values
extern uint32_t APB2Freq;                    // Частота шины APB2
extern uint32_t APB1Freq;                    // Частота шины APB1
extern uint32_t SysClkD1C;                   // Частота, которая получается после деления SysClk на D1CPRE Prescaler
extern uint32_t SysClk4Bus;                  // Частота, которая получается после деления SysClkD1C на HPRE Prescaler

extern uint32_t CommonPLLSFreq;        // Частота, которая подаётся на все умножители PLL1, PLL2, PLL3
extern uint32_t PLL1InFreq;            // Входная частота для множителя PLL1 (эта частота потом будет умножаться на N1 и делиться на P1, Q1, R1)
extern uint32_t PLL2InFreq;            // Входная частота для множителя PLL2 (эта частота потом будет умножаться на N2 и делиться на P1, Q1, R1)
extern uint32_t PLL3InFreq;            // Входная частота для множителя PLL3 (эта частота потом будет умножаться на N3 и делиться на P1, Q1, R1)

extern uint32_t PLL1PFreq;             // Частота на выходе блока PLL1, которая получается после дилителя DIVP1
extern uint32_t PLL1QFreq;             // Частота на выходе блока PLL1, которая получается после дилителя DIVQ1
extern uint32_t PLL1RFreq;             // Частота на выходе блока PLL1, которая получается после дилителя DIVR1
extern uint32_t PLL2PFreq;             // Частота на выходе блока PLL2, которая получается после дилителя DIVP2
extern uint32_t PLL2QFreq;             // Частота на выходе блока PLL2, которая получается после дилителя DIVQ2
extern uint32_t PLL2RFreq;             // Частота на выходе блока PLL2, которая получается после дилителя DIVR2
extern uint32_t PLL3PFreq;             // Частота на выходе блока PLL3, которая получается после дилителя DIVP3
extern uint32_t PLL3QFreq;             // Частота на выходе блока PLL3, которая получается после дилителя DIVQ3
extern uint32_t PLL3RFreq;             // Частота на выходе блока PLL3, которая получается после дилителя DIVR3



/**
  * @}
  */

/** @addtogroup STM32H7xx_System_Exported_Constants
  * @{
  */

/**
  * @}
  */

/** @addtogroup STM32H7xx_System_Exported_Macros
  * @{
  */

/**
  * @}
  */

/** @addtogroup STM32H7xx_System_Exported_Functions
  * @{
  */

void SystemInit(void);

int32_t SetMyClockFreqency(void);       // Эта процедура формирует тактирование
int32_t PWRConfigSupply(void);          // Настройка питания

void SystemCoreClockUpdate(void);
/**
  * @}
  */


#endif /* SYSTEM_STM32H7XX_H */

/**
  * @}
  */

/**
  * @}
  */
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
