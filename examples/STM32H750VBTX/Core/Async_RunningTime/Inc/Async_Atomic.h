/*
 * Async_Atomic.h
 *
 *  Created on: Aug 4, 2026
 *      Author: 32655
 */

#ifndef ASYNC_RUNNINGTIME_INC_ASYNC_ATOMIC_H_
#define ASYNC_RUNNINGTIME_INC_ASYNC_ATOMIC_H_
#include "Async_Struct.h"

#if defined(STM32H743xx)
  #include "stm32h743xx.h"
#elif defined(STM32H753xx)
  #include "stm32h753xx.h"
#elif defined(STM32H750xx)
  #include "stm32h750xx.h"
#elif defined(STM32H742xx)
  #include "stm32h742xx.h"
#elif defined(STM32H745xx)
  #include "stm32h745xx.h"
#elif defined(STM32H745xG)
  #include "stm32h745xg.h"
#elif defined(STM32H755xx)
  #include "stm32h755xx.h"
#elif defined(STM32H747xx)
  #include "stm32h747xx.h"
#elif defined(STM32H747xG)
  #include "stm32h747xg.h"
#elif defined(STM32H757xx)
  #include "stm32h757xx.h"
#elif defined(STM32H7B0xx)
  #include "stm32h7b0xx.h"
#elif defined(STM32H7B0xxQ)
  #include "stm32h7b0xxq.h"
#elif defined(STM32H7A3xx)
  #include "stm32h7a3xx.h"
#elif defined(STM32H7B3xx)
  #include "stm32h7b3xx.h"
#elif defined(STM32H7A3xxQ)
  #include "stm32h7a3xxq.h"
#elif defined(STM32H7B3xxQ)
  #include "stm32h7b3xxq.h"
#elif defined(STM32H735xx)
  #include "stm32h735xx.h"
#elif defined(STM32H733xx)
  #include "stm32h733xx.h"
#elif defined(STM32H730xx)
  #include "stm32h730xx.h"
#elif defined(STM32H730xxQ)
  #include "stm32h730xxq.h"
#elif defined(STM32H725xx)
  #include "stm32h725xx.h"
#elif defined(STM32H723xx)
  #include "stm32h723xx.h"
#else
// #error "Please select first the target STM32H7xx device used in your application (in stm32h7xx.h file)"
  #define HAS_HW_ATOMIC    0
#endif

#ifndef HAS_HW_ATOMIC
  #define HAS_HW_ATOMIC    1
#endif

#if HAS_HW_ATOMIC == 0
typedef struct{uintptr_t Data;} AtomicPtr;
#define LDREX_PTR(Ver, LocalVar, AtomicObjPoint) do{(void)Ver; Var = __LDREXW(&((AtomicObjPoint)->Data))}while(0)
#define STREX_PTR(Ver, LocalVar, AtomicObjPoint) ({(void)Ver;__STREXW(Var, &(AtomicObjPoint->Data))})

#else
typedef volatile struct{int32_t Version; uintptr_t Owner; uintptr_t Data[2];} AtomicPtr;
#define LDREX_PTR(Ver, LocalVar, AtomicObjPoint)\
	do{\
		AtomicPtr* temp = AtomicObjPoint;\
		int32_t __Ver;\
		__Ver=temp->Version;\
		__DMB();\
		LocalVar=temp->Data[__Ver & 0x01];\
		__DMB();\
		if(__Ver != temp->Version)\
		{__Ver = -1;}\
		Ver = __Ver;\
	}while(0)
#define STREX_PTR(Ver, LocalVar, AtomicObjPoint)\
	(__STREX_PTR(AtomicObjPoint, LocalVar, Ver) < 0)
int32_t __STREX_PTR(AtomicPtr* const AtomicPtr_Point, const uintptr_t new_Var, const int32_t Version);
//(!((Ver >= 0)&&STREX_SOFT_PTR(LocalVar, AtomicObjPoint, Ver)))

//#define AtomicObj(_Ty) struct{void* owner; int version; typeof(_Ty) data[2];}

#endif


#endif /* ASYNC_RUNNINGTIME_INC_ASYNC_ATOMIC_H_ */
