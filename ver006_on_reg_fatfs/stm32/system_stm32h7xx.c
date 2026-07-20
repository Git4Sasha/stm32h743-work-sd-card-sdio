/**
  ******************************************************************************
  * @file    system_stm32h7xx.c
  * @author  MCD Application Team
  * @brief   CMSIS Cortex-Mx Device Peripheral Access Layer System Source File.
  *
  *   This file provides two functions and one global variable to be called from
  *   user application:
  *      - SystemInit(): This function is called at startup just after reset and
  *                      before branch to main program. This call is made inside
  *                      the "startup_stm32h7xx.s" file.
  *
  *      - SystemCoreClock variable: Contains the core clock, it can be used
  *                                  by the user application to setup the SysTick
  *                                  timer or configure other parameters.
  *
  *      - SystemCoreClockUpdate(): Updates the variable SystemCoreClock and must
  *                                 be called whenever the core clock is changed
  *                                 during program execution.
  *
  *
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

/** @addtogroup STM32H7xx_System_Private_Includes
  * @{
  */

#include "stm32h7xx.h"
#include <math.h>


/************************* Miscellaneous Configuration ************************/
/*!< Uncomment the following line if you need to use initialized data in D2 domain SRAM (AHB SRAM) */
/* #define DATA_IN_D2_SRAM */

/*!< Uncomment the following line if you need to relocate your vector Table in
     Internal SRAM. */
/* #define VECT_TAB_SRAM */
#define VECT_TAB_OFFSET  0x00000000UL /*!< Vector Table base offset field.
                                      This value must be a multiple of 0x200. */
/******************************************************************************/

/**
  * @}
  */

/** @addtogroup STM32H7xx_System_Private_Macros
  * @{
  */

/**
  * @}
  */

/** @addtogroup STM32H7xx_System_Private_Variables
  * @{
  */
  /* This variable is updated in three ways:
      1) by calling CMSIS function SystemCoreClockUpdate()
      2) by calling HAL API function HAL_RCC_GetHCLKFreq()
      3) each time HAL_RCC_ClockConfig() is called to configure the system clock frequency
         Note: If you use this function to configure the system clock; then there
               is no need to call the 2 first functions listed above, since SystemCoreClock
               variable is updated automatically.
  */
#define HSE_VALUE    ((uint32_t)25000000)   //  Value of the External oscillator in Hz
#define CSI_VALUE    ((uint32_t)4000000)    //  Value of the Internal oscillator in Hz
#define HSI_VALUE    ((uint32_t)64000000)   //  Value of the Internal oscillator in Hz

  uint32_t SystemCoreClock = 64000000;
  uint32_t SystemD2Clock = 64000000;
  const  uint8_t D1CorePrescTable[16] = {0, 0, 0, 0, 1, 2, 3, 4, 1, 2, 3, 4, 6, 7, 8, 9};
  uint32_t APB2Freq = 15000000;       // Частота шины APB2
  uint32_t APB1Freq = 15000000;       // Частота шины APB1
  uint32_t SysClkD1C = 64000000;      // Частота, которая получается после деления SysClk на D1CPRE Prescaler
  uint32_t SysClk4Bus = 64000000;     // Частота, которая получается после деления SysClkD1C на HPRE Prescaler

  uint32_t CommonPLLSFreq = 0;        // Частота, которая подаётся на все умножители PLL1, PLL2, PLL3
  uint32_t PLL1InFreq = 0;            // Входная частота для множителя PLL1 (эта частота потом будет умножаться на N1 и делиться на P1, Q1, R1)
  uint32_t PLL2InFreq = 0;            // Входная частота для множителя PLL2 (эта частота потом будет умножаться на N2 и делиться на P1, Q1, R1)
  uint32_t PLL3InFreq = 0;            // Входная частота для множителя PLL3 (эта частота потом будет умножаться на N3 и делиться на P1, Q1, R1)

  uint32_t PLL1PFreq = 0;             // Частота на выходе блока PLL1, которая получается после дилителя DIVP1
  uint32_t PLL1QFreq = 0;             // Частота на выходе блока PLL1, которая получается после дилителя DIVQ1
  uint32_t PLL1RFreq = 0;             // Частота на выходе блока PLL1, которая получается после дилителя DIVR1
  uint32_t PLL2PFreq = 0;             // Частота на выходе блока PLL2, которая получается после дилителя DIVP2
  uint32_t PLL2QFreq = 0;             // Частота на выходе блока PLL2, которая получается после дилителя DIVQ2
  uint32_t PLL2RFreq = 0;             // Частота на выходе блока PLL2, которая получается после дилителя DIVR2
  uint32_t PLL3PFreq = 0;             // Частота на выходе блока PLL3, которая получается после дилителя DIVP3
  uint32_t PLL3QFreq = 0;             // Частота на выходе блока PLL3, которая получается после дилителя DIVQ3
  uint32_t PLL3RFreq = 0;             // Частота на выходе блока PLL3, которая получается после дилителя DIVR3



/**
  * @brief  Setup the microcontroller system
  *         Initialize the FPU setting and  vector table location
  *         configuration.
  * @param  None
  * @retval None
  */
void SystemInit (void)
{
#if defined (DATA_IN_D2_SRAM)
 __IO uint32_t tmpreg;
#endif /* DATA_IN_D2_SRAM */

  /* FPU settings ------------------------------------------------------------*/
  //#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
    SCB->CPACR |= ((3UL << (10*2))|(3UL << (11*2)));  /* set CP10 and CP11 Full Access */
  //#endif
  /* Reset the RCC clock configuration to the default reset state ------------*/

   /* Increasing the CPU frequency */
  if(FLASH_LATENCY_DEFAULT  > (READ_BIT((FLASH->ACR), FLASH_ACR_LATENCY)))
  {
    /* Program the new number of wait states to the LATENCY bits in the FLASH_ACR register */
	MODIFY_REG(FLASH->ACR, FLASH_ACR_LATENCY, (uint32_t)(FLASH_LATENCY_DEFAULT));
  }

  /* Set HSION bit */
  RCC->CR |= RCC_CR_HSION;

  /* Reset CFGR register */
  RCC->CFGR = 0x00000000;

  /* Reset HSEON, HSECSSON, CSION, HSI48ON, CSIKERON, PLL1ON, PLL2ON and PLL3ON bits */
  RCC->CR &= 0xEAF6ED7FU;
  
   /* Decreasing the number of wait states because of lower CPU frequency */
  if(FLASH_LATENCY_DEFAULT  < (READ_BIT((FLASH->ACR), FLASH_ACR_LATENCY)))
  {
    /* Program the new number of wait states to the LATENCY bits in the FLASH_ACR register */
	MODIFY_REG(FLASH->ACR, FLASH_ACR_LATENCY, (uint32_t)(FLASH_LATENCY_DEFAULT));
  }

#if defined(D3_SRAM_BASE)
  /* Reset D1CFGR register */
  RCC->D1CFGR = 0x00000000;

  /* Reset D2CFGR register */
  RCC->D2CFGR = 0x00000000;

  /* Reset D3CFGR register */
  RCC->D3CFGR = 0x00000000;
#else
  /* Reset CDCFGR1 register */
  RCC->CDCFGR1 = 0x00000000;

  /* Reset CDCFGR2 register */
  RCC->CDCFGR2 = 0x00000000;

  /* Reset SRDCFGR register */
  RCC->SRDCFGR = 0x00000000;
#endif
  /* Reset PLLCKSELR register */
  RCC->PLLCKSELR = 0x02020200;

  /* Reset PLLCFGR register */
  RCC->PLLCFGR = 0x01FF0000;
  /* Reset PLL1DIVR register */
  RCC->PLL1DIVR = 0x01010280;
  /* Reset PLL1FRACR register */
  RCC->PLL1FRACR = 0x00000000;

  /* Reset PLL2DIVR register */
  RCC->PLL2DIVR = 0x01010280;

  /* Reset PLL2FRACR register */

  RCC->PLL2FRACR = 0x00000000;
  /* Reset PLL3DIVR register */
  RCC->PLL3DIVR = 0x01010280;

  /* Reset PLL3FRACR register */
  RCC->PLL3FRACR = 0x00000000;

  /* Reset HSEBYP bit */
  RCC->CR &= 0xFFFBFFFFU;

  /* Disable all interrupts */
  RCC->CIER = 0x00000000;

#if (STM32H7_DEV_ID == 0x450UL)
  /* dual core CM7 or single core line */
  if((DBGMCU->IDCODE & 0xFFFF0000U) < 0x20000000U)
  {
    /* if stm32h7 revY*/
    /* Change  the switch matrix read issuing capability to 1 for the AXI SRAM target (Target 7) */
    *((__IO uint32_t*)0x51008108) = 0x000000001U;
  }
#endif

#if defined (DATA_IN_D2_SRAM)
  /* in case of initialized data in D2 SRAM (AHB SRAM) , enable the D2 SRAM clock (AHB SRAM clock) */
#if defined(RCC_AHB2ENR_D2SRAM3EN)
  RCC->AHB2ENR |= (RCC_AHB2ENR_D2SRAM1EN | RCC_AHB2ENR_D2SRAM2EN | RCC_AHB2ENR_D2SRAM3EN);
#elif defined(RCC_AHB2ENR_D2SRAM2EN)
  RCC->AHB2ENR |= (RCC_AHB2ENR_D2SRAM1EN | RCC_AHB2ENR_D2SRAM2EN);
#else
  RCC->AHB2ENR |= (RCC_AHB2ENR_AHBSRAM1EN | RCC_AHB2ENR_AHBSRAM2EN);
#endif /* RCC_AHB2ENR_D2SRAM3EN */

  tmpreg = RCC->AHB2ENR;
  (void) tmpreg;
#endif /* DATA_IN_D2_SRAM */

#if defined(DUAL_CORE) && defined(CORE_CM4)
  /* Configure the Vector Table location add offset address for cortex-M4 ------------------*/
#ifdef VECT_TAB_SRAM
  SCB->VTOR = D2_AXISRAM_BASE | VECT_TAB_OFFSET; /* Vector Table Relocation in Internal SRAM */
#else
  SCB->VTOR = FLASH_BANK2_BASE | VECT_TAB_OFFSET; /* Vector Table Relocation in Internal FLASH */
#endif /* VECT_TAB_SRAM */

#else

  /*
   * Disable the FMC bank1 (enabled after reset).
   * This, prevents CPU speculation access on this bank which blocks the use of FMC during
   * 24us. During this time the others FMC master (such as LTDC) cannot use it!
   */
  FMC_Bank1_R->BTCR[0] = 0x000030D2;

  /* Configure the Vector Table location add offset address for cortex-M7 ------------------*/
#ifdef VECT_TAB_SRAM
  SCB->VTOR = D1_AXISRAM_BASE  | VECT_TAB_OFFSET; /* Vector Table Relocation in Internal AXI-RAM */
#else
  SCB->VTOR = FLASH_BANK1_BASE | VECT_TAB_OFFSET; /* Vector Table Relocation in Internal FLASH */
#endif

#endif /*DUAL_CORE && CORE_CM4*/

}


static void PWRVoltageScalingConfig()
{
  /* Configure the Voltage Scaling 1 */
  MODIFY_REG(PWR->D3CR, PWR_D3CR_VOS, PWR_D3CR_VOS_1 | PWR_D3CR_VOS_0);  // Установка более высокого напряжения на ядре
  while(!READ_BIT(PWR->D3CR, PWR_D3CR_VOS));                             // Ожидание установки напряжения на ядре

  // Если включается режим питания ядра VOS1, то нужно включить режим Overdrive mode enabled (the LDO generates VOS0 for VCORE)
  SET_BIT(SYSCFG->PWRCR, SYSCFG_PWRCR_ODEN);

  READ_BIT(SYSCFG->PWRCR, SYSCFG_PWRCR_ODEN);                            // Вызывается просто для задержки

  while(!READ_BIT(PWR->D3CR, PWR_D3CR_VOSRDY)){}                         // Ожидание готовности
}

int32_t PWRConfigSupply(void)
{
  uint32_t i=0;

  if(!READ_BIT(PWR->CR3, PWR_CR3_SCUEN))       // Check if supply source was configured
  {
    if(!(READ_BIT(PWR->CR3, PWR_CR3_LDOEN)))   // Check supply configuration
    {
      return -1;                               // Supply configuration update locked, can't apply a new supply config
    }
    else
    {
      /* Supply configuration update locked, but new supply configuration
         matches with old supply configuration : nothing to do
      */

      PWRVoltageScalingConfig();

      return 0;
    }
  }

  /* Set the power supply configuration */
  MODIFY_REG(PWR->CR3, PWR_CR3_BYPASS | PWR_CR3_LDOEN | PWR_CR3_SCUEN, PWR_CR3_LDOEN);
  i = 0;
  while(!(READ_BIT(PWR->CSR1, PWR_CSR1_ACTVOSRDY)))
  {
    i++;if(i>100000) return -2;
  }

  PWRVoltageScalingConfig();

  return 0;
}



int32_t SetMyClockFreqency(void) // Эта процедура формирует тактирование
{
  int i=0;

  // Данная процедура настраивает тактирование от внешнего источника сигнал с которого поступает на умножитель, а умножитель в свою очередь формирует тактирование для ядра.

  MODIFY_REG(FLASH->ACR, FLASH_ACR_LATENCY, FLASH_ACR_LATENCY_4WS);
  if(READ_BIT((FLASH->ACR), FLASH_ACR_LATENCY) != FLASH_ACR_LATENCY_4WS) return -1;


  SET_BIT(RCC->CR, RCC_CR_HSEON);               // Включение внешнего източника тактирования
  while(!READ_BIT(RCC->CR, RCC_CR_HSERDY))      // Ожидание бита готовности внешнего источника тактирования
  {
    i++; if(i>1000000) return -2;               // Выход с ошибкой, если внешний источник тактирования не выдал готовность в течении заданного времени
  }

  // Настройка параметров умножителя (PLL) пред тем как его включать и ожидать его готовности, если всё пройдёт нормально, то будет выполняться переход тактирования на умножитель

  // Установка внешнего источника тактирования (HSE) для умножителя (PLL) и
  // Установка делителя, который установлен между источником тактирования и умножителем (модуль PLL)
  uint32_t divm1 = 5;
  MODIFY_REG(RCC->PLLCKSELR, RCC_PLLCKSELR_PLLSRC | RCC_PLLCKSELR_DIVM1, RCC_PLLCKSELR_PLLSRC_HSE | (divm1 << RCC_PLLCKSELR_DIVM1_Pos));

  // Установка параметров умножителя
  uint32_t divn1 = 192;
  uint32_t divp1 = 2;
  uint32_t divq1 = 2;
  uint32_t divr1 = 2;

  MODIFY_REG(RCC->PLL1DIVR, RCC_PLL1DIVR_N1_Msk, (divn1-1)<<RCC_PLL1DIVR_N1_Pos);
  MODIFY_REG(RCC->PLL1DIVR, RCC_PLL1DIVR_P1_Msk, (divp1-1)<<RCC_PLL1DIVR_P1_Pos);
  MODIFY_REG(RCC->PLL1DIVR, RCC_PLL1DIVR_Q1_Msk, (divq1-1)<<RCC_PLL1DIVR_Q1_Pos);
  MODIFY_REG(RCC->PLL1DIVR, RCC_PLL1DIVR_R1_Msk, (divr1-1)<<RCC_PLL1DIVR_R1_Pos);

  // Запрет работы с дробными множителями для умножителя частоты
  CLEAR_BIT(RCC->PLLCFGR, RCC_PLLCFGR_PLL1FRACEN);

  // Установка 0-й дробной части делителя
  MODIFY_REG(RCC->PLL1FRACR, RCC_PLL1FRACR_FRACN1, 0 << RCC_PLL1FRACR_FRACN1_Pos);

  // Установка диапазона частот на для умножителя PLL1 от 4 до 8 МГц
  MODIFY_REG(RCC->PLLCFGR, RCC_PLLCFGR_PLL1RGE, 2<<RCC_PLLCFGR_PLL1RGE_Pos);

  // Сброс бита, чтобы диапазон ГУНа для PLL1 был от 192 до 960 МГц
  CLEAR_BIT(RCC->PLLCFGR,  RCC_PLLCFGR_PLL1VCOSEL);

  SET_BIT(RCC->PLLCFGR, RCC_PLLCFGR_DIVP1EN);
  SET_BIT(RCC->PLLCFGR, RCC_PLLCFGR_DIVQ1EN);
  SET_BIT(RCC->PLLCFGR, RCC_PLLCFGR_DIVR1EN);

  // Разрешение работы с дробными множителями для умножителя частоты
  SET_BIT(RCC->PLLCFGR, RCC_PLLCFGR_PLL1FRACEN);



  SET_BIT(RCC->CR, RCC_CR_PLL1ON);              // Включение умножителя
  while(!(READ_BIT(RCC->CR, RCC_CR_PLL1RDY)))   // Ожидание бита готовности источника тактирования через умножитель
  {
    i++; if(i>1000000) return -3;               // Выход с ошибкой, если модуль умножения для тактирования не выдал готовность в течении заданного времени
  }


  // Установка делителя для HPRE (AHB3 Clock)
  MODIFY_REG(RCC->D1CFGR, RCC_D1CFGR_HPRE, RCC_D1CFGR_HPRE_DIV2);

  // Установка делителя для D1PPRE (APB3 clock)
  MODIFY_REG(RCC->D1CFGR, RCC_D1CFGR_D1PPRE, RCC_D1CFGR_D1PPRE_DIV2);

  // Установка делителя для D2PPRE1 (APB1 clock)
  MODIFY_REG(RCC->D2CFGR, RCC_D2CFGR_D2PPRE1, RCC_D2CFGR_D2PPRE1_DIV2);

  // Установка делителя для D2PPRE2 (APB2 clock)
  MODIFY_REG(RCC->D2CFGR, RCC_D2CFGR_D2PPRE2, RCC_D2CFGR_D2PPRE2_DIV2);

  // Установка делителя для D3PPRE (APB4 clock)
  MODIFY_REG(RCC->D3CFGR, RCC_D3CFGR_D3PPRE, RCC_D3CFGR_D3PPRE_DIV2);


  MODIFY_REG(RCC->CFGR, RCC_CFGR_SW, RCC_CFGR_SW_PLL1);  // Выбор PLL1 в качестве источника для системной частоты
  while(READ_BIT(RCC->CFGR, RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL1)
  {
    i++; if(i>1000000) return -4;                         // Ожидание готовности перехода на источник тактирования от умножителя чатоты (PLL)
  }

  return 0;
}


static uint32_t GetSysClkD1C(void) // Из этой частоты формируются частоты для ядра, шин AHB, APB, AXI
{
  uint32_t tmp;

  tmp = RCC->D1CFGR & RCC_D1CFGR_D1CPRE;    // Выделение битов отвечающих за делитель DC1PRE Prescaler (смотри схему тактирования)
  if(tmp != RCC_D1CFGR_D1CPRE_DIV1)
  {
    switch(tmp)
    {
      case RCC_D1CFGR_D1CPRE_DIV2  : return(SystemCoreClock / 2  );
      case RCC_D1CFGR_D1CPRE_DIV4     : return(SystemCoreClock / 4  );
      case RCC_D1CFGR_D1CPRE_DIV8     : return(SystemCoreClock / 8  );
      case RCC_D1CFGR_D1CPRE_DIV16    : return(SystemCoreClock / 16 );
      case RCC_D1CFGR_D1CPRE_DIV64    : return(SystemCoreClock / 64 );
      case RCC_D1CFGR_D1CPRE_DIV128   : return(SystemCoreClock / 128);
      case RCC_D1CFGR_D1CPRE_DIV256   : return(SystemCoreClock / 256);
      case RCC_D1CFGR_D1CPRE_DIV512   : return(SystemCoreClock / 512);
    }
  }
  return SystemCoreClock;
}

static uint32_t GetSysClk4Bus(void)  // Частота, которая получается после деления SysClkD1C на HPRE Prescaler
{
  // Эта функция должна запускаться после GetSysClkD1C, т.к. она изпользует значение SysClkD1C

  uint32_t tmp;

  tmp = RCC->D1CFGR & RCC_D1CFGR_HPRE;    // Выделение битов отвечающих за делитель HPRE Prescaler (смотри схему тактирования)
  if(tmp != RCC_D1CFGR_HPRE_DIV1)
  {
    switch(tmp)
    {
      case RCC_D1CFGR_HPRE_DIV2     : return (SysClkD1C / 2  );
      case RCC_D1CFGR_HPRE_DIV4     : return (SysClkD1C / 4  );
      case RCC_D1CFGR_HPRE_DIV8     : return (SysClkD1C / 8  );
      case RCC_D1CFGR_HPRE_DIV16    : return (SysClkD1C / 16 );
      case RCC_D1CFGR_HPRE_DIV64    : return (SysClkD1C / 64 );
      case RCC_D1CFGR_HPRE_DIV128   : return (SysClkD1C / 128);
      case RCC_D1CFGR_HPRE_DIV256   : return (SysClkD1C / 256);
      case RCC_D1CFGR_HPRE_DIV512   : return (SysClkD1C / 512);
    }
  }
  return SysClkD1C;
}

static uint32_t GetAPB1Freq(void) // Получение значения частоты шини APB1
{
  // Эту функцию нужно запуска, после того, как будут определены значения для SysClkD1C и SysClk4Bus

  uint32_t tmp;

  tmp = RCC->D2CFGR & RCC_D2CFGR_D2PPRE1;
  if(tmp != RCC_D2CFGR_D2PPRE1_DIV1)
  {
    switch(tmp)
    {
      case RCC_D2CFGR_D2PPRE1_DIV2   : return ( SysClk4Bus / 2 );
      case RCC_D2CFGR_D2PPRE1_DIV4   : return ( SysClk4Bus / 4 );
      case RCC_D2CFGR_D2PPRE1_DIV8   : return ( SysClk4Bus / 8 );
      case RCC_D2CFGR_D2PPRE1_DIV16  : return ( SysClk4Bus / 16 );
    }
  }
  return SysClk4Bus;
}

static uint32_t GetAPB2Freq(void) // Получение значения частоты шини APB1
{
  // Эту функцию нужно запуска, после того, как будут определены значения для SysClkD1C и SysClk4Bus

  uint32_t tmp;

  tmp = RCC->D2CFGR & RCC_D2CFGR_D2PPRE2;
  if(tmp != RCC_D2CFGR_D2PPRE2_DIV1)
  {
    switch(tmp)
    {
      case RCC_D2CFGR_D2PPRE2_DIV2   : return ( SysClk4Bus / 2 );
      case RCC_D2CFGR_D2PPRE2_DIV4   : return ( SysClk4Bus / 4 );
      case RCC_D2CFGR_D2PPRE2_DIV8   : return ( SysClk4Bus / 8 );
      case RCC_D2CFGR_D2PPRE2_DIV16  : return ( SysClk4Bus / 16 );
    }
  }
  return SysClk4Bus;
}

static void GetCommonPLLSFreq()      // Процедура заполняет значение переменной CommonPLLSFreq
{
  uint32_t pllsource;
  uint32_t pllm1, pllm2, pllm3;

  pllsource = (RCC->PLLCKSELR & RCC_PLLCKSELR_PLLSRC);                                      // Источник тактирования для всех PLL (HSI, CSI, HSE)

  pllm1 = READ_BIT(RCC->PLLCKSELR, RCC_PLLCKSELR_DIVM1_Msk) >> RCC_PLLCKSELR_DIVM1_Pos;    // Делитель источника тактирования PLL1 (DIVM1)
  pllm2 = READ_BIT(RCC->PLLCKSELR, RCC_PLLCKSELR_DIVM2_Msk) >> RCC_PLLCKSELR_DIVM2_Pos;    // Делитель источника тактирования PLL2 (DIVM1)
  pllm3 = READ_BIT(RCC->PLLCKSELR, RCC_PLLCKSELR_DIVM3_Msk) >> RCC_PLLCKSELR_DIVM3_Pos;    // Делитель источника тактирования PLL3 (DIVM1)

  switch (pllsource)   // Различные варианты в зависимости от источника тактирования PLL1, PLL2, PLL3
  {
    case RCC_PLLCKSELR_PLLSRC_HSI:  /* HSI used as PLL clock source */
      CommonPLLSFreq = (HSI_VALUE >> ((RCC->CR & RCC_CR_HSIDIV)>> 3)) ;
      break;

    case RCC_PLLCKSELR_PLLSRC_CSI:  /* CSI used as PLL clock source */
      CommonPLLSFreq = CSI_VALUE;
      break;

    case RCC_PLLCKSELR_PLLSRC_HSE:  /* HSE used as PLL clock source */
      CommonPLLSFreq = HSE_VALUE;
      break;

    default:
      CommonPLLSFreq = 0;
      break;
  }

  if(pllm1) PLL1InFreq = CommonPLLSFreq / pllm1; else PLL1InFreq = CommonPLLSFreq;
  if(pllm2) PLL2InFreq = CommonPLLSFreq / pllm2; else PLL2InFreq = CommonPLLSFreq;
  if(pllm3) PLL3InFreq = CommonPLLSFreq / pllm3; else PLL3InFreq = CommonPLLSFreq;
}

static void GetPLLSFreqs(void)           // Процедура заполняет значения для переменных PLL1PFreq, PLL1QFreq, PLL1RFreq и те же частоты для PLL2 и PLL3
{
  uint32_t pllp, pllq, pllr, pllfracen, pllmuln;
  float_t fracn, pllvco;

  // Всё для PLL1
  pllp = (READ_BIT(RCC->PLL1DIVR, RCC_PLL1DIVR_P1_Msk) >> RCC_PLL1DIVR_P1_Pos) + 1U;               // Значение делителя divp1
  pllq = (READ_BIT(RCC->PLL1DIVR, RCC_PLL1DIVR_Q1_Msk) >> RCC_PLL1DIVR_Q1_Pos) + 1U;               // Значение делителя divq1
  pllr = (READ_BIT(RCC->PLL1DIVR, RCC_PLL1DIVR_R1_Msk) >> RCC_PLL1DIVR_R1_Pos) + 1U;               // Значение делителя divr1

  pllfracen = READ_BIT(RCC->PLLCFGR, RCC_PLLCFGR_PLL1FRACEN_Msk) >> RCC_PLLCFGR_PLL1FRACEN_Pos;     // Задействован ли дробный множитель
  fracn = (float_t)(uint32_t)(pllfracen * (READ_BIT(RCC->PLL1FRACR, RCC_PLL1FRACR_FRACN1_Msk) >> RCC_PLL1FRACR_FRACN1_Pos));   // Значение дробной части множителся (DIVN1 - называется "DIV" хотя на самом деле это множитель )
  pllmuln = READ_BIT(RCC->PLL1DIVR, RCC_PLL1DIVR_N1_Msk) >> RCC_PLL1DIVR_N1_Pos;

  pllvco = (float_t)PLL1InFreq * ((float_t)pllmuln + (fracn/(float_t)0x2000) +(float_t)1 );

  PLL1PFreq = (uint32_t)(float_t)(pllvco/(float_t)pllp);    // Частота на выходе PLL1DIVP1
  PLL1QFreq = (uint32_t)(float_t)(pllvco/(float_t)pllq);    // Частота на выходе PLL1DIVQ1
  PLL1RFreq = (uint32_t)(float_t)(pllvco/(float_t)pllr);    // Частота на выходе PLL1DIVR1


  // Всё для PLL2
  pllp = (READ_BIT(RCC->PLL2DIVR, RCC_PLL2DIVR_P2_Msk) >> RCC_PLL2DIVR_P2_Pos) + 1U;               // Значение делителя divp2
  pllq = (READ_BIT(RCC->PLL2DIVR, RCC_PLL2DIVR_Q2_Msk) >> RCC_PLL2DIVR_Q2_Pos) + 1U;               // Значение делителя divq2
  pllr = (READ_BIT(RCC->PLL2DIVR, RCC_PLL2DIVR_R2_Msk) >> RCC_PLL2DIVR_R2_Pos) + 1U;               // Значение делителя divr2

  pllfracen = READ_BIT(RCC->PLLCFGR, RCC_PLLCFGR_PLL2FRACEN_Msk) >> RCC_PLLCFGR_PLL2FRACEN_Pos;    // Задействован ли дробный множитель
  fracn = (float_t)(uint32_t)(pllfracen * (READ_BIT(RCC->PLL2FRACR, RCC_PLL2FRACR_FRACN2_Msk) >> RCC_PLL2FRACR_FRACN2_Pos));   // Значение дробной части множителся (DIVN1 - называется "DIV" хотя на самом деле это множитель )
  pllmuln = READ_BIT(RCC->PLL2DIVR, RCC_PLL2DIVR_N2_Msk) >> RCC_PLL2DIVR_N2_Pos;

  pllvco = (float_t)PLL2InFreq * ((float_t)pllmuln + (fracn/(float_t)0x2000) +(float_t)1 );

  PLL2PFreq = (uint32_t)(float_t)(pllvco/(float_t)pllp);    // Частота на выходе PLL2DIVP2
  PLL2QFreq = (uint32_t)(float_t)(pllvco/(float_t)pllq);    // Частота на выходе PLL2DIVQ2
  PLL2RFreq = (uint32_t)(float_t)(pllvco/(float_t)pllr);    // Частота на выходе PLL2DIVR2


  // Всё для PLL3
  pllp = (READ_BIT(RCC->PLL3DIVR, RCC_PLL3DIVR_P3_Msk) >> RCC_PLL3DIVR_P3_Pos) + 1U;               // Значение делителя divp3
  pllq = (READ_BIT(RCC->PLL3DIVR, RCC_PLL3DIVR_Q3_Msk) >> RCC_PLL3DIVR_Q3_Pos) + 1U;               // Значение делителя divq3
  pllr = (READ_BIT(RCC->PLL3DIVR, RCC_PLL3DIVR_R3_Msk) >> RCC_PLL3DIVR_R3_Pos) + 1U;               // Значение делителя divr3

  pllfracen = ((RCC->PLLCFGR & RCC_PLLCFGR_PLL3FRACEN)>>RCC_PLLCFGR_PLL3FRACEN_Pos);        // Задействован ли дробный множитель
  fracn = (float_t)(uint32_t)(pllfracen * (READ_BIT(RCC->PLL3FRACR, RCC_PLL3FRACR_FRACN3_Msk) >> RCC_PLL3FRACR_FRACN3_Pos));   // Значение дробной части множителся (DIVN1 - называется "DIV" хотя на самом деле это множитель )
  pllmuln = READ_BIT(RCC->PLL3DIVR, RCC_PLL3DIVR_N3_Msk) >> RCC_PLL3DIVR_N3_Pos;

  pllvco = (float_t)PLL3InFreq * ((float_t)pllmuln + (fracn/(float_t)0x2000) +(float_t)1 );

  PLL3PFreq = (uint32_t)(float_t)(pllvco/(float_t)pllp);    // Частота на выходе PLL3DIVP3
  PLL3QFreq = (uint32_t)(float_t)(pllvco/(float_t)pllq);    // Частота на выходе PLL3DIVQ3
  PLL3RFreq = (uint32_t)(float_t)(pllvco/(float_t)pllr);    // Частота на выходе PLL3DIVR3
}


void SystemCoreClockUpdate (void)
{
  uint32_t tmp;
  uint32_t common_system_clock;


  // Необходимо получить частоту тактового сигнала, который используется для всех модулей множителей (для всех PLL, коих 3 штуки)
  GetCommonPLLSFreq();      // Процедура заполняет значение переменной CommonPLLSFreq
  GetPLLSFreqs();           // Процедура заполняет значения для переменных PLL1PFreq, PLL1QFreq, PLL1RFreq и те же частоты для PLL2 и PLL3


  /* Get SYSCLK source -------------------------------------------------------*/

  switch (RCC->CFGR & RCC_CFGR_SWS)   // Определение частоты SYSClk от неё тактируется ядро, и шины микроконтроллера
  {
    case RCC_CFGR_SWS_HSI:  /* HSI used as system clock source */
      common_system_clock = (uint32_t) (HSI_VALUE >> ((RCC->CR & RCC_CR_HSIDIV)>> 3));
      break;

    case RCC_CFGR_SWS_CSI:  /* CSI used as system clock  source */
      common_system_clock = CSI_VALUE;
      break;

    case RCC_CFGR_SWS_HSE:  /* HSE used as system clock  source */
      common_system_clock = HSE_VALUE;
      break;

    case RCC_CFGR_SWS_PLL1:  // SysClk тактируется от частотного множителя PLL

      // PLL_VCO = (HSE_VALUE or HSI_VALUE or CSI_VALUE/ PLLM) * PLLN
      // SYSCLK = PLL_VCO / PLLR
      common_system_clock =  PLL1PFreq;   // Частота на выходе PLL1DIVP1
      break;

    default:
      common_system_clock = CSI_VALUE;
      break;
  }

  /* Compute SystemClock frequency --------------------------------------------------*/
#if defined (RCC_D1CFGR_D1CPRE)
  tmp = D1CorePrescTable[(RCC->D1CFGR & RCC_D1CFGR_D1CPRE)>> RCC_D1CFGR_D1CPRE_Pos];

  /* common_system_clock frequency : CM7 CPU frequency  */
  common_system_clock >>= tmp;

  /* SystemD2Clock frequency : CM4 CPU, AXI and AHBs Clock frequency  */
  SystemD2Clock = (common_system_clock >> ((D1CorePrescTable[(RCC->D1CFGR & RCC_D1CFGR_HPRE)>> RCC_D1CFGR_HPRE_Pos]) & 0x1FU));

#else
  tmp = D1CorePrescTable[(RCC->CDCFGR1 & RCC_CDCFGR1_CDCPRE)>> RCC_CDCFGR1_CDCPRE_Pos];

  /* common_system_clock frequency : CM7 CPU frequency  */
  common_system_clock >>= tmp;

  /* SystemD2Clock frequency : AXI and AHBs Clock frequency  */
  SystemD2Clock = (common_system_clock >> ((D1CorePrescTable[(RCC->CDCFGR1 & RCC_CDCFGR1_HPRE)>> RCC_CDCFGR1_HPRE_Pos]) & 0x1FU));

#endif

#if defined(DUAL_CORE) && defined(CORE_CM4)
  SystemCoreClock = SystemD2Clock;
#else
  SystemCoreClock = common_system_clock;
#endif /* DUAL_CORE && CORE_CM4 */

  SysClkD1C  =   GetSysClkD1C();
  SysClk4Bus =   GetSysClk4Bus();
  APB1Freq   =   GetAPB1Freq();
  APB2Freq   =   GetAPB2Freq();
}


/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
