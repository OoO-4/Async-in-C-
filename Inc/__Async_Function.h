/*
 * __Async_Function.h
 *
 *  Created on: Aug 2, 2026
 *      Author: 32655
 */

#ifndef ASYNC_RUNNINGTIME_INC___ASYNC_FUNCTION_H_
#define ASYNC_RUNNINGTIME_INC___ASYNC_FUNCTION_H_
#include "__Async_Struct.h"
//发起系统调用Number(0~255字面量)
#define SVC(Number)	__asm__ volatile ("svc #" #Number)
//编译器屏障 Compiler Memory Barrier
#define __CMB()	__asm__ volatile ("":::"memory")



#endif /* ASYNC_RUNNINGTIME_INC___ASYNC_FUNCTION_H_ */
