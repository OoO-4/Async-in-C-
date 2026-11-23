/*
 * StructuralTemplate.h
 *
 *  Created on: Aug 1, 2026
 *      Author: 32655
 *
 *	本文件用于定义类型模板。
 */

#ifndef ASYNC_RUNNINGTIME_INC_STRUCTURALTEMPLATE_H_
#define ASYNC_RUNNINGTIME_INC_STRUCTURALTEMPLATE_H_
//队列类型模板，包含头/尾指针
#define Queue(_Type) typeof(struct{_Type *Head, *Tail;})
#define QueueGetType(_Type) typeof(*(_Type.Head))




#endif /* ASYNC_RUNNINGTIME_INC_STRUCTURALTEMPLATE_H_ */
