/*
 * Async_Function.h
 *
 *  Created on: Aug 1, 2026
 *      Author: 32655
 *
 * 	本文件用于声明外部可见的函数。
 */

#ifndef ASYNC_RUNNINGTIME_INC_ASYNC_FUNCTION_H_
#define ASYNC_RUNNINGTIME_INC_ASYNC_FUNCTION_H_
#include "Async_Struct.h"

void Async_Run(void(*)(void));
void __TestAndReady(__AsyncWaitable_Typedef *Observation, size_t index);
void SubjectNotify(SubjectBase_Typedef* subject);
bool Async_SetOSTimer(size_t TimerFreq_kHz, uint16_t* TimerCnt, TimerConfigFunc TimerConfig, /*size_t DMACount, */void(*Cleanup)(void));
//重新设置监听目标前清除已占用的资源
void __ResetWaitObj(__AsyncWaitable_Typedef*const waitable);
//设置监听目标
void __SetWaitObj(__AsyncWaitable_Typedef*const waitable, SubjectBase_Typedef**const subject);
// 执行指定任务
void __RunTask(__ResumeContext_Typedef* TCB);
//启动一个任务
void Async_RunTask(void (*Task)(void));
void Async_RunTaskFast(void (*Task)(void));
void GetTimeStampBase(TimeStampBase_Type *const TSB);
TimeStamp DecodeTimeStampBase(TimeStampBase_Type *const TSB);
TimeStamp GetTimeStamp(void);

#endif /* ASYNC_RUNNINGTIME_INC_ASYNC_FUNCTION_H_ */
