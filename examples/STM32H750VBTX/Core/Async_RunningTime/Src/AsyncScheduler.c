/*
 * AsyncScheduler.c
 *
 *  Created on: Aug 1, 2026
 *      Author: 32655
 *
 *	本文件用于编写调度相关功能。
 */

#include "__Async.h"
#if 1 //全局变量区
OS_Kernel_Typedef OS_Kernel = {
		.Private = {
				.SyscallQueue.Tail = (void*)&OS_Kernel.Private.SyscallQueue.Head,
				.ReadyQueue.Tail = (void*)&OS_Kernel.Private.ReadyQueue.Head,
				.TimerUnordered.Tail = (void*)&OS_Kernel.Private.TimerUnordered.Head,
				.TimerOrdered.Tail = (void*)&OS_Kernel.Private.TimerOrdered.Head,
		},
};
#endif
#if 1 //静态声明
void __TimeoutSort(void);
static inline void SystemInterface(void);
//static inline void RunToBlock(void);
static inline void BlockToReady(__ResumeContext_Typedef* Context);
static inline void TimeoutToReady(void);
static inline bool TimeoutInMS(void);
static void OS_Tick_IRQ(void);
static void __StartupRoutine(void);
static void __StartupRoutine2(void);
static inline __WaitableWithTimeout* SortTasksByMerge(__WaitableWithTimeout* Tasks);
static inline __WaitableWithTimeout* Merge(__WaitableWithTimeout* l1, __WaitableWithTimeout* l2);
static RegType __Async_Entry__(__Async_Typedef* __async_p__, RegType sp);
static void __Async_Exit__(__Async_Typedef* __async_p__, RegType sp);
//static inline uint8_t CLZ(uint32_t value);
#endif



#if 1 //接口函数定义

//开始异步调度。正常使用中永不返回
void Async_Run(void(*Main)(void))
{
	__ResumeContext_Typedef SystemInterfaceContex;
	SystemInterfaceContex.Next = &SystemInterfaceContex;
	SystemInterfaceContex.Pre = &SystemInterfaceContex;
	OS_Kernel.Private.RunningTask = &SystemInterfaceContex;
	Async_RunTask(Main);
	SystemInterface();
}
void __TestAndReady(__AsyncWaitable_Typedef *Waitable, size_t index)
{
	typeof(((__AsyncWaitable_Typedef*)0)->HeadLine) *this = (void*)Waitable;
	__ResumeContext_Typedef *Local_Context = this->ResumePoint.Context;
	if(this->ResumePoint.Claimed != -1)
	{
		if(this->Condition(index))
		{
			this->ResumePoint.Claimed = -1;
			__CMB();
			if(this->ResumePoint.Contested != -1)
			{
				this->ResumePoint.Contested = -1;
				__CMB();
				BlockToReady(Local_Context);
			}
		}
	}
}
//通知频道的全部观察者
void SubjectNotify(SubjectBase_Typedef* Subject)
{
	struct __Observation_Typedef* Observer = Subject->Head;
	while(Observer)
	{
//		typeof(((__AsyncWaitable_Typedef*)0)->HeadLine) *this = (typeof(this))(((__AsyncWaitable_Typedef*)Observer) - Observer->Index);

		__TestAndReady(((__AsyncWaitable_Typedef*)Observer) - Observer->Index, Observer->Index);
		Observer = Observer->Next;
	}
}

/*
 * @brief	配置硬件定时器资源
 * @param 	TimerFreq_Hz	定时器定时频率
 * @param	TimerCnt		读取硬件定时值的指针最多可能需要提供16位定时
 * @param 	TimerConfig		配置定时器的函数
 * @param	DMACount		DMA最大传输次数 | 0:禁用DMA加速
 * @param	Cleanup			用于变频系统中更新定时器配置时自动调用。NULL为不需要自动清除
 * 使用系统软件定时器需要保证时钟频率在1k~65536k。超过1k部分走硬件[+DMA]模式
 */
bool Async_SetOSTimer(size_t TimerFreq_kHz, uint16_t* TimerCnt, TimerConfigFunc TimerConfig, /*size_t DMACount, */void(*Cleanup)(void))
{
////时钟频率大概率与系统总线频率直接相关，防止DMA占满总线
//#define DMAFre (64)
//	static const uint8_t TimeStampDMA[] =
//	{		//生成脚本
//			//(()=>{let s = "";for(let i = 0; i < 256; i++){if(!(i & 0xF))s+="\n";s+=`0x${("00" +(i + 1).toString(16)).slice(-2)}, `}console.log(s)})()
//			0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10,
//			0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20,
//			0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30,
//			0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40,
//			0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x50,
//			0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f, 0x60,
//			0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70,
//			0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f, 0x80,
//			0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f, 0x90,
//			0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f, 0xa0,
//			0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf, 0xb0,
//			0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf, 0xc0,
//			0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf, 0xd0,
//			0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf, 0xe0,
//			0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef, 0xf0,
//			0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff, 0xff,
//	};
	if(TimerFreq_kHz < 1 || TimerFreq_kHz > 65536)
	{
		return false;
	}
	if(OS_Kernel.Private.TimerCleanup)
	{
		OS_Kernel.Private.TimerCleanup();
	}
//	if(DMACount && TimerFreq_kHz >= DMAFre)
//	{
//		DMACount = (DMACount > 256)
//				?(256)
//				:(DMACount);
//		DMACount = (DMACount > (TimerFreq_kHz / DMAFre))
//				?(TimerFreq_kHz / DMAFre)
//				:(DMACount);
//	}
//	else
//	{
//		DMACount = 0;
//	}
	OS_Kernel.Private.TimerCNT = TimerCnt;
	OS_Kernel.Private.TimerFreq_kHz = TimerFreq_kHz;
	OS_Kernel.Private.TimerCycle = (uint32_t)(1 << 31) / TimerFreq_kHz;
//	OS_Kernel.Private.TimerDMACount = DMACount;
	OS_Kernel.Private.TimerCleanup = Cleanup;
//	if(DMACount)
//	{	//进行DMA精度提升
//		TimerConfig(TimerFreq_kHz / DMACount - 1, OS_Tick_IRQ, DMACount, (uint8_t*)TimeStampDMA, (uint8_t*)(1 + (uintptr_t)&(OS_Kernel.Private.CurrentTime)));
//	}
//	else
	{	//不进行DMA精度提升
		TimerConfig(TimerFreq_kHz - 1, OS_Tick_IRQ);//, 0, 0, 0);
	}
	return true;
//#undef DMAFre
}
__attribute__((noinline))
static void __Async_RunTask(void (*Task)(void), __ResumeContext_Typedef *const newContext, void (*StartupRoutine)(void))
{
	__ResumeContext_Typedef *const thisContext = OS_Kernel.Private.RunningTask;
	newContext->ProgramCounter = StartupRoutine;
	newContext->FramePoint = (RegType)Task;
	newContext->Pre = thisContext->Pre;
	newContext->Next = thisContext;
	newContext->Pre->Next = newContext;
	thisContext->Pre = newContext;
}
//创建一个新任务流并加入调度。
//必要时可通过创建任务后直接执行，通过r0-r3传递传输到任务函数或传递到原执行流。
//需自定义传参规则与向前一任务发起调用的方式并保证任务之间连续。
//正常调度下只有本函数会向任务前插入任务；只有任务执行结束/任务主动挂起会将任务从就绪队列中摘除。
//该特性可用于实现生成器或其他懒计算封装。
void Async_RunTask(void (*Task)(void))
{
	__Async_RunTask(Task, (__ResumeContext_Typedef *)malloc(sizeof(__ResumeContext_Typedef)), __StartupRoutine);
}

__attribute__((naked))
void Async_RunTaskFast(void (*Task)(void))
{
	__asm__ volatile(
			"ldr r2, =%[StackPointBase]\n"
			"ldr r1, [r2, #0]\n"
			"push {r1}\n"
			"sub sp, sp, %[sizeof_rctx]\n"
			"mov r1, sp\n"
			"str r1, [r2, #0]\n"
			"ldr r2, =%[StartupRoutine2]\n"
			"b %[__Async_RunTask]\n"
			::
			[sizeof_rctx]"i"(sizeof(__ResumeContext_Typedef)),
			[StackPointBase]"i"(&OS_Kernel.Private.StackPointBase),
			[StartupRoutine2]"i"(__StartupRoutine2),
			[__Async_RunTask]"i"(__Async_RunTask));
}

void GetTimeStampBase(TimeStampBase_Type *const TSB)
{
	static volatile uint32_t *const OS_TimeStamp = (uint32_t*)&OS_Kernel.Private.CurrentTime;
	//使DMA加速段有效更新
//	SCB_CleanInvalidateDCache_by_Addr((uint32_t*)OS_TimeStamp, sizeof(TimeStamp));
	TSB->u32[2] = OS_TimeStamp[0];
	__CMB();
	//受双读保护的硬件时间域
	TSB->timerCNT = *OS_Kernel.Private.TimerCNT;
	__CMB();
	TSB->u32[0] = OS_TimeStamp[0];
	__CMB();
	TSB->u32[1] = OS_TimeStamp[1];
}

TimeStamp DecodeTimeStampBase(TimeStampBase_Type *const TSB)
{
	if(TSB->u32[0] != TSB->u32[2])
	{
		TSB->timerCNT = 0;
	}
	TSB->u32[1] -= (TSB->u32[1] - TSB->u32[0]) & 0b11;
	TSB->u16 = (((TSB->u16 & 0b11) + TSB->timerCNT)	//转化为单调计数值
			* OS_Kernel.Private.TimerCycle) >> 15;	//转化为平均增长格式
	return TSB->u64;
}
#if 0
TimeStamp GetTimeStamp(void)
{
	return OS_Kernel.Private.CurrentTime + *OS_Kernel.Private.TimerCNT;
}
#else
TimeStamp GetTimeStamp(void)
{
	TimeStampBase_Type TSB;
	GetTimeStampBase(&TSB);
	return DecodeTimeStampBase(&TSB);
}
#endif

//清除已挂起的监视器
void __ResetWaitObj(__AsyncWaitable_Typedef*const waitable)
{
	const size_t size = waitable[0].HeadLine.Size;
	waitable[0].HeadLine.Size = 0;
	for(size_t index = 0; index < size; index++)
	{
		struct __Observation_Typedef* const this = &(waitable[index + 1].Observation);
		this->Next->Pre = this->Pre;
		this->Pre->Next = this->Next;
	}
}

//挂起监视器（但未生效）
void __SetWaitObj(__AsyncWaitable_Typedef*const waitable, SubjectBase_Typedef**const subject)
{
	const size_t size = waitable[0].HeadLine.Size;
	waitable[0].HeadLine.ResumePoint.Context = (void*)-1;
	for(size_t index = 0; index < size; index++)
	{
		struct __Observation_Typedef* const this = &(waitable[index + 1].Observation);
		struct __Observation_Typedef* const Tail = subject[index]->Tail;
		this->Index = index + 1;
		Tail->Next = this;
		subject[index]->Tail = this;
		this->Pre = Tail;
	}
}

void __TimeoutSort(void)
{
	__WaitableWithTimeout *Task = OS_Kernel.Private.TimerUnordered.Head;
	OS_Kernel.Private.TimerUnordered.Head = NULL;
	OS_Kernel.Private.TimerUnordered.Tail
	= (__WaitableWithTimeout*)&(OS_Kernel.Private.TimerUnordered.Head);

	Task = Merge(SortTasksByMerge(Task)
			, OS_Kernel.Private.TimerOrdered.Head);
	OS_Kernel.Private.TimerOrdered.Head = Task;
	Task->Pre = (void*)&OS_Kernel.Private.TimerOrdered.Head;
	while(Task->Next)
	{
		Task->Next->Pre = Task;
		Task = Task->Next;
	}
	OS_Kernel.Private.TimerOrdered.Tail = Task;
}

__attribute__((naked))
void __RunTask(__ResumeContext_Typedef* TCB)
{
	__asm__ volatile(
		"ldr r1, =%[OS]\n"
		"str r0, [r1, %[TaskPoint]]\n"			// OS_Kernel.Private.RunningTask = TCB
		"ldr "__reg_fp", [r0, %[offset_fp]]\n"	// fp = TCB->FramePoint
		"ldr pc, [r0, %[offset_pc]]\n"			// pc = TCB->ProgramCounter
		::
		[OS]	   "i"(&OS_Kernel),
		[TaskPoint]"i"(offsetof(OS_Kernel_Typedef, Private.RunningTask)),
		[offset_pc]"i"(offsetof(__ResumeContext_Typedef, ProgramCounter)),
		[offset_fp]"i"(offsetof(__ResumeContext_Typedef, FramePoint)));
}

#endif
#if 1 //内部函数定义

#include "main.h"
//调度中进行系统调度的接口
static inline void SystemInterface(void)
{
	__ResumeContext_Typedef *SystemTask = OS_Kernel.Private.RunningTask;
	OS_Kernel.Private.Flags = (const struct __Async_Config){.IsAlloc = 1, .IsFree = 1, .IsRun = 1,};
	OS_Kernel.Private.StackPointBase = __GetSP();
	SystemTask->FramePoint = __GetFP();
	SystemTask->ProgramCounter = (void*)(1 + (uintptr_t)&&Begin);
	goto *SystemTask->ProgramCounter;
//	volatile __attribute__((used)) TimeStamp cost;
Begin:
	InvalidateRegisters();
//	cost = GetTimeStamp();
	if(OS_Kernel.Exit)
	{	//预留的强制退出接口，未考虑堆平衡
		return;
	}
	//step1:处理系统调用
	if(OS_Kernel.Private.SyscallQueue.Head != NULL)
	{
		QueueGetType(OS_Kernel.Private.SyscallQueue)* Local_Head
				= OS_Kernel.Private.SyscallQueue.Head;
		OS_Kernel.Private.SyscallQueue.Head = NULL;
		__CMB();
		OS_Kernel.Private.SyscallQueue.Tail = (typeof(Local_Head))&(OS_Kernel.Private.SyscallQueue);
		__CMB();
		while(Local_Head)
		{
			typeof(Local_Head) least_head;
			//系统调用/新建2就绪/阻塞到就绪
			switch(Local_Head->SyscallNumber)
			{
			case SystemFunction_Done:
				break;
			default:
				Local_Head->SyscallNumber = SystemFunction_UnknowError;
				break;
			}
			least_head = Local_Head;
			Local_Head = Local_Head->Next;
			least_head->Next = NULL;
		}
	}


	while(1)//等待到有任务需处理
	{
		extern WWDG_HandleTypeDef hwwdg1;
		HAL_WWDG_Refresh(&hwwdg1);

		//step2:就绪超时任务
		TimeoutToReady();

		__disable_irq();
		//step3:处理ReadyQueue
		if(OS_Kernel.Private.ReadyQueue.Head != NULL)
		{
			__enable_irq();
			__ResumeContext_Typedef* Local_Head
					= OS_Kernel.Private.ReadyQueue.Head;
			__ResumeContext_Typedef* Local_Tail
					= OS_Kernel.Private.ReadyQueue.Tail;
			OS_Kernel.Private.ReadyQueue.Head = NULL;
			__CMB();
			OS_Kernel.Private.ReadyQueue.Tail = (__ResumeContext_Typedef*)&(OS_Kernel.Private.ReadyQueue.Head);
			__CMB();
			Local_Head->Pre = SystemTask->Pre;
			Local_Head->Pre->Next = Local_Head;
			Local_Tail->Next = SystemTask;
			SystemTask->Pre = Local_Tail;

			OS_Kernel.Private.RunningTask = Local_Head;//优先调度新就绪任务
			break;
		}
		else if(SystemTask != SystemTask->Next)//运行队列非空，可继续执行
		{
			__enable_irq();
			OS_Kernel.Private.RunningTask = SystemTask->Next;
			break;
		}
		else if(!TimeoutInMS())//定时器中断后才就绪，睡眠等待
		{
			__WFI();
			__enable_irq();
		}
		__enable_irq();
	}
//	cost = GetTimeStamp() - cost;
	InvalidateRegisters();
	__RunTask(OS_Kernel.Private.RunningTask);
}

//将任务流加入就绪队列
static inline void BlockToReady(__ResumeContext_Typedef* Context)
{	//单一消费者在最低优先级，所以可以依赖顺序保证安全。
	Context->Next = NULL;
	Context->Pre = OS_Kernel.Private.ReadyQueue.Tail;
	__CMB();
	OS_Kernel.Private.ReadyQueue.Tail = Context;
	__CMB();
	while(Context->Pre->Next != NULL)
	{
		Context->Pre = Context->Pre->Next;
	}
	Context->Pre->Next = Context;
}
//超时任务入队
static inline void TimeoutToReady(void)
{
	const TimeStamp CurrentTime = GetTimeStamp();
	if(OS_Kernel.Private.TimerUnordered.Head != NULL
	&& OS_Kernel.Private.TimerUnordered.Head->Timeout <= CurrentTime)
	{	//无序队列发生超时，统一进行排序
		__TimeoutSort();
	}
	if(OS_Kernel.Private.TimerOrdered.Head != NULL
	&& OS_Kernel.Private.TimerOrdered.Head->Timeout <= CurrentTime)
	{	//存在超时任务，将所有超时任务入就绪队列
		//迭代器初始化为哨兵节点
		__WaitableWithTimeout* iter = (__WaitableWithTimeout*)&OS_Kernel.Private.TimerOrdered.Head;//(struct __WaitableWithTimeout *)&(OS_Kernel.Private.TimerOrdered);
		__WaitableWithTimeout* first = iter->Next;
		while(iter->Next && iter->Next->Timeout <= CurrentTime)
		{
			iter = iter->Next;
			typeof(((__AsyncWaitable_Typedef*)0)->HeadLine) *this = &(iter->Context->HeadLine);
			__ResumeContext_Typedef *Local_Context = this->ResumePoint.Context;
			if(this->ResumePoint.Claimed != -1)
			{
				this->Condition(-1);
				this->ResumePoint.Claimed = -1;
				__CMB();
				if(this->ResumePoint.Contested != -1)
				{
					this->ResumePoint.Contested = -1;
					__CMB();
					BlockToReady(Local_Context);
				}
			}
		}
		OS_Kernel.Private.TimerOrdered.Head = iter->Next;
		if(iter->Next)
			iter->Next->Pre = (struct __WaitableWithTimeout *)&(OS_Kernel.Private.TimerOrdered);
		//使超时的任务成环，减少边界检测开销
		first->Pre = iter;
		iter->Next = first;

	}
}

//判断是否存在下一次定时器中断前的超时任务
static inline bool TimeoutInMS(void)
{
	return  (OS_Kernel.Private.TimerOrdered.Head->Timeout < OS_Kernel.Private.NextTick) ||
			(OS_Kernel.Private.TimerUnordered.Head->Timeout < OS_Kernel.Private.NextTick);
}

//系统时钟中断回调函数
static void OS_Tick_IRQ(void)
{
	static uint32_t *const p_next_tick = (uint32_t*)&OS_Kernel.Private.NextTick;
	static uint32_t *const p_curr_time = (uint32_t*)&OS_Kernel.Private.CurrentTime;

	//写入顺序与读取顺序相反，保证任意顺序执行顺序高位只会比低位新(转化为单向误差，以便修正)
	p_curr_time[1] = p_next_tick[1];
	__CMB();
	p_curr_time[0] = p_next_tick[0];
//	//写入后刷新缓存，避免覆盖DMA结果
//	SCB_CleanInvalidateDCache_by_Addr(p_curr_time, sizeof(TimeStamp));

	OS_Kernel.Private.NextTick += 0x10000;					//每tick增加1ms
	p_next_tick[0] &= ~(uint64_t)0x0FFFF;		//0~7位无效|8~15位DMA更新
	p_next_tick[0] |= p_next_tick[1] & 0b11;	//2为映射高32bit时钟，用于保证读取无撕裂
}

//任务流入口函数
__attribute__((naked))
static void __StartupRoutine(void)
{
	//新任务流入口函数
	//管理任务流控制块生命周期
	__asm__ volatile(
			"ldr r4, =%[OS]\n"					//被调用者保存寄存器，函数调用前后可复用
			"blx "__reg_fp"\n"					//执行任务函数
			"ldr r0, [r4, %[TaskPoint]]\n"		//*this
			"ldr r1, [r0, %[offset_pr]]\n"		//*pre
			"ldr r2, [r0, %[offset_ne]]\n"		//*next
			"str r1, [r2, %[offset_pr]]\n"		//next->pre = pre
			"str r2, [r1, %[offset_ne]]\n"		//pre->next = next
			"str r2, [r4, %[TaskPoint]]\n"		//this = next
			"ldr lr, [r2, %[offset_pc]]\n"		//free后直接跳转到下一任务
			"ldr "__reg_fp", [r2, %[offset_fp]]\n"//加载下一任务栈帧
			"b free\n"
			::
			[OS]	   "i"(&OS_Kernel),
			[SP_Base]  "i"(offsetof(OS_Kernel_Typedef, Private.StackPointBase)),
			[TaskPoint]"i"(offsetof(OS_Kernel_Typedef, Private.RunningTask)),
			[offset_pc]"i"(offsetof(__ResumeContext_Typedef, ProgramCounter)),
			[offset_fp]"i"(offsetof(__ResumeContext_Typedef, FramePoint)),
			[offset_pr]"i"(offsetof(__ResumeContext_Typedef, Pre)),
			[offset_ne]"i"(offsetof(__ResumeContext_Typedef, Next)));
}
__attribute__((naked))
void __StartupRoutine2(void)
{
	__asm__ volatile(
		"blx "__reg_fp"\n"					//执行任务函数
		"ldr r3, =%[OS]\n"					//r3:OS_Kernel
		"ldr r0, [r3, %[TaskPoint]]\n"		//r0:*this/现stackpointbase
		"mov "__reg_fp", r0\n"				//fp = r0
		"str r0, [r0, %[offset_fp]]\n"		//set fp
		"ldr r1, yieldpoint + 1\n"
		"str r1, [r0, %[offset_pc]]\n"		//set pc
		"yieldpoint:\n"
		"mov r0, "__reg_fp"\n"				//r0:*this
		"mov r1, sp\n"						//r1:sp
		"cmp r0, r1\n"
		"ldr r2, [r0, %[offset_ne]]\n"		//r2:*next
		"str r2, [r3, %[TaskPoint]]\n"		//this = next
		"ldr r3, =%[OS]\n"					//r3:OS_Kernel
		//不满足LIFO, 忙等
		"bne runnext\n"
		//执行退出动作
		"add sp, sp, %[sizeof_rctx]\n"
		"pop {r1}\n"						//r1:原StackPointBase
		"str r1, [r3, %[SP_Base]]\n"
		"ldr r1, [r0, %[offset_pr]]\n"		//r1:*pre
		"str r1, [r2, %[offset_pr]]\n"		//next->pre = pre
		"str r2, [r1, %[offset_ne]]\n"		//pre->next = next
		"runnext:\n"
		"ldr "__reg_fp", [r2, %[offset_fp]]\n"//加载下一任务栈帧
		"ldr pc, [r2, %[offset_pc]]\n"		//跳转到下一任务
		::
		[OS]	   "i"(&OS_Kernel),
		[SP_Base]  "i"(offsetof(OS_Kernel_Typedef, Private.StackPointBase)),
		[TaskPoint]"i"(offsetof(OS_Kernel_Typedef, Private.RunningTask)),
		[sizeof_rctx]"i"(sizeof(__ResumeContext_Typedef)),
		[offset_pc]"i"(offsetof(__ResumeContext_Typedef, ProgramCounter)),
		[offset_fp]"i"(offsetof(__ResumeContext_Typedef, FramePoint)),
		[offset_pr]"i"(offsetof(__ResumeContext_Typedef, Pre)),
		[offset_ne]"i"(offsetof(__ResumeContext_Typedef, Next)));
}

//仅做单链表排序，还需恢复反向链表
__attribute__((always_inline))
static inline __WaitableWithTimeout* SortTasksByMerge(__WaitableWithTimeout* Tasks)
{
//归并用缓存大小，支持最多POW(2,MAX_LEVEL)个节点的无序链表
#define MAX_LEVEL	(10)
	__WaitableWithTimeout* cache[MAX_LEVEL] = {NULL,};  // 缓存各级归并后的链表头。0表示无效
	__WaitableWithTimeout* next;
	int i = 0;

	cache[0] = Tasks;
	while (cache[0]) {
		// 取出当前节点作为一级缓存
		next = cache[0]->Next;
		// 拆分成单独链表进行计算
		cache[0]->Next = NULL;

		// 逐级向上合并，直到遇到空缓存

		int level = 0;// 当前合并到的层
		while(cache[level] && cache[level + 1]) {
			cache[level + 1] = Merge(cache[level], cache[level + 1]);
			cache[level++] = NULL;
		}

		cache[level + 1] = cache[level];
		cache[level] = NULL;

		cache[0] = next;
	}

	// 合并所有缓存
	for (; i < MAX_LEVEL - 1; i++) {
		cache[i + 1] = Merge(cache[i], cache[i + 1]);
	}

	return cache[MAX_LEVEL - 1];
#undef MAX_LEVEL
}

__attribute__((always_inline))
static inline __WaitableWithTimeout* Merge(__WaitableWithTimeout* l1, __WaitableWithTimeout* l2)
{
    __WaitableWithTimeout dummy;
    __WaitableWithTimeout* tail = &dummy;

    while (l1 && l2) {
        if (l1->Timeout <= l2->Timeout) {
            tail->Next = l1;
            l1 = l1->Next;
        } else {
            tail->Next = l2;
            l2 = l2->Next;
        }
        tail = tail->Next;
    }

    tail->Next = l1 ? l1 : l2;
    return dummy.Next;
}

//async_on_Entry
__attribute__((naked))
void __Async_Entry(__Async_Typedef* __async_p__)
{
	__asm__ volatile(
			"mov r1, sp\n"
			"push {lr}\n"
			"bl __Async_Entry__\n"
			"add "__reg_fp", "__reg_fp", r0\n");
	__asm__ volatile(
			"ldr r0, [sp]\n"
			"mov sp, %[sp_base]\n"
			"bx r0\n"::[sp_base]"r"(OS_Kernel.Private.StackPointBase));
	(void)__Async_Entry__;
}
//栈上分配版本（需要保证永不释放）
__attribute__((naked))
void __Async_Entry2(__Async_Typedef* __async_p__)
{
	__asm__ volatile(
			"ldr r1, =%[OS_kernel]\n"
			"ldr r2, [r1, %[Offset_Flag]]\n"
			"str r2, [r0, %[async_flags]]\n"
			"mov r2, sp\n"
			"str r2, [r0, %[async_sp]]\n"
			"str r2, [r0, %[async_hp]]\n"
			"str r2, [r1, %[Offset_Stack]]\n"
			"bx lr\n"
			::
			[async_flags]	"i"(offsetof(__Async_Typedef, flags)),
			[async_sp]		"i"(offsetof(__Async_Typedef, sp)),
			[async_hp]		"i"(offsetof(__Async_Typedef, hp)),
			[OS_kernel]		"i"(&OS_Kernel),
			[Offset_Flag]	"i"(offsetof(OS_Kernel_Typedef, Private.Flags)),
			[Offset_Stack]	"i"(offsetof(OS_Kernel_Typedef, Private.StackPointBase))
			);
}
//async_on_Entry内部独立栈帧部分
//返回值：HeapFrame - StackFrame
__attribute__((used))
static RegType __Async_Entry__(__Async_Typedef* __async_p__, RegType sp)
{
	__async_p__->flags = OS_Kernel.Private.Flags;
	__async_p__->sp = sp;
	__async_p__->size = OS_Kernel.Private.StackPointBase - sp;
	if(__async_p__->flags.IsAlloc || (OS_Kernel.Private.CalleeFrameSize < __async_p__->size))
	{
		__async_p__->hp = (RegType)malloc(__async_p__->size);
	}
	else
	{
		OS_Kernel.Private.CalleeFrame += __async_p__->size;
		OS_Kernel.Private.CalleeFrameSize -= __async_p__->size;
		__async_p__->hp = OS_Kernel.Private.CalleeFrame;
	}

	for(size_t i = 0; i < __async_p__->size; i += sizeof(int))
	{
		*(int*)(__async_p__->hp + i) = *(int*)(__async_p__->sp + i);
	}

	return __async_p__->hp - __async_p__->sp;
}

//async_on_Exit
__attribute__((naked))
void __Async_Exit(__Async_Typedef* __async_p__)
{
	__asm__ volatile(
			"ldr r1, [r0, %[offset_sp]]\n"
			"mov sp, r1\n"
			"ldr r2, [r0, %[offset_hp]]\n"
			"sub r2, r1, r2\n"
			"add "__reg_fp", "__reg_fp", r2\n"
			"b __Async_Exit__\n"
			::
			[offset_sp]"i"(offsetof(__Async_Typedef, sp)),
			[offset_hp]"i"(offsetof(__Async_Typedef, hp)));
	(void)__Async_Exit__;
}

__attribute__((used))
static void __Async_Exit__(__Async_Typedef* __async_p__, RegType sp)
{
	for(size_t i = 0; i < __async_p__->size; i += sizeof(int))
	{
		*(int*)(__async_p__->sp + i) = *(int*)(__async_p__->hp + i);
	}
	if(__async_p__->flags.IsFree)
	{
		if(__async_p__->hp != OS_Kernel.Private.CalleeFrame - __async_p__->size)
		{
			free((void*)(OS_Kernel.Private.CalleeFrame));
		}
		free((void*)(__async_p__->hp));
		OS_Kernel.Private.CalleeFrame = (RegType)NULL;
		OS_Kernel.Private.CalleeFrameSize = 0;
	}
	else
	{
		OS_Kernel.Private.CalleeFrame = __async_p__->hp;
		OS_Kernel.Private.CalleeFrameSize = __async_p__->size;
	}
}

////前导0计算
//__attribute__((always_inline,unused))
//static inline uint8_t CLZ(uint32_t value)
//{
//  /* Even though __builtin_clz produces a CLZ instruction on ARM, formally
//     __builtin_clz(0) is undefined behaviour, so handle this case specially.
//     This guarantees ARM-compatible results if happening to compile on a non-ARM
//     target, and ensures the compiler doesn't decide to activate any
//     optimisations using the logic "value was passed to __builtin_clz, so it
//     is non-zero".
//     ARM Compiler 6.10 and possibly earlier will optimise this test away, leaving a
//     single CLZ instruction.
//   */
//  if (value == 0U)
//  {
//    return 32U;
//  }
//  return __builtin_clz(value);
//}

#endif
