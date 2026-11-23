/*
 * __Async_Struct.h
 *
 *  Created on: Aug 1, 2026
 *      Author: 32655
 *
 *	本文件用于声明外部不可见结构。
 */

#ifndef ASYNC_RUNNINGTIME_INC___ASYNC_STRUCT_H_
#define ASYNC_RUNNINGTIME_INC___ASYNC_STRUCT_H_
#include "Async_Struct.h"

//中断栈帧格式，用于需要解析中断栈帧以跨执行流通讯的场景。
//基于AAPCS调用约定，计算前需要先&0x07对8字节对齐。
typedef union{
	struct{
		RegType R0, R1, R2, R3, R12, LR, PC, xPSR;
	}Base_Stack;
	struct{
		RegType R0, R1, R2, R3, R12, LR, PC, xPSR;
		RegType Sx[16], FPSCR, Reserved;
	}FPU_Stack;
}__Interrupt_Frame;



#endif /* ASYNC_RUNNINGTIME_INC___ASYNC_STRUCT_H_ */
