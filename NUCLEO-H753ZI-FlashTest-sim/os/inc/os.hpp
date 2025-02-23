#pragma once


#include <chrono>
#include <cstdint>
#include <wlib.hpp>

#include <thread>
#include <semaphore>
#include <mutex>
#include <functional>
#include <static_format.h>

namespace os {

	namespace internal
	{
		using stack_t = uint64_t;
		constexpr uint32_t calculate_depth(uint32_t size_in_byte) { return (size_in_byte + (sizeof(stack_t) - 1)) / sizeof(stack_t); }
	}


	/////////////////////////////////////////////
	// this_thread
	/////////////////////////////////////////////

	namespace this_thread {

		inline void yield() {
			std::this_thread::yield();
		}

		template<typename _Rep, typename _Period>
		void sleep_for( const std::chrono::duration<_Rep,_Period> & dur ) {
			std::this_thread::sleep_for(dur);
		}

	    template <class Clock, class Duration> void sleep_until(const std::chrono::time_point<Clock, Duration>& sleep_time)
	    {
	    	if( sleep_time < std::chrono::steady_clock::now() ) {
	    		return;
	    	}

	      return std::this_thread::sleep_for(sleep_time - Clock::now());
	    }

	    /*
	     * @return The task's notification count before it is either cleared to zero or
	     * decremented (see the xClearCountOnExit parameter).
	     *
	     * 0   : on timeout
	     * > 0 : amount of notifications by other tasks
	     */
	    uint32_t wait_for_notify();


	    /* returns:
	     *   0 : on timeout
	     * > 0 : amount of notifications by other tasks
	     */
	    uint32_t try_wait_for_notify_for(std::chrono::nanoseconds const& delay);

	    /* returns:
	     *   0 : on timeout
	     * > 0 : amount of notifications by other tasks
	     */
	    template <class Rep, class Period> uint32_t try_wait_for_notify_for(const std::chrono::duration<Rep, Period>& rel_time)
	    {
	      return try_wait_for_notify_for(std::chrono::duration_cast<std::chrono::nanoseconds>(rel_time));
	    }


	    /* returns:
	     *   0 : on timeout
	     * > 0 : amount of notifications by other tasks
	     */
	    template <class Clock, class Duration> uint32_t try_wait_for_notify_until(const std::chrono::time_point<Clock, Duration>& abs_time)
	    {
	      auto now = Clock::now();
	      if (abs_time < now)
	        return try_wait_for_notify_for(std::chrono::steady_clock::duration(0));
	      return try_wait_for_notify_for(abs_time - now);
	    }

	    /**
	     * returns true as long as the thread should run
	     */
	    bool keep_running();

	} // namespace os:this_thread


	/////////////////////////////////////////////
	// counting_semaphore
	/////////////////////////////////////////////

	template <uint16_t LeastMaxValue = std::numeric_limits<uint16_t>::max()>
	class counting_semaphore : public std::counting_semaphore<LeastMaxValue>
	{
	public:
		using base = std::counting_semaphore<LeastMaxValue>;
		using base::base;

	};

	/////////////////////////////////////////////
	// binary_semaphore
	/////////////////////////////////////////////

	using binary_semaphore = counting_semaphore<1>;




	/////////////////////////////////////////////
	// mutex
	/////////////////////////////////////////////

	using mutex = std::mutex;



	/////////////////////////////////////////////
	// lock_guard
	/////////////////////////////////////////////

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


	/////////////////////////////////////////////
	// Task_Interface
	/////////////////////////////////////////////

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

	private:
		std::thread::id id;

	public:
		Task_Interface()                                 = default;
		Task_Interface(Task_Interface const&)            = delete;
		Task_Interface(Task_Interface&&)                 = delete;
		Task_Interface& operator=(Task_Interface const&) = delete;
		Task_Interface& operator=(Task_Interface&&)      = delete;
		virtual ~Task_Interface();

	    virtual void start() = 0;

	    void notify();

	    void join();

	protected:

	    //bool create_task(void (&)(void*), char const*, uint32_t const& stack_size_in_byte, void*, uint32_t const&, uint64_t* stack_begin, uint64_t* tcb_begin);
	    void delete_task();

	    void register_task( std::thread & handle, std::thread::id id, const char *name );
	    void task_ended( std::thread::id id );

	    bool switch_to_user_defined_stack( Task_Interface *thread, std::span<std::byte> m_stack_span );
	    static void entry_point_switched_stacks( Task_Interface *thread );

#ifdef _WIN32
	    static void entry_point_switched_stacks_win32();
#endif

	    virtual void call_process() = 0;
	  private:

	};

	/////////////////////////////////////////////
	// Static_Task<0>
	/////////////////////////////////////////////

	template <std::size_t N> class Static_Task;

	  template <> class Static_Task<0>: public Task_Interface
	  {
	  protected:
		using stack_t = internal::stack_t;

	  private:
		static constexpr uint32_t control_block_size_in_byte = 88;
		std::thread handle;

	  public:
		Static_Task(std::span<stack_t> stack_span, const char* name, Task_Interface::Priority const& prio = Task_Interface::Priority::idle)
			: m_stack_span(stack_span)
			, m_name(name)
			, m_prio(prio)
		{
		}

		virtual ~Static_Task() {
			this->join();
			this->delete_task();
		}

		void start() override
		{
			handle = std::thread([this]() {
				try
				  {
					register_task( handle, std::this_thread::get_id(), m_name );

					auto stack = std::span<std::byte>(reinterpret_cast<std::byte*>(m_stack_span.data()),m_stack_span.size_bytes());

					if( !switch_to_user_defined_stack(this,stack) ) {
						process();
						task_ended( std::this_thread::get_id() );
					}
				  }
				  catch( const std::exception & error ) {
						CPPDEBUG( Tools::static_format<100>("Task: '%s' exception caught: '%s'", this->m_name, error.what() ) );
						while (true)
						{
						}
				  }
				  catch (...)
				  {
					CPPDEBUG( Tools::static_format<100>("Task: '%s' unknown exception caught", this->m_name ) );
					while (true)
					{
					}
				  }
			});
		}

		void join()
		{
			Task_Interface::join();
		}

	  protected:
		void call_process() override {
			process();
		}

	  private:
		virtual void process() = 0;

		stack_t            m_mem_tcb[internal::calculate_depth(control_block_size_in_byte)] = {};
		std::span<stack_t> m_stack_span                                                     = {};
		const char*        m_name                                                           = nullptr;
		Priority           m_prio                                                           = Priority::idle;
	  };


	  /////////////////////////////////////////////
	  // Static_Task
	  /////////////////////////////////////////////


	  template <std::size_t N> class Static_Task: public Task_Interface
	  {
	  protected:
	    using stack_t = internal::stack_t;

	  private:
	    static constexpr uint32_t control_block_size_in_byte = 88;
	    static constexpr uint32_t stack_size_in_byte         = N;
	    static constexpr uint32_t calculate_depth(uint32_t size_in_byte) { return (size_in_byte + (sizeof(stack_t) - 1)) / sizeof(stack_t); }
	    std::thread handle;

	  public:
	    Static_Task(const char* name, Task_Interface::Priority const& prio = Task_Interface::Priority::idle)
	        : m_name(name)
	        , m_prio(prio)
	    {
	    }
	    virtual ~Static_Task() {
	    	this->join();
	    	this->delete_task();
	    }

	    void start() override
	    {
			handle = std::thread([this]() {
				try
				  {
					register_task( handle, std::this_thread::get_id(), m_name );

					auto stack = std::span<std::byte>(reinterpret_cast<std::byte*>(m_mem_stack),sizeof(m_mem_stack));

					if( !switch_to_user_defined_stack(this,stack) ) {
						process();
						task_ended( std::this_thread::get_id() );
					}
				  }
				  catch( const std::exception & error ) {
						CPPDEBUG( Tools::static_format<100>("Task: '%s' exception caught: '%s'", this->m_name, error.what() ) );
						while (true) {}
				  }
				  catch (...)
				  {
					CPPDEBUG( Tools::static_format<100>("Task: '%s' unknown exception caught", this->m_name ) );
					while (true) {}
				  }
			});
	    }

		void join()
		{
			Task_Interface::join();
		}

	  protected:
		void call_process() override {
			process();
		}

	  private:
	    virtual void process() = 0;

	    stack_t     m_mem_tcb[calculate_depth(control_block_size_in_byte)] = {};
	    stack_t     m_mem_stack[calculate_depth(stack_size_in_byte)]       = {};
	    const char* m_name                                                 = nullptr;
	    Priority    m_prio                                                 = Priority::idle;
	  };



	  /////////////////////////////////////////////
	  // Static_CallbackTask<0>
	  /////////////////////////////////////////////

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

	  /////////////////////////////////////////////
	  // Static_CallbackTask
	  /////////////////////////////////////////////

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


	  /////////////////////////////////////////////
	  // Static_MemberfunctionCallbackTask<0>
	  /////////////////////////////////////////////

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



	  /////////////////////////////////////////////
	  // Static_MemberfunctionCallbackTask
	  /////////////////////////////////////////////

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


	    class thread_accessor
	    {
	    	std::thread::id id;
	    public:

	    	thread_accessor( std::thread::id id_ )
	    	: id( id_ )
	    	{}

	    	bool set_keep_running( bool state );

	    	void join();

	    	const char * get_name();
	    };

	    void for_each_thread( std::function<bool(thread_accessor)> func );

	    void quit( int exit_code );

} // namespace os

