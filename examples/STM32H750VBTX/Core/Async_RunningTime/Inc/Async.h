/*
 * Async.h
 *
 *  Created on: Aug 1, 2026
 *      Author: 32655
 *
 *	本文件用于包含全部外部可见接口。
 *	VER 1.0.1
 */

#ifndef ASYNC_RUNNINGTIME_INC_ASYNC_H_
#define ASYNC_RUNNINGTIME_INC_ASYNC_H_

//链表节点结构体顺序：Next-Pre-Other
//队列结构体顺序：Head-Tail

//本库依赖帧指针，禁用帧指针优化  -fno-omit-frame-pointer
#pragma GCC optimize ("no-omit-frame-pointer")


#include "Async_Struct.h"
#include "Async_Atomic.h"
#include "Async_Function.h"
#include "Async_Staticinline.h"




#endif /* ASYNC_RUNNINGTIME_INC_ASYNC_H_ */
