/*
 * Async_Staticinline.h
 *
 *  Created on: Aug 1, 2026
 *      Author: 32655
 *
 *	本文件用于定义需要强制内联到应用任务的函数。
 */

#ifndef ASYNC_RUNNINGTIME_INC_ASYNC_STATICINLINE_H_
#define ASYNC_RUNNINGTIME_INC_ASYNC_STATICINLINE_H_
#include <stdlib.h>
//#include "Async_Atomic.h"
#include "Async_Struct.h"

//失效全部寄存器、内存
#define InvalidateRegisters() __asm__ volatile("":::"r0","r1","r2","r3","r4","r5","r6","r8","r9","r10","r11","r12","r14","r15","d0","d1","d2","d3","d4","d5","d6","d7","d8","d9","d10","d11","d12","d13","d14","d15","memory")
//可变参数数量计算宏
#define __VA_ARGS_Count(...) __VA_ARGS_Count__(__VA_ARGS__ __VA_OPT__(,) 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
//辅助计算参数数量宏
#define __VA_ARGS_Count__(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, Num, ...) (Num)
//获取FP指针
#define __GetFP()	({RegType temp;__asm__ volatile("mov %0, "__reg_fp"\n": "=r"(temp));temp;})
//__attribute__((noinline, naked))
//static RegType __GetFP()
//{
//	__asm__ volatile(
//			"mov r0, "__reg_fp"\n"
//			"bx lr\n"
//			);
//}
//获取SP指针
#define __GetSP()	({RegType temp;__asm__ volatile("mov %0, sp\n": "=r"(temp));temp;})
//设置FP指针
#define __SetFP(FP)	__asm__ volatile("mov "__reg_fp", %0\n"::"r"(FP))
//设置SP指针
#define __SetSP(SP) __asm__ volatile("mov sp, %0\n"::"r"(SP))
//设置PC指针(跳转)
#define __SetPC(PC) ({InvalidateRegisters();__asm__ volatile("mov pc, %0\n"::"r"(PC));})

//async函数作用域
#define async(Ret, FunctionName, Params) \
Ret FunctionName Params{\
	extern void __Async_Entry(__Async_Typedef*);\
	extern void __Async_Exit(__Async_Typedef*);\
	__Async_Typedef __attribute__((cleanup(__Async_Exit),unused)) __async__;\
	__Async_Entry(&__async__);\
	if(!__async__.flags.IsRun){\
		__builtin_choose_expr( \
			__builtin_types_compatible_p(Ret, void), \
			({return;}), \
			({return (Ret)0;})\
		);\
	}
//栈上协程，应该严格满足LIFO
#define asyncFast(Ret, FunctionName, Params) \
Ret FunctionName Params{\
	extern void __Async_Entry2(__Async_Typedef*);\
	extern void __Async_Exit2(__Async_Typedef*);\
	__Async_Typedef __attribute__((cleanup(__Async_Exit2),unused)) __async__;\
	__Async_Entry2(&__async__);\
	if(!__async__.flags.IsRun){\
		__builtin_choose_expr( \
			__builtin_types_compatible_p(Ret, void), \
			({return;}), \
			({return (Ret)0;})\
		);\
	}
#define async_end }

//async内嵌函数跳板修正
#if (defined(__CORTEX_M7) || defined(__CORTEX_M55)) || (defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U) && defined(__ICACHE_PRESENT) && (__ICACHE_PRESENT == 1U))
#define asyncNestedFunction(Function) do{((uint32_t*)((intptr_t)Function & ~1))[2] += __async__.hp - __async__.sp;\
		SCB_CleanDCache_by_Addr((uint32_t *)((intptr_t)Function & ~1), 16);\
		SCB_InvalidateICache_by_Addr((uint32_t *)((intptr_t)Function & ~1), 16);\
	}while(0)
#elif defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U)
#define asyncNestedFunction(Function) do{((uint32_t*)((intptr_t)Function & ~1))[2] += __async__.hp - __async__.sp;\
	SCB_CleanDCache_by_Addr((uint32_t *)((intptr_t)Function & ~1), 16);\
	}while(0)
#elif defined(__ICACHE_PRESENT) && (__ICACHE_PRESENT == 1U)
#define asyncNestedFunction(Function) do{((uint32_t*)((intptr_t)Function & ~1))[2] += __async__.hp - __async__.sp;\
	SCB_InvalidateICache_by_Addr((uint32_t *)((intptr_t)Function & ~1), 16);\
	}while(0)
#else
	#define asyncNestedFunction(Function) do{((uint32_t*)((intptr_t)Function & ~1))[2] += __async__.hp - __async__.sp;}while(0)
#endif

//yield核心功能，切换下一任务。在async函数中可以安全地直接使用
__attribute__((always_inline, used))
static inline void YieldFast(void)
{
	OS_Kernel.Private.RunningTask->FramePoint = __GetFP();
	if(!(OS_Kernel.Private.RunningTask->FramePoint))goto yield_point;//产生goto label，避免label错误
	OS_Kernel.Private.RunningTask->ProgramCounter = 1 + &&yield_point;
	__RunTask(OS_Kernel.Private.RunningTask->Next);
yield_point:
	InvalidateRegisters();
}
//带栈保存功能的Yield,速度较慢。可以在任意函数中使用
__attribute__((always_inline, used))
static inline async(void, Yield, (void))
	__async__.flags.IsFree = __async__.flags.IsAlloc;

	YieldFast();
}
//不带栈保存的WaitFor
__attribute__((always_inline, used))
static inline void AsyncWaitFast(__AsyncWaitable_Typedef* WaitableObj)
{
	__ResumeContext_Typedef* this = OS_Kernel.Private.RunningTask;
	__ResumeContext_Typedef* next = this->Next;
	//阻塞当前任务
	this->Pre->Next = this->Next;
	this->Next->Pre = this->Pre;
//	this->Next = NULL;
//	this->Pre = NULL;
	this->FramePoint = __GetFP();
	if(!(this->FramePoint))goto YieldPoint;//产生goto label，避免label错误
	this->ProgramCounter = 1 + &&YieldPoint;
	//生效唤起
	WaitableObj[0].HeadLine.ResumePoint.Context = this;
	//延时执行回调
	__TestAndReady(WaitableObj, 0);
	//继续下一任务
	__RunTask(next);
	YieldPoint:
	InvalidateRegisters();
	return;
}
//等待到指定时刻
__attribute__((always_inline, used))
static inline void AsyncWaitAtFast(__AsyncWaitable_Typedef* WaitableObj, TimeStamp AbsoluteTime)
{
	void __TimeoutSort(void);
	__WaitableWithTimeout Timeout = {
			.Context = WaitableObj,
			.Timeout = AbsoluteTime,
	};
	//比无序队列最小值小，头插到无序队列
	if(OS_Kernel.Private.TimerUnordered.Head == NULL)
	{
		Timeout.Pre = (__WaitableWithTimeout*)&OS_Kernel.Private.TimerUnordered.Head;
		Timeout.Next = OS_Kernel.Private.TimerUnordered.Head;
		OS_Kernel.Private.TimerUnordered.Head = &Timeout;
//		Timeout.Next->Pre = &Timeout;
		OS_Kernel.Private.TimerUnordered.Tail = &Timeout;
	}
	else
	if(AbsoluteTime <= OS_Kernel.Private.TimerUnordered.Head->Timeout)
	{
		Timeout.Pre = (__WaitableWithTimeout*)&OS_Kernel.Private.TimerUnordered.Head;
		Timeout.Next = OS_Kernel.Private.TimerUnordered.Head;
		OS_Kernel.Private.TimerUnordered.Head = &Timeout;
		Timeout.Next->Pre = &Timeout;
	}
	//延时进行排序，尾插到无序队列
	else
	{
		Timeout.Pre = OS_Kernel.Private.TimerUnordered.Tail;
		Timeout.Next = NULL;
		OS_Kernel.Private.TimerUnordered.Tail = &Timeout;
		Timeout.Pre->Next = &Timeout;
	}
	AsyncWaitFast(WaitableObj);
	if(OS_Kernel.Private.TimerUnordered.Head == &Timeout)
	{	//需删除无序最早任务，触发排序
		__TimeoutSort();
	}
	Timeout.Pre->Next = Timeout.Next;
	if(Timeout.Next)
		Timeout.Next->Pre = Timeout.Pre;
}
//等待指定时长
__attribute__((always_inline, used))
static inline void AsyncWaitForFast(__AsyncWaitable_Typedef* WaitableObj, TimeStamp Duration)
{
	AsyncWaitAtFast(WaitableObj, GetTimeStamp() + Duration);
}

__attribute__((always_inline, used))
static inline void __Async_Exit2(__Async_Typedef* __async_p__)
{
	while(__GetSP() != __async_p__->sp)
	{
		YieldFast();
	}
	OS_Kernel.Private.StackPointBase = __async_p__->sp + __async_p__->size;
}

#define AsyncWaitable_NewVal(ValName, MaxSubject) __AsyncWaitable_Typedef ValName[MaxSubject + 1] = {0,}
/*
#define Endequeue(Queue_p, Node_p) do{\
		Node_p->Next=NULL;\
		typeof(Queue_p->Tail) Tail = Queue_p->Tail;\
		Queue_p->Tail = Node_p;\
		Tail->Next = Node_p;\
		Node_p->Pre = Tail;\
	}while(0)

#define __SetWaitObj(waitable_p, index, Subjects, ...) \
	do{\
		SubjectBase_Typedef *Subject_p = (sizeof(Subjects) == sizeof(uintptr_t))\
			?(*(SubjectBase_Typedef**)&(Subjects)):((SubjectBase_Typedef*)&(Subjects));\
		waitable_p[index].HeadLine.Size = index;\
		Endequeue(Subject_p, ((struct __Observation_Typedef*)(waitable_p + index)));\
		__VA_OPT__(__SetWaitObj(waitable_p, index + 1, __VA_ARGS__););\
	}while(0)

//TODO:未实现：清除现场
#define SetWaitObj(WaitableObj, ConditonFunction, Subjects...) \
	do{\
		size_t size;\
		AsyncWaitable_Typedef* waitable_p;\
		if(sizeof(WaitableObj)==sizeof(uintptr_t)){\
			size=sizeof(*(WaitableObj));\
			waitable_p = *(AsyncWaitable_Typedef**)&(WaitableObj);\
		}else{\
			size=sizeof((WaitableObj));\
			waitable_p = (AsyncWaitable_Typedef*)&(WaitableObj);\
		}\
		waitable_p[0].HeadLine.ResumePoint.Context = (void*)-1;\
		waitable_p[0].HeadLine.Condition = (ConditonFunction);\
		if(size < (__VA_ARGS_Count(Subjects) + 1) * sizeof(AsyncWaitable_Typedef))\
		{\
			waitable_p[0].HeadLine.Size = 0;\
		}else{\
			waitable_p[0].HeadLine.Size = __VA_ARGS_Count(Subjects);\
			__SetWaitObj(waitable_p, 1, Subjects);\
		}\
	}while(0)
	*/
#define SetWaitObj(WaitableObj, ConditonFunction, Subjects...)\
	do{\
		size_t size;\
		__AsyncWaitable_Typedef* waitable_p;\
		if(sizeof(WaitableObj)==sizeof(uintptr_t)){\
			size=sizeof(*(WaitableObj));\
			waitable_p = *(__AsyncWaitable_Typedef**)&(WaitableObj);\
		}else{\
			size=sizeof((WaitableObj));\
			waitable_p = (__AsyncWaitable_Typedef*)&(WaitableObj);\
		}\
		waitable_p[0].HeadLine.ResumePoint.Context = (void*)-1;\
		waitable_p[0].HeadLine.Condition = (ConditonFunction);\
		__ResetWaitObj(waitable_p);\
		if(size >= (__VA_ARGS_Count(Subjects) + 1) * sizeof(__AsyncWaitable_Typedef))\
		{\
			waitable_p[0].HeadLine.Size = __VA_ARGS_Count(Subjects);\
			void* temp[] = {Subjects};\
			__SetWaitObj(waitable_p, (SubjectBase_Typedef**)temp);\
		}\
	}while(0)


#define SystemCalls(SystemFunction, ...)\
	do{\
		RegType params[] = {__VA_ARGS__};\
		__SystemCall_Typedef temp = {.Next = NULL, .Param = &params};\
		OS_Kernel.Private.SyscallQueue.Tail->Next = &temp;\
		OS_Kernel.Private.SyscallQueue.Tail = &temp;\
	}while(false)


#endif /* ASYNC_RUNNINGTIME_INC_ASYNC_STATICINLINE_H_ */
