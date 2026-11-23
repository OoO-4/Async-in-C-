/*
 * Async_Struct.h
 *
 *  Created on: Aug 1, 2026
 *      Author: 32655
 *
 *	本文件用于定义外部可见的类型。
 */

#ifndef ASYNC_RUNNINGTIME_INC_ASYNC_STRUCT_H_
#define ASYNC_RUNNINGTIME_INC_ASYNC_STRUCT_H_
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdatomic.h>
#include "cmsis_gcc.h"
#include "StructuralTemplate.h"


#ifdef __thumb__
#define __reg_fp "r7"
#else
#define __reg_fp "r11"
#endif

#if 1//类型原型定义
typedef uint64_t TimeStamp;
typedef uintptr_t RegType;

//
typedef struct __Async_Config{
	bool IsAlloc	:1;//SupendFunction是否需要分配栈帧执行。使用时设置
	bool IsFree		:1;//SupendFunction退出是是否释放内存。使用时设置
	bool IsRun		:1;//SupendFunction是否运行函数体。用于进行函数栈帧预分配/资源回收。使用后马上清零
}__Async_Config;

//异步作用域辅助变量
typedef struct{RegType sp, hp;size_t size;__Async_Config flags;} __Async_Typedef;

// 异步的可挂起上下文
typedef struct __ResumeContext_Typedef{
	struct __ResumeContext_Typedef *Next, *Pre;
	RegType FramePoint;
	void* ProgramCounter;
} __ResumeContext_Typedef;

// 等待执行对象
struct __Observation_Typedef{
	struct __Observation_Typedef *Next, *Pre;
	size_t Index;
};
typedef union __AsyncWaitable_Typedef{
	struct {
		bool(*Condition)(size_t);//
		volatile union{
			__ResumeContext_Typedef* Context;
			struct{
				int8_t Claimed;					//-1:已抢占。因为内存对齐，Context值不可能0x1结尾。
				int8_t Contested;				//-1:已有处理
			};
		}ResumePoint;
		size_t Size;
	}HeadLine;
	struct __Observation_Typedef Observation;
} __AsyncWaitable_Typedef;

//通知者对象
#define Subject(_Ty) struct{SubjectBase_Typedef Observers; _Ty *Data;}
typedef Queue(struct __Observation_Typedef) SubjectBase_Typedef;
//typedef struct SubjectBase{
//	Queue(AsyncWaitable_Typedef) Observers;
//}SubjectBase;

// 软件定时器
typedef struct __WaitableWithTimeout{
	struct __WaitableWithTimeout *Next, *Pre;
	TimeStamp Timeout;
	__AsyncWaitable_Typedef* Context;
} __WaitableWithTimeout;

/*
 *  @brief	用户需要提供的硬件时钟配置函数
 * 	@param	Reload		需要配置的自动重装载值
 * 	@param	OS_Irq		需要被挂载在定时器中断的中断函数
 * 	@param	DMA_Count	使用DMA加速时DMA配置的循环次数			| 禁用DMA加速时参数为0
 * 	@param	DMA_Src		使用DMA加速时DMA配置的源地址（自增）		| 禁用DMA加速时参数为0
 * 	@param	DMA_Dest	使用DMA加速时DMA配置的目的地址（固定地址）	| 禁用DMA加速时参数为0
 */
typedef void(*TimerConfigFunc)(size_t Reload, void(*OS_Irq)(void));//, size_t DMA_Count, uint8_t* DMA_Src, uint8_t* DMA_Dest);

// 系统函数列表
typedef enum SystemFunction{
	SystemFunction_Done = 0,
	//用于判断系统调用号合法
	SystemFunction_Count,
	SystemFunction_UnknowError = -1,
} SystemFunction;

typedef struct TimeStampBase_Type{
	union{
		uint16_t u16;
		uint32_t u32[3];
		uint64_t u64;
	};
	uint16_t timerCNT;
}TimeStampBase_Type;

// 系统调用队列节点
typedef struct __SystemCall_Typedef{
	struct __SystemCall_Typedef *Next;
	RegType (*Param)[];//指向参数列表的指针。仅1个参数时直接传递参数内容
	SystemFunction SyscallNumber;
} __SystemCall_Typedef;

// 全局唯一的内核对象
typedef struct{
	bool	Exit	: 1;//完成当前任务后终止当前进行的事件循环,强制退出调度。该返回方式可能导致堆上内存未释放！
	bool	ResetCost:1;//重新开始计算最大调度周期
	union{
		const struct {//定义外部可见不可写对象。必须于private前n项相同
			volatile TimeStamp CurrentTime;//实时时间，持续更新
			volatile TimeStamp MaxLoopCost;//当前最大调度周期。用于性能分析/高精度定时。
		};
		struct {
			volatile TimeStamp CurrentTime;
			volatile TimeStamp MaxLoopCost;
			TimeStamp NextTick;//下一时刻时钟。用于
			void(*TimerCleanup)(void);
//			size_t TimerDMACount;
			size_t TimerFreq_kHz;
			size_t TimerCycle;
			uint16_t* TimerCNT;
			__ResumeContext_Typedef* RunningTask;		//当前执行中任务的上下文对象
			Queue(__SystemCall_Typedef) SyscallQueue;	//维护当前循环中挂起的系统调用
			RegType StackPointBase;//异步任务的临时栈栈底
			RegType CalleeFrame;//调用者保存的被调用者栈帧指针
			size_t	CalleeFrameSize;//调用者保存的被调用者栈帧指针大小
			Queue(__ResumeContext_Typedef) ReadyQueue;//就绪队列，循环完成后加入调度
			Queue(__WaitableWithTimeout) TimerUnordered;
			Queue(__WaitableWithTimeout) TimerOrdered;
			__Async_Config Flags;
		}Private;
	};
} OS_Kernel_Typedef;
#endif

#if 1//外部可见对象声明
extern OS_Kernel_Typedef OS_Kernel;
#endif



#endif /* ASYNC_RUNNINGTIME_INC_ASYNC_STRUCT_H_ */
