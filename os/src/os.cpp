#include <atomic>
#include <chrono>
#include <os.hpp>
#include <stm32h753xx.h>

//
#include <FreeRTOS.h>
#include <semphr.h>
#include <task.h>

namespace
{
  std::atomic<uint32_t> chrono_val_low  = 0;
  std::atomic<uint32_t> chrono_val_high = 0;
}    // namespace

std::chrono::steady_clock::time_point std::chrono::steady_clock::now() noexcept
{
  using T = std::chrono::steady_clock;
  while (true)
  {
    uint32_t const h   = chrono_val_high;
    uint32_t const l   = chrono_val_low;
    uint32_t const sub = ((480'000 - SysTick->VAL) * 1'000) / 480;

    if (l == chrono_val_low.load() && h == chrono_val_high.load())
    {
      uint64_t const val = (((static_cast<uint64_t>(h) << 32) + static_cast<uint64_t>(l)) * 1'000'000) + sub;
      return T::time_point(T::duration(std::chrono::nanoseconds(val)));
    }
  }
}

std::chrono::system_clock::time_point std::chrono::system_clock::now() noexcept
{
  using T = std::chrono::system_clock;
  return T::time_point(T::duration(std::chrono::nanoseconds(std::chrono::steady_clock::now().time_since_epoch().count())));
}

extern "C" void vAssertCalled([[maybe_unused]] uint32_t ulLine, [[maybe_unused]] const char* pcFile)
{
  __asm("bkpt 255");
  __asm("bx lr");
}

extern "C" void vApplicationStackOverflowHook([[maybe_unused]] TaskHandle_t xTask, [[maybe_unused]] char* pcTaskName)
{
  __asm("bkpt 255");
  __asm("bx lr");
}

extern "C" void vApplicationMallocFailedHook()
{
  __asm("bkpt 255");
  __asm("bx lr");
}

extern "C" void vApplicationTickHook()
{
  uint32_t const l_new = chrono_val_low + 1;
  chrono_val_low       = l_new;
  if (l_new == 0)
    chrono_val_high++;
}

/* configSUPPORT_STATIC_ALLOCATION is set to 1, so the application must provide an
   implementation of vApplicationGetIdleTaskMemory() to provide the memory that is
   used by the Idle task. */
extern "C" void vApplicationGetIdleTaskMemory(StaticTask_t** ppxIdleTaskTCBBuffer, StackType_t** ppxIdleTaskStackBuffer, uint32_t* pulIdleTaskStackSize)
{
  /* If the buffers to be provided to the Idle task are declared inside this
     function then they must be declared static - otherwise they will be allocated on
     the stack and so not exists after this function exits. */
  static StaticTask_t xIdleTaskTCB;
  static StackType_t  uxIdleTaskStack[configMINIMAL_STACK_SIZE];

  /* Pass out a pointer to the StaticTask_t structure in which the Idle task's
     state will be stored. */
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;

  /* Pass out the array that will be used as the Idle task's stack. */
  *ppxIdleTaskStackBuffer = uxIdleTaskStack;

  /* Pass out the size of the array pointed to by *ppxIdleTaskStackBuffer.
     Note that, as the array is necessarily of type StackType_t,
     configMINIMAL_STACK_SIZE is specified in words, not bytes. */
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

/*-----------------------------------------------------------*/

/* configSUPPORT_STATIC_ALLOCATION and configUSE_TIMERS are both set to 1, so the
   application must provide an implementation of vApplicationGetTimerTaskMemory()
   to provide the memory that is used by the Timer service task. */
extern "C" void vApplicationGetTimerTaskMemory(StaticTask_t** ppxTimerTaskTCBBuffer, StackType_t** ppxTimerTaskStackBuffer, uint32_t* pulTimerTaskStackSize)
{
  /* If the buffers to be provided to the Timer task are declared inside this
     function then they must be declared static - otherwise they will be allocated on
     the stack and so not exists after this function exits. */
  static StaticTask_t xTimerTaskTCB;
  static StackType_t  uxTimerTaskStack[configTIMER_TASK_STACK_DEPTH];

  /* Pass out a pointer to the StaticTask_t structure in which the Timer
     task's state will be stored. */
  *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;

  /* Pass out the array that will be used as the Timer task's stack. */
  *ppxTimerTaskStackBuffer = uxTimerTaskStack;

  /* Pass out the size of the array pointed to by *ppxTimerTaskStackBuffer.
     Note that, as the array is necessarily of type StackType_t,
    configTIMER_TASK_STACK_DEPTH is specified in words, not bytes. */
  *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}

namespace os::internal
{
  bool is_isr() noexcept { return (SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk) != 0; }

  void delay_until(std::chrono::steady_clock::time_point const& time_point) noexcept
  {
    constexpr int64_t max_time = portMAX_DELAY - 1;
    int64_t           wait_time =
        std::chrono::duration_cast<std::chrono::milliseconds>((time_point - std::chrono::steady_clock::now()) + std::chrono::milliseconds(1)).count();
    while (wait_time >= max_time)
    {
      vTaskDelay(max_time);
      wait_time = std::chrono::duration_cast<std::chrono::milliseconds>((time_point - std::chrono::steady_clock::now()) + std::chrono::milliseconds(1)).count();
    }
    vTaskDelay(wait_time);
  }

  semaphore::semaphore(uint16_t max_count, uint16_t init_value)
  {
    static_assert(alignof(StaticQueue_t) <= alignof(stack_t));
    static_assert(sizeof(StaticQueue_t) <= sizeof(this->m_mem_cb));

    QueueHandle_t sem_handle = xSemaphoreCreateCountingStatic(max_count, init_value, reinterpret_cast<StaticQueue_t*>(&this->m_mem_cb[0]));

    if (sem_handle == nullptr)
      return;    // TODO ex
    this->m_handle = sem_handle;
  }

  semaphore::~semaphore() {}

  void semaphore::release()
  {
    if (internal::is_isr())
    {
      BaseType_t xHigherPriorityTaskWoken = pdFALSE;
      xSemaphoreGiveFromISR(this->m_handle, &xHigherPriorityTaskWoken);
      portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
    else
    {
      xSemaphoreGive(this->m_handle);
    }
  }
  void semaphore::acquire() { xSemaphoreTake((QueueHandle_t)this->m_handle, portMAX_DELAY); }
  bool semaphore::try_acquire() { return xSemaphoreTake((QueueHandle_t)this->m_handle, pdMS_TO_TICKS(0)) == pdTRUE; }

  bool semaphore::try_acquire_for_ms( const std::chrono::milliseconds rel_time )
  {
    int64_t val = rel_time.count();
    constexpr int64_t max_time = portMAX_DELAY - 1;


    while (val > max_time)
    {
      val -= max_time;
      if (xSemaphoreTake((QueueHandle_t)this->m_handle, pdMS_TO_TICKS(max_time)) == pdTRUE)
        return true;
    }
    return xSemaphoreTake((QueueHandle_t)this->m_handle, pdMS_TO_TICKS(val)) == pdTRUE;
  }
}    // namespace os::internal

namespace os
{
  mutex::mutex()
  {
    static_assert(alignof(StaticQueue_t) <= alignof(stack_t));
    static_assert(sizeof(StaticQueue_t) <= sizeof(this->m_mem_cb));

    QueueHandle_t sem_handle = xSemaphoreCreateMutexStatic(reinterpret_cast<StaticQueue_t*>(&this->m_mem_cb[0]));
    if (sem_handle == nullptr)
      return;    // TODO ex
    this->m_handle = sem_handle;
  }

  mutex::~mutex() {}

  void mutex::lock() { xSemaphoreTake((QueueHandle_t)this->m_handle, portMAX_DELAY); }
  bool mutex::try_lock() { return xSemaphoreTake((QueueHandle_t)this->m_handle, pdMS_TO_TICKS(0)) == pdTRUE; }
  void mutex::unlock() { xSemaphoreGive((QueueHandle_t)this->m_handle); }

  recursive_mutex::recursive_mutex()
  {
    static_assert(alignof(StaticQueue_t) <= alignof(stack_t));
    static_assert(sizeof(StaticQueue_t) <= sizeof(this->m_mem_cb));

    QueueHandle_t sem_handle = xSemaphoreCreateRecursiveMutexStatic(reinterpret_cast<StaticQueue_t*>(&this->m_mem_cb[0]));
    if (sem_handle == nullptr)
      return;    // TODO ex
    this->m_handle = sem_handle;
  }
  recursive_mutex::~recursive_mutex() {}

  void recursive_mutex::lock() { xSemaphoreTakeRecursive((QueueHandle_t)this->m_handle, portMAX_DELAY); }
  bool recursive_mutex::try_lock() { return xSemaphoreTakeRecursive((QueueHandle_t)this->m_handle, pdMS_TO_TICKS(0)) == pdTRUE; }
  void recursive_mutex::unlock() { xSemaphoreGiveRecursive((QueueHandle_t)this->m_handle); }

}    // namespace os

namespace os::this_thread
{
  void     yield() noexcept { taskYIELD(); }
  uint32_t wait_for_notify() { return ulTaskNotifyTake(pdTRUE, portMAX_DELAY); }
  uint32_t try_wait_for_notify_for(std::chrono::nanoseconds const& delay)
  {
    constexpr int64_t max_time = portMAX_DELAY - 1;

    int64_t wait_time = std::chrono::duration_cast<std::chrono::milliseconds>(delay + std::chrono::milliseconds(1)).count();
    while (wait_time > max_time)
    {
      uint32_t val = ulTaskNotifyTake(pdTRUE, max_time);
      if (val != 0)
        return val;
      wait_time -= max_time;
    }
    return ulTaskNotifyTake(pdTRUE, wait_time);
  }

  TaskStatus_t get_info()
  {
    TaskStatus_t status;
    vTaskGetInfo(NULL, &status, pdTRUE, eInvalid);
    return status;
  }
  void print_info(char* buffer, std::size_t len)
  {
    auto        info = get_info();
    std::size_t pos  = 0;
    pos += snprintf(buffer, len - pos, "## TASKINFO NAME:%s; STACKHW:%u\n", info.pcTaskName, info.usStackHighWaterMark);
  }

}    // namespace os::this_thread

namespace
{
  os::mutex                           task_list_mtex;
  std::array<os::Task_Interface*, 30> task_list;

  bool register_task(os::Task_Interface& task)
  {
    os::lock_guard l{ task_list_mtex };
    for (auto& el : task_list)
    {
      if (el != nullptr)
        continue;

      el = &task;
      return true;
    }
    return false;
  }
  void unregister_task(os::Task_Interface& task)
  {
    os::lock_guard l{ task_list_mtex };
    for (auto& el : task_list)
    {
      if (el != &task)
        continue;

      el = nullptr;
      return;
    }
  }
}    // namespace

namespace os
{
  void Task_Interface::notify()
  {
    if (internal::is_isr())
    {
      BaseType_t xHigherPriorityTaskWoken = pdFALSE;
      vTaskNotifyGiveFromISR((TaskHandle_t)this->m_handle, &xHigherPriorityTaskWoken);
      portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
    else
    {
      xTaskNotifyGive((TaskHandle_t)this->m_handle);
    }
  }

  Task_Interface::Task_Interface() { register_task(*this); }

  Task_Interface::~Task_Interface() { unregister_task(*this); }

  void print_info(char* buffer, std::size_t len)
  {
    os::lock_guard l{ task_list_mtex };
    char*          pos = buffer;
    char*          end = buffer + len;
    for (auto el : task_list)
    {
      if (el == nullptr)
        continue;

      auto info = el->get_info();
      pos += snprintf(pos, end - pos, "## TASKINFO NAME:%s; STACKHW:%lu\n", info.name, info.stack_left);
    }
  }

  Task_Interface::task_info_t Task_Interface::get_info()
  {
    TaskStatus_t status;
    vTaskGetInfo(reinterpret_cast <TaskHandle_t>(this->m_handle), &status, pdTRUE, eInvalid);
    return task_info_t{ .name{ status.pcTaskName }, .stack_left{ status.usStackHighWaterMark * sizeof(uint32_t) } };
  }

  bool Task_Interface::create_task(void (&handle)(void*),
                                   char const*     name,
                                   uint32_t const& stack_size_in_byte,
                                   void*           obj,
                                   uint32_t const& prio,
                                   uint64_t*       stack_begin,
                                   uint64_t*       control_block)
  {
    if (this->m_handle != nullptr)
      return false;

    static_assert(alignof(StackType_t) <= alignof(uint64_t));
    static_assert(alignof(StaticTask_t) <= alignof(uint64_t));
    TaskHandle_t task_handle = xTaskCreateStatic(handle, name, stack_size_in_byte / sizeof(StackType_t), obj, prio, reinterpret_cast<StackType_t*>(stack_begin),
                                                 reinterpret_cast<StaticTask_t*>(control_block));
    if (task_handle == nullptr)
      return false;

    this->m_handle = task_handle;
    return true;
  }
  void Task_Interface::delete_task()
  {
    if (this->m_handle == nullptr)
      return;
    vTaskDelete(reinterpret_cast<TaskHandle_t>(this->m_handle));
  }

  bool running()
  {
	  return xTaskGetSchedulerState() == taskSCHEDULER_RUNNING;
  }
}    // namespace os
