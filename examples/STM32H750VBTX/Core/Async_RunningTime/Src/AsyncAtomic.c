/*
 * AsyncAtomic.c
 *
 *  Created on: Aug 6, 2026
 *      Author: 32655
 */
#include "__Async.h"

#if (HAS_HW_ATOMIC == 0)
#else
__attribute__((noinline))
int32_t __STREX_PTR(AtomicPtr* const AtomicPtr_Point, const uintptr_t new_Var, const int32_t Version)
{
	__Interrupt_Frame *p_hstack = (__Interrupt_Frame*)(AtomicPtr_Point->Owner);
	int32_t L_Version = AtomicPtr_Point->Version;
	if(L_Version != Version || Version < 0)
	{	//开始写入前原加载的版本已失效,直接退出
		//主要用于允许丢失时减低冲突时效率（可删除）
		return -2;
	}
	if(p_hstack
		&& !(((uintptr_t)p_hstack & 0x02)
			&& (((uintptr_t)p_hstack ^ L_Version) & 0x01)))
	{	//失效低优先级正在进行的写入
		//该分支重复执行不产生副作用
		//剔除额外信息，获取实际栈帧
		p_hstack = (__Interrupt_Frame*)((uintptr_t)p_hstack & (~0x7));
		//设置低优先级返回错误码
		p_hstack->Base_Stack.R0 = -1;
		//设置低优先级直接退出执行
		p_hstack->Base_Stack.PC = (RegType)&&STR_EX_FAULT;
	}
	__DMB();
	//原子进入独占状态1--版本号处理阶段
	//设置Owner为发生中断压栈后的栈指针
	AtomicPtr_Point->Owner = ((uintptr_t)__GetSP() & (~0x07))
		-  ((__get_CONTROL() & 0x04)?(sizeof(p_hstack->FPU_Stack)):(sizeof(p_hstack->Base_Stack)));
	__DMB();
	L_Version = AtomicPtr_Point->Version;
	if(L_Version != Version || Version < 0)
	{	//版本发生变更，退出临界区并返回错误码
		AtomicPtr_Point->Owner = 0;
		return -2;
	}
	//计算新版本号
	if(++L_Version < 0)
	{
		L_Version = 0;
	}
	//原子进入独占状态2--绑定版本号的预备状态
	AtomicPtr_Point->Owner |= 0x02 | (L_Version & 0x01);
	__DMB();
	//写入数据，但未生效
	AtomicPtr_Point->Data[L_Version & 0x01] = new_Var;
	__DMB();
	//原子进入独占状态3--生效写入数据，再被中断不跳转结束
	AtomicPtr_Point->Version = L_Version;
	__DMB();
	//退出临界区
	AtomicPtr_Point->Owner = 0;
	__asm__("":::"memory");
	{//局部作用域内定义r0，减少寄存器占用副作用
		register uintptr_t r0 asm("r0") = L_Version;
//被高优先级打断后跳转到此处，保证检查点到写入点无窗口期。
//前后都使用r0，保证异常分支中设置的r0成功替代返回值。
STR_EX_FAULT:
		__asm__("":"+r"(r0)::);
		return r0;
	}
}
#endif

