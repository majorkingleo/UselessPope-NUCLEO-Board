#pragma once
#ifndef OS_HPP_INCLUDED
#define OS_HPP_INCLUDED

#include <chrono>
#include <cstdint>
#include <wlib.hpp>

#include <CpputilsDebug.h>
#include <static_format.h>

namespace os
{
  namespace internal
  {
    using stack_t = uint64_t;
    constexpr uint32_t calculate_depth(uint32_t size_in_byte) { return (size_in_byte + (sizeof(stack_t) - 1)) / sizeof(stack_t); }

    bool is_isr() noexcept;
    void delay_until(std::chrono::steady_clock::time_point const& time_point) noexcept;

    class semaphore
    {
    public:
      semaphore(uint16_t max_count, uint16_t init_value);
      virtual ~semaphore();

      void release();

      void acquire();

      bool try_acquire();

      template <class Rep, class Period> bool try_acquire_for(const std::chrono::duration<Rep, Period>& rel_time) {
    	  return try_acquire_for_ms( std::chrono::duration_cast<std::chrono::milliseconds>(rel_time) );
      }

      template <class Clock, class Duration> bool try_acquire_until(const std::chrono::time_point<Clock, Duration>& abs_time);

    private:
      bool try_acquire_for_ms( const std::chrono::milliseconds rel_time );

    private:
      using stack_t                                        = internal::stack_t;
      static constexpr uint32_t control_block_size_in_byte = 88;

      void*   m_handle = nullptr;
      stack_t m_mem_cb[internal::calculate_depth(control_block_size_in_byte)]{};
    };
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

    /*
     * @return The task's notification count before it is either cleared to zero or
     * decremented (see the xClearCountOnExit parameter).
     *
     * 0   : on timeout
     * > 0 : amount of notifications by other tasks
     */
    uint32_t wait_for_notify();

    uint32_t try_wait_for_notify_for(const std::chrono::nanoseconds& timeout);

    template <class Rep, class Period> uint32_t try_wait_for_notify_for(const std::chrono::duration<Rep, Period>& timeout)
    {
      return try_wait_for_notify_for(std::chrono::duration_cast<std::chrono::nanoseconds>(timeout));
    }

    template <class Clock, class Duration> uint32_t try_wait_for_notify_until(const std::chrono::time_point<Clock, Duration>& abs_time)
    {
      auto now = Clock::now();
      if (abs_time < now)
        return try_wait_for_notify_for(std::chrono::steady_clock::duration(0));
      return try_wait_for_notify_for(abs_time - now);
    }

    //void get_info();
    void print_info(char* buffer, std::size_t len);

    /**
     * returns true as long as the thread should run
     */
    constexpr inline bool keep_running() { return true; }

  }    // namespace this_thread

  template <uint16_t LeastMaxValue = std::numeric_limits<uint16_t>::max()> class counting_semaphore: public internal::semaphore
  {
  public:
    counting_semaphore(uint16_t init_value)
        : semaphore(LeastMaxValue, init_value) {};

    template <class Rep, class Period> bool try_acquire_for(const std::chrono::duration<Rep, Period>& rel_time)  {
    	return internal::semaphore::try_acquire_for(rel_time);
    }

    template <class Clock, class Duration> bool try_acquire_until(const std::chrono::time_point<Clock, Duration>& abs_time);
  };

  using binary_semaphore = counting_semaphore<1>;

  class Task_Interface
  {
  public:
    enum class Priority : uint32_t
    {
      idle    = 0,
      low     = 1,
      normal  = 2,
      high    = 3,
      highest = 4,
    };

    struct task_info_t
    {
      char const* name;
      uint32_t    stack_left;
    };

    Task_Interface();
    Task_Interface(Task_Interface const&)            = delete;
    Task_Interface(Task_Interface&&)                 = delete;
    Task_Interface& operator=(Task_Interface const&) = delete;
    Task_Interface& operator=(Task_Interface&&)      = delete;
    virtual ~Task_Interface();

    virtual void start() = 0;

    void notify();

    task_info_t get_info();

  protected:
    bool create_task(void (&)(void*), char const*, uint32_t const& stack_size_in_byte, void*, uint32_t const&, uint64_t* stack_begin, uint64_t* tcb_begin);
    void delete_task();

  private:
    void* m_handle = nullptr;
  };

  template <std::size_t N> class Static_Task;

  template <> class Static_Task<0>: public Task_Interface
  {
  protected:
    using stack_t = internal::stack_t;

  private:
    static constexpr uint32_t control_block_size_in_byte = 88;

  public:
    Static_Task(std::span<stack_t> stack_span, const char* name, Task_Interface::Priority const& prio = Task_Interface::Priority::idle)
        : m_stack_span(stack_span)
        , m_name(name)
        , m_prio(prio)
    {
    }

    virtual ~Static_Task() { this->delete_task(); }

    void start() override
    {
      this->create_task(Static_Task::handle, this->m_name, this->m_stack_span.size_bytes(), this, static_cast<uint32_t>(this->m_prio),
                        this->m_stack_span.data(), &this->m_mem_tcb[0]);
    }

  private:
    static void handle(void* obj)
    {
      Static_Task& self = *reinterpret_cast<Static_Task*>(obj);
      try
      {
        self.process();
      }
	  catch( const std::exception & error ) {
			CPPDEBUG( Tools::static_format<100>("Task: '%s' exception caught: '%s'", self.m_name, error.what() ) );
			while (true)
			{
			}
	  }
	  catch (...)
	  {
		CPPDEBUG( Tools::static_format<100>("Task: '%s' unknown exception caught", self.m_name ) );
		while (true)
		{
		}
	  }

      self.delete_task();
    }

    virtual void process() = 0;

    stack_t            m_mem_tcb[internal::calculate_depth(control_block_size_in_byte)] = {};
    std::span<stack_t> m_stack_span                                                     = {};
    const char*        m_name                                                           = nullptr;
    Priority           m_prio                                                           = Priority::idle;
  };

  template <std::size_t N> class Static_Task: public Task_Interface
  {
  protected:
    using stack_t = internal::stack_t;

  private:
    static constexpr uint32_t control_block_size_in_byte = 88;
    static constexpr uint32_t stack_size_in_byte         = N;
    static constexpr uint32_t calculate_depth(uint32_t size_in_byte) { return (size_in_byte + (sizeof(stack_t) - 1)) / sizeof(stack_t); }

  public:
    Static_Task(const char* name, Task_Interface::Priority const& prio = Task_Interface::Priority::idle)
        : m_name(name)
        , m_prio(prio)
    {
    }
    virtual ~Static_Task() { this->delete_task(); }

    void start() override
    {
      this->create_task(Static_Task::handle, this->m_name, this->stack_size_in_byte, this, static_cast<uint32_t>(this->m_prio), &this->m_mem_stack[0],
                        &this->m_mem_tcb[0]);
    }

  private:
    static void handle(void* obj)
    {
      Static_Task& self = *reinterpret_cast<Static_Task*>(obj);
      try
      {
        self.process();
      }
      catch( const std::exception & error ) {
			CPPDEBUG( Tools::static_format<100>("Task: '%s' exception caught: '%s'", self.m_name, error.what() ) );
			while (true)
			{
			}
	  }
	  catch (...)
	  {
		CPPDEBUG( Tools::static_format<100>("Task: '%s' unknown exception caught", self.m_name ) );
		while (true)
		{
		}
	  }

      self.delete_task();
    }

    virtual void process() = 0;

    stack_t     m_mem_tcb[calculate_depth(control_block_size_in_byte)] = {};
    stack_t     m_mem_stack[calculate_depth(stack_size_in_byte)]       = {};
    const char* m_name                                                 = nullptr;
    Priority    m_prio                                                 = Priority::idle;
  };

  template <std::size_t N> class Static_CallbackTask;

  template <> class Static_CallbackTask<0>: public Static_Task<0>
  {
  public:
    Static_CallbackTask(wlib::Callback<void()>&         callback,
                        std::span<stack_t>              stack_span,
                        char const*                     name,
                        Task_Interface::Priority const& prio = Task_Interface::Priority::idle)
        : Static_Task(stack_span, name, prio)
        , m_process_callback(callback)
    {
    }

  private:
    virtual void process() override { return this->m_process_callback(); };

    wlib::Callback<void()>& m_process_callback;
  };

  template <std::size_t N> class Static_CallbackTask: public Static_Task<N>
  {
  public:
    Static_CallbackTask(wlib::Callback<void()>& callback, char const* name, Task_Interface::Priority const& prio = Task_Interface::Priority::idle)
        : Static_Task<N>(name, prio)
        , m_process_callback(callback)
    {
    }

  private:
    virtual void process() override { return this->m_process_callback(); };

    wlib::Callback<void()>& m_process_callback;
  };

  template <typename T, std::size_t N> class Static_MemberfunctionCallbackTask;

  template <typename T> class Static_MemberfunctionCallbackTask<T, 0>: public Static_Task<0>
  {
    using mem_fnc_t = void (T::*)();

  public:
    Static_MemberfunctionCallbackTask(T&                              obj,
                                      mem_fnc_t                       mem_fnc,
                                      std::span<stack_t>              stack_span,
                                      char const*                     name,
                                      Task_Interface::Priority const& prio = Task_Interface::Priority::idle)
        : Static_Task(stack_span, name, prio)
        , m_callback{ obj, mem_fnc }
    {
    }

  private:
    virtual void process() override { return this->m_callback(); };

    wlib::Memberfunction_Callback<T, void()> m_callback;
  };

  template <typename T, std::size_t N> class Static_MemberfunctionCallbackTask: public Static_Task<N>
  {
    using mem_fnc_t = void (T::*)();

  public:
    Static_MemberfunctionCallbackTask(T&                              obj,
                                      mem_fnc_t                       mem_fnc,
                                      char const*                     name,
                                      Task_Interface::Priority const& prio = Task_Interface::Priority::idle)
        : Static_Task<N>(name, prio)
        , m_callback{ obj, mem_fnc }
    {
    }

  private:
    virtual void process() override { return this->m_callback(); };

    wlib::Memberfunction_Callback<T, void()> m_callback;
  };

  class mutex
  {
  public:
    mutex();
    ~mutex();

    void lock();
    bool try_lock();
    void unlock();

  private:
    using stack_t                                        = internal::stack_t;
    static constexpr uint32_t control_block_size_in_byte = 88;

    void*   m_handle = nullptr;
    stack_t m_mem_cb[internal::calculate_depth(control_block_size_in_byte)]{};
  };

  class recursive_mutex
  {
  public:
    recursive_mutex();
    ~recursive_mutex();

    void lock();
    bool try_lock();
    void unlock();

  private:
    using stack_t                                        = internal::stack_t;
    static constexpr uint32_t control_block_size_in_byte = 88;

    void*   m_handle = nullptr;
    stack_t m_mem_cb[internal::calculate_depth(control_block_size_in_byte)]{};
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

  inline void quit( [[maybe_unused]] int exit_code ) {};

}    // namespace os

namespace os
{
  void print_info(char* buffer, std::size_t len);

  // true if os is already started
  bool running();
}
#endif
