#ifndef STM32H7xx_H
#define STM32H7xx_H



#if defined(DUAL_CORE) && !defined(CORE_CM4) && !defined(CORE_CM7)
 #error "Dual core device, please select CORE_CM4 or CORE_CM7"
#endif


#include "stm32h743xx.h"




/** @addtogroup Exported_macros
  * @{
  */
#define SET_BIT(REG, BIT)     ((REG) |= (BIT))

#define CLEAR_BIT(REG, BIT)   ((REG) &= ~(BIT))

#define READ_BIT(REG, BIT)    ((REG) & (BIT))

#define CLEAR_REG(REG)        ((REG) = (0x0))

#define WRITE_REG(REG, VAL)   ((REG) = (VAL))

#define READ_REG(REG)         ((REG))

#define MODIFY_REG(REG, CLEARMASK, SETMASK)  WRITE_REG((REG), (((READ_REG(REG)) & (~(CLEARMASK))) | (SETMASK)))

#define POSITION_VAL(VAL)     (__CLZ(__RBIT(VAL)))





#endif /* STM32H7xx_H */


