# Async-in-C-
C语言同步语法异步编程。适合用于资源受限的嵌入式设备。

---

### 更轻量，更高效，更抽象

## 项目说明

> 嵌入式多任务并发时常使用RTOS这样的调度系统进行多任务并发执行。但RTOS这种系统：使用抢占式调度，存在大量非必要开销；使用外部指定的任务栈，存在大量无用内存浪费。
> 为解决上述问题，设计本系统。本系统采用的协作式调度，任务栈帧完全按需分配，以期望降低运行成本，在更资源更受限的平台中同样使用。

### 我们有什么优势？

- 无栈状态机
  > 不需要指定任务栈大小，运行时按需进行分配。让你不用再计算任务栈大小。
  > 内存占用仅比普通函数调用大一点（用于内核管理）
  >
- 协作式调度
  > 任务切换不进中断，接近零开销切换（只保存/加载FP、PC）
  > 可以实现完全无锁编程（任务切换只会发生在出让点，合理设计下可以实现完全无锁）
  >
- 高效内核
  > 空闲时自动休眠（需要短暂关中断以保证响应效率）
  > 系统调用自动合并（暂未有使用场景）
  > 内核调度完全无锁、不关中断
  >
- 高精度、大范围系统时钟
  > 只占用一个硬件定时器（如不配置硬件定时器，则时间相关功能将无法使用）
  > 提供高精度系统时钟，精度最高可达15.2ns（依赖硬件定时器频率）
  > 时钟范围约8900年
  > 时钟严格单调（除非中断被阻塞导致软件时间戳更新错误）
  > 可选无分支原子读取最高精度绝对时间戳（需要延后解析），麻麻再也不用担心我时间测量不准了
  > 支持系统时钟分辨率下任务调度（调度时间抖动由就绪任务执行时间决定），轻松实现us级调度
  >
- 中断安全
  > 提供了纯软件的中断安全原子操作接口，让你可以在无硬件支持原子操作的平台也能安全使用中断（需要注意：纯软件原子操作开销很大，应尽量少使用。采用乐观保护，高冲突时存在性能退化）
  > 可以通过部分特性将中断不安全限制到最小范围，让你更容易写出中断安全的代码
  >
- 等待机制
  > 统一的等待接口等待中断或其他事件源通知
  > 支持多条件或等待（要条件与等待的自己封装吧）
  > 任意任务数量可设超时。定时任务挂起/取消开销均摊O(longN)，N=挂起+取消。（只要处理得过来，多少任务都能一起定时而不额外占用硬件定时器）
  >

### 我们有什么问题？

- 依赖编译器行为（部分功能不保证跨编译器可用）
- 使用了部分汇编，跨平台使用需要通过目标平台ABI更换部分汇编代码才能使用
- 单核使用，无多核并发数据安全保证（内核调度器只可以运行于单个核心）
- 需要动态内存分配（虽然合理使用时只会分配一次）

## 如何使用

- **使用条件**

  - 编译器支持变量声明`__attribute__((cleanup(function)))`属性（用于实现协程函数）
  - 编译时需加入`-fno-omit-frame-pointer`（以保证协作式任务切换可用）
  - 动态内存管理接口`malloc(size_t)`和`free(intptr_t)`（用于管理协程栈帧）
- **使用示例**

  - 配置硬件定时器资源并开启事件调度
    ```C
    // 自行定义的硬件定时器配置函数
    void TimerConfig(size_t Reload, void(*OS_Irq)(void))
    {
    	htim7.Instance->ARR = Reload;
    	OS_Timer_Irq = OS_Irq;
    	HAL_TIM_Base_Start_IT(&htim7);
    }
    //按键事件分发，test1识别按键事件，更新时通过btnListener通知挂起了监听的任务
    Subject(enum btn) btnListener = {
        .Observers.Tail = (void*)&btnListener.Observers.Head,
    };
    asyncFast(void, test1, ()){
      const int BTN_DEBOUNCE_MS    = 20;
      const int BTN_LONG_MS        = 600;
      const int BTN_DOUBLE_GAP_MS  = 200;
      TimeStamp time = 16;
      TimeStamp Time_ms = 0;
      bool key_status = 0;
      char gpio;
      enum btn btnStatus = BTN_IDLE;
      AsyncWaitable_NewVal(Timer, 0);
      SetWaitObj(Timer, RetrunTrue, );

      btnListener.Data = &btnStatus;

      while(true)
      {
        bool isUpdata = false;
        time += 1 << 16;//10ms定时
        Time_ms += 1;
        AsyncWaitAtFast(Timer, time);
        gpio = !HAL_GPIO_ReadPin(KeyOnBoard_GPIO_Port, KeyOnBoard_Pin);
        if(btnStatus > BTN_TO_IDLE)
        {
          btnStatus = BTN_IDLE;
        }
        if(key_status != gpio)
        {
          Time_ms = 0;
          key_status = gpio;   // 保存本次电平
        }
        if (gpio)   // gpio == 1 表示按下
        {
            switch (btnStatus)
            {
              ……
            }
        }
        else   // gpio == 0 表示松开
        {
            switch (btnStatus)
            {
              ……
            }
        }
        if(isUpdata)
        {
          SubjectNotify(&btnListener.Observers);
        }
      }
    }
    async_end
    // 协程任务函数
    async(void, test2, ()){
    	AsyncWaitable_NewVal(Waitable, 1);
    	enum btn btnStatus = BTN_IDLE;
    	int i = 0;
    	bool LoadBtnStatus(size_t index)//内嵌函数，限制中断数据安全临界区
    	{
    		if(index == 1)
    		{
    			btnStatus = *(btnListener.Data);
    			return true;
    		}
    		return false;
    	}

    	//内嵌函数修正
    	asyncNestedFunction(LoadBtnStatus);
    	//初始化等待对象
    	SetWaitObj(Waitable, LoadBtnStatus, &btnListener);

    	for(; i < 20; i++)
    	{
    		AsyncWaitAtFast(Waitable, GetTimeStamp() + (50<<16));
    		HAL_GPIO_TogglePin(LEDonBoard_GPIO_Port, LEDonBoard_Pin);
    	}
    	while(true)
    	{
    		AsyncWaitFast(Waitable);
    		switch(btnStatus)
    		{
          ……
    		}
    	}
    }
    }//等效async_end
    //任务入口，创建多个任务流
    void Entry(void){
    	Async_RunTask(test1);
    	Async_RunTask(test2);
    }
    int main(void){
        // 初始化系统时钟
        Async_SetOSTimer(60000, (uint16_t*)&(htim7.Instance->CNT), TimerConfig, NULL);
        // 进入调度。并在开始调度后调用FirstTask作为第一个任务
        Async_Run(Entry);
    }
    ```
  - 定义协程函数（协程函数才保证局部变量在挂起点有效）
    **缺少一个`{`是因为async宏中已经包含了**
    该宏函数将使函数开始执行前/返回前执行内部的栈帧挂起/恢复函数，以实现完全以C语言语法进行调用/返回
    ```C
    //定义异步函数宏函数async(ret, functionName, Params)
    async(void, test2, ()){
    	AsyncWaitable_NewVal(Waitable, 1);
    	enum btn btnStatus = BTN_IDLE;
    	int i = 0;
    	bool LoadBtnStatus(size_t index)//内嵌函数，限制中断数据安全临界区
    	{
    		if(index == 1)
    		{
    			btnStatus = *(btnListener.Data);
    			return true;
    		}
    		return false;
    	}

    	//内嵌函数修正
    	asyncNestedFunction(LoadBtnStatus);
    	//初始化等待对象
    	SetWaitObj(Waitable, LoadBtnStatus, &btnListener);

    	for(; i < 20; i++)
    	{
    		AsyncWaitAtFast(Waitable, GetTimeStamp() + (50<<16));
    		HAL_GPIO_TogglePin(LEDonBoard_GPIO_Port, LEDonBoard_Pin);
    	}
    	while(true)
    	{
    		AsyncWaitFast(Waitable);
    		switch(btnStatus)
    		{
          ……
    		}
    	}
    }
    }//等效async_end
    ```
  - 创建不等待的结果的新任务
    ```C
    //仅作为入口开启多个任务
    void Entry(void){
      Async_RunTask(test1);
      Async_RunTask(test2);
    }
    //栈上协程创建一个新的栈上协程，并持续执行某任务
    asyncFast(void, AsyncMain,(void))
    {
        Async_RunTaskFast(test1);
      while(1){
        ……
      }
    }
    async_end
    ```
- ### **注意事项**


  > 由于C语言无原生协程支持，本库的协程实现与一般协程不完全相同。因此需要注意以下几点
  >

  - 协程（可挂起）函数定义
    与原生支持异步函数的语言不同，C语言不支持async关键字。因此使用宏函数实现可挂起函数定义。  
    部分功能只能在协程函数中使用，部分功能在协程函数中使用以获得最优性能。
  - 协程函数中内嵌函数定义
    因为协程执行时内嵌函数的跳板随栈帧被搬移，所以需要修正跳板中加载的函数栈帧指针。  
    内嵌函数作为唤醒条件判断在事件源中使用能实现很好的封装。
  - 普通函数禁止传递局部变量指针穿过协程函数
    普通函数调用协程函数后其栈帧可能随协程函数栈帧转移，导致原局部变量地址不可用。
  - 性能与安全
    大量接口定义了普通版和快速版。普通版在更多场景下可以使用而快速版只拥有核心功能而缺少条件检测。使用时请明确条件再使用。下面列出部分差异
    - `async` VS `asyncFast`: async版本使用`malloc`分配协程栈帧；asyncFast版本协程函数使用栈上分配，应该保证LIFO。
    - `Yield` VS `YieldFast`: Yield版本YieldFast版本需要在协程函数中使用，否则可能跑飞。
    - `AsyncWait[At/For]` VS `AsyncWait[At/For]Fast`: 同上。
  - 不可忽略内联失败告警
    存在大量强制内联`__attribute__((always_inline))`函数。这些函数需要将栈帧内联到调用者栈帧才能正常使用。如果内联失败会导致创建独立栈帧会导致运行行为错误。


