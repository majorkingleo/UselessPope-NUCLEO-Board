#pragma once
#ifndef WLIB_OS_HPP
#define WLIB_OS_HPP

#include <FreeRTOS.h>
#include <chrono>
#include <cstddef>
#include <limits>
#include <semphr.h>
#include <task.h>
#include <wlib_callback.hpp>

namespace os
{
  class Task
  {
    static void  task_handle(void* obj);
    static void* create_task(char const* name, uint32_t const& min_stack_size_bytes, Task& task, int16_t prio = 0);

  public:
    template <typename... Args> class handle_t final: public wlib::Callback<void()>
    {
      void                (&m_fnc)(Args...);
      std::tuple<Args...> m_param;

    public:
      handle_t(void (&fnc)(Args...), std::tuple<Args...> args)
          : m_fnc(fnc)
          , m_param(args)
      {
      }

      void operator()() override { return std::apply(this->m_fnc, this->m_param); }
    };

    template <typename... Args> Task(char const* name, uint32_t const& min_stack_size_bytes, void (&handle)(Args...), std::tuple<Args...> args)
    {
      static_assert(sizeof(handle_t<Args...>) < sizeof(m_obj_mem));

      if (new (&this->m_obj_mem[0]) handle_t<Args...>(handle, args) == nullptr)
      {
        // TODO: error
      }
      this->handle = create_task(name, min_stack_size_bytes, *this);
    }

    Task(char const* name, uint32_t const& min_stack_size_bytes, void (&handle)())
    {
      if (new (&this->m_obj_mem[0]) wlib::Function_Callback<void()>(handle) == nullptr)
      {
        // TODO: error
      }
      this->handle = create_task(name, min_stack_size_bytes, *this);
    }

    template <typename T> Task(char const* name, uint32_t const& min_stack_size_bytes, wlib::Memberfunction_Callback<T, void()>& handle, int16_t prio = 0)
    {
      if (new (&this->m_obj_mem[0]) wlib::Memberfunction_Callback<T, void()>(handle) == nullptr)
      {
        // TODO: error
      }
      this->handle = create_task(name, min_stack_size_bytes, *this, prio);
    }

  private:
    void*    handle = nullptr;
    uint32_t m_obj_mem[10]{};
  };

  namespace internal
  {
    bool is_isr() noexcept;
    void delay_until(std::chrono::steady_clock::time_point const& time_point) noexcept;
  }    // namespace internal

  namespace this_thread
  {
    void yield() noexcept;

    template <class Rep, class Period> void sleep_for(const std::chrono::duration<Rep, Period>& sleep_duration)
    {
      return internal::delay_until(std::chrono::steady_clock::now() + sleep_duration);
    }
    template <class Clock, class Duration> void sleep_until(const std::chrono::time_point<Clock, Duration>& sleep_time)
    {
      return internal::delay_until(std::chrono::steady_clock::now() + (sleep_time - Clock::now()));
    }
  }    // namespace this_thread

  template <uint16_t LeastMaxValue = std::numeric_limits<uint16_t>::max()> class counting_semaphore
  {
  public:
    counting_semaphore(uint16_t init_value)
        : m_handle(xSemaphoreCreateCounting(LeastMaxValue, init_value))
    {
    }

    ~counting_semaphore() { vSemaphoreDelete(this->m_handle); }

    void release()
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

    void acquire() { xSemaphoreTake((QueueHandle_t)this->m_handle, portMAX_DELAY); }

    bool try_acquire() { return xSemaphoreTake((QueueHandle_t)this->m_handle, pdMS_TO_TICKS(0)) == pdTRUE; }

    template <class Rep, class Period> bool try_acquire_for(const std::chrono::duration<Rep, Period>& rel_time)
    {
      int64_t val = std::chrono::duration_cast<std::chrono::milliseconds>(rel_time).count();
      constexpr int64_t max_time = portMAX_DELAY - 1;
      
      
      while (val > max_time)
      {
        val -= max_time;
        if (xSemaphoreTake((QueueHandle_t)this->m_handle, pdMS_TO_TICKS(max_time)) == pdTRUE)
          return true;
      }
      return xSemaphoreTake((QueueHandle_t)this->m_handle, pdMS_TO_TICKS(val)) == pdTRUE;
    }

    template <class Clock, class Duration> bool try_acquire_until(const std::chrono::time_point<Clock, Duration>& abs_time);

  private:
    void* m_handle{};
  };

  using binary_semaphore = counting_semaphore<1>;

  class mutex
  {
  public:
    mutex()
        : m_handle(xSemaphoreCreateMutex())
    {
    }
    ~mutex() { vSemaphoreDelete(this->m_handle); }

    void lock() { xSemaphoreTake((QueueHandle_t)this->m_handle, portMAX_DELAY); }
    bool try_lock() { return xSemaphoreTake((QueueHandle_t)this->m_handle, pdMS_TO_TICKS(0)) == pdTRUE; }
    void unlock() { xSemaphoreGive(this->m_handle); }

  private:
    void* m_handle = {};
  };

  class recursive_mutex
  {
  public:
    recursive_mutex()
        : m_handle(xSemaphoreCreateRecursiveMutex())
    {
    }
    ~recursive_mutex() { vSemaphoreDelete(this->m_handle); }

    void lock() { xSemaphoreTakeRecursive((QueueHandle_t)this->m_handle, portMAX_DELAY); }
    bool try_lock() { return xSemaphoreTakeRecursive((QueueHandle_t)this->m_handle, pdMS_TO_TICKS(0)) == pdTRUE; }
    void unlock() { xSemaphoreGiveRecursive((QueueHandle_t)this->m_handle); }

  private:
    void* m_handle = {};
  };

  template <typename T> class lock_guard
  {
  public:
    lock_guard(T& mutex)
        : m_tex{ mutex }
    {
      this->m_tex.lock();
    }

    lock_guard(lock_guard const&)            = delete;
    lock_guard(lock_guard&&)                 = delete;
    lock_guard& operator=(lock_guard const&) = delete;
    lock_guard& operator=(lock_guard&&)      = delete;

    ~lock_guard() { this->m_tex.unlock(); }

  private:
    T& m_tex;
  };
}    // namespace os

#endif
