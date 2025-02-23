#ifdef _WIN32
#   define WIN32_LEAN_AND_MEAN
#endif

#include "os.hpp"
#include <map>
#include <memory>
#include <list>
#include <stderr_exception.h>
#include <cstring>
#include <static_format.h>
#include <format.h>

#ifdef _WIN32
#	include <libco.h>
#   include <windows.h>
#   include <sysinfoapi.h>
#   include <utf8_util.h>
#else
#	include <sys/mman.h>
#   include <ucontext.h>
#endif

#ifndef _MSC_VER
#    include <unistd.h>
#endif

// #define ENABLE_STACK_SWITCHING // terminating threads is not supported on WIN32, it will crash
// #define ENABLE_SYNC_THREAD // sync back to origin stack if wished

using namespace Tools;

namespace {

#ifdef _WIN32
  std::basic_string<TCHAR> GetErrorMessage(DWORD dwErrorCode)
  {
    LPTSTR psz{ nullptr };
    const DWORD cchMsg = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM
      | FORMAT_MESSAGE_IGNORE_INSERTS
      | FORMAT_MESSAGE_ALLOCATE_BUFFER,
      NULL, // (not used with FORMAT_MESSAGE_FROM_SYSTEM)
      dwErrorCode,
      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),      
      reinterpret_cast<LPTSTR>(&psz),
      0,
      NULL);
    if (cchMsg > 0)
    {
      // Assign buffer to smart pointer with custom deleter so that memory gets released
      // in case String's c'tor throws an exception.
      auto deleter = [](void* p) { ::LocalFree(p); };
      std::unique_ptr<TCHAR, decltype(deleter)> ptrBuffer(psz, deleter);
      return std::basic_string<TCHAR>(ptrBuffer.get(), cchMsg);
    }
    else
    {
      auto error_code{ ::GetLastError() };
      throw std::system_error( error_code, std::system_category(),
        "Failed to retrieve error message string.");
    }
  }

  std::string GetErrorMessageA(DWORD dwErrorCode)
  {
    auto msg = GetErrorMessage(dwErrorCode);

    return Utf8Util::wStringToUtf8(msg);
  }

#ifdef _MSC_VER
  // Determining approximately how much stack space is available by using alloca
  // https://devblogs.microsoft.com/oldnewthing/20200609-00/?p=103847

  __declspec(noinline)
    void* is_stack_available(size_t amount)
  {
    __try {
      void *start = _alloca(amount);

      // fill the stack with a known watermark value also used by freeRTOS
      memset( start, 0xa5, amount );

      return start;
    } __except (
      GetExceptionCode() == EXCEPTION_STACK_OVERFLOW
      ? EXCEPTION_EXECUTE_HANDLER
      : EXCEPTION_CONTINUE_SEARCH) {
      _resetstkoflw();
      return nullptr;
    }
  }
#endif

  struct StackLimits
  {
    void* LowLimit = nullptr;
    void* HighLimit = nullptr;
  };


	StackLimits add_watermark_to_stack_and_get_limits()
	{
		static int page_size = 0;
		static int granularity = 0;

		if( page_size == 0 ) {
		  SYSTEM_INFO si{};
		  GetSystemInfo(&si);

		  page_size = si.dwPageSize;
		  granularity = si.dwAllocationGranularity;
		}

		// Determining approximately how much stack space is available via GetCurrentThreadStackLimits
		// https://devblogs.microsoft.com/oldnewthing/20200610-00/?p=103855
		ULONG_PTR LowLimit;
		ULONG_PTR HighLimit;

		GetCurrentThreadStackLimits(&LowLimit, &HighLimit);

		// reserved space for exception handling, not available as stack
		ULONG guarantee = 0;
		SetThreadStackGuarantee(&guarantee);

		// So these sizes are a approximation of the usable stack space,
		// now use alloca to really allocate the space and check the exact available space.
		std::byte* start = reinterpret_cast<std::byte*>(&guarantee) + sizeof(decltype(guarantee));
		std::size_t size = (start - reinterpret_cast<std::byte*>(LowLimit)) - guarantee;

		// DEBUG(Tools::format("Thread stack limits: 0x%08X - 0x%08X size: %dKB %dMB", LowLimit, HighLimit, size / 1024, size / 1024 / 1024));

		uint32_t *low_limit = nullptr;
#ifdef _MSC_VER
		// reduce the size in steps of page_size until we find a size that is available
		for( ; low_limit == nullptr && size > 0; size -= page_size ) {
		  low_limit = reinterpret_cast<uint32_t*>(is_stack_available(size));
		  // CPPDEBUG(Tools::format("low_limit: %p size: %dkb", low_limit, size/1024));
		}
#else
		// mingw has no SEH, so use the size and reduce it by 100kb that seems to work
		// we should get 1MB on windows
		{
			size -= 100 * 1024;
			low_limit = reinterpret_cast<uint32_t*>(alloca( size ));
			// fill the stack with a known watermark value also used by freeRTOS
			memset( low_limit, 0xa5, size );
			//CPPDEBUG( Tools::format( "low_limit at: %p value 0x%0X", low_limit, *low_limit ) );
		}
#endif

		// Yes use the address of the local variable LowLimit to use it as high water mark.
		// This will cast away the os specific overhead.
		return { low_limit, reinterpret_cast<void*>(&LowLimit) };
	}

#endif

std::size_t used_size( std::span<std::byte> & stack )
{
	// test byte wise
	/*
	for( unsigned i = 0; i < stack.size(); ++i ) {
		if( stack[i] != std::byte(0xA5) ) {
			return stack.size() - i;
		}
	}
	*/

	// test in 32bit steps
	uint32_t fillsign;
	memset(&fillsign,0xA5,sizeof(fillsign));

	void *v_start = stack.data();
	std::size_t size = stack.size_bytes();

	uint32_t *p_start = reinterpret_cast<uint32_t*>( std::align( alignof(fillsign), stack.size_bytes(), v_start, size ) );

	for( unsigned i = 0; i < (size / sizeof(decltype(fillsign))); ++i ) {
		if( p_start[i] != fillsign ) {
			return size - (i * sizeof(decltype(fillsign)));
		}
	}

	return 0;
}

class ThreadStorage
{
public:
	struct ThreadInfo
	{
		std::thread::id id{};
		std::thread * thread = nullptr;
		std::atomic<bool> joined = false;
		os::binary_semaphore notifier{0};
		const char *name = nullptr;

#ifdef _WIN32
        void* mem_thread_free_page = nullptr;
#else
		ucontext_t uc_default_thread;
		ucontext_t uc_user_stack;
#endif

		std::span<std::byte> stack{};

		bool free_mapped_stack = false;
		std::span<std::byte> mapped_stack{};
		bool unmap_stack = false;
		std::atomic<bool> stop_sync_thread = false;
		std::optional<std::thread> sync_thread;
		std::atomic<bool> stop_thread = false;

		~ThreadInfo()
		{
			if( sync_thread ) {
				stop_sync_thread = true;
				sync_thread->join();
			}

			if( !mapped_stack.empty() && free_mapped_stack ) {
#ifdef _WIN32
                VirtualFree(mapped_stack.data(), 0, MEM_RELEASE);                
#else 
				munmap( mapped_stack.data(), mapped_stack.size_bytes() );
#endif
                mapped_stack = {};
			}

#ifdef _WIN32
            if (mem_thread_free_page) {
              VirtualFree(mem_thread_free_page, 0, MEM_RELEASE);
              mem_thread_free_page = nullptr;
            }
#endif
		}

		void start_sync_stack_thread( std::span<std::byte> origin_stack ) {
			sync_thread = std::thread([this,origin_stack](){
				while( !stop_sync_thread ) {
					if( origin_stack.size_bytes() != stack.size_bytes() ) {
						throw STDERR_EXCEPTION( Tools::static_format<100>("Thread: '%s' different stack sizes: %d != %d",
								name,
								stack.size_bytes(),
								origin_stack.size_bytes() ));
					}

					std::memcpy( origin_stack.data(), stack.data(), origin_stack.size_bytes() );
					std::this_thread::sleep_for(std::chrono::milliseconds(300));
				}
			});
		}

		void join() {
			if( !joined && thread ) {
				thread->join();
			}

			joined = true;
		}
	};

	std::map<std::thread::id,std::shared_ptr<ThreadInfo>> threads;
	std::mutex m_threads;

public:

	std::shared_ptr<ThreadInfo> get( std::thread::id id ) {
		auto lock = std::lock_guard( m_threads );

		auto it = threads.find( id );
		if( it != threads.end() ) {
			return it->second;
		}

		return {};
	}

	std::shared_ptr<ThreadInfo> create( std::thread::id id ) {
		auto lock = std::lock_guard( m_threads );

		auto thread_info = std::make_shared<ThreadInfo>();

		thread_info->id = id;

		threads[id] = thread_info;

		return thread_info;
	}

	void remove( std::thread::id id ) {
		auto lock = std::lock_guard( m_threads );

		auto it = threads.find( id );
		if( it != threads.end() ) {
			threads.erase(it);
		}
	}

#ifdef _WIN32
private:
	os::Task_Interface *active_thread = nullptr;

public:
	void set_active_thread_and_lock( os::Task_Interface *active_thread_ ) {
		m_threads.lock();

		active_thread = active_thread_;
	}

	 os::Task_Interface* get_active_thread_and_release() {
		 os::Task_Interface* ret = active_thread;
		 m_threads.unlock();
		 return ret;
	}


#endif


	 void for_each_thread( std::function<bool(os::thread_accessor)> func )
	 {
		 std::vector<std::thread::id> ids;

		 {
			 auto lock = std::lock_guard( m_threads );
			 ids.reserve(threads.size());

			 for( auto & thread : threads ) {
				 ids.push_back( thread.first );
			 }
		 }

		 for( auto & id : ids ) {
			 if( !func( os::thread_accessor(id) ) ) {
				 break;
			 }
		 }
	 }
};

static ThreadStorage thread_storage;

} // namespace

/*
 * @return The task's notification count before it is either cleared to zero or
 * decremented (see the xClearCountOnExit parameter).
 *
 * 0   : on timeout
 * > 0 : amount of notifications by other tasks
 */
uint32_t os::this_thread::wait_for_notify()
{
	auto thread_info = thread_storage.get(std::this_thread::get_id());

	if( !thread_info ) {
		throw STDERR_EXCEPTION( "cannot find my own thread in storage" );
	}

	thread_info->notifier.acquire();
	return 1;
}

uint32_t os::this_thread::try_wait_for_notify_for(std::chrono::nanoseconds const& delay)
{
	auto thread_info = thread_storage.get(std::this_thread::get_id());

	if( !thread_info ) {
		throw STDERR_EXCEPTION( "cannot find my own thread in storage" );
	}

	auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(delay + std::chrono::milliseconds(1));

	if( thread_info->notifier.try_acquire_for( dur ) ) {
		return 1;
	}

	return 0;
}

/**
 * returns true as long as the thread should run
 */
bool os::this_thread::keep_running()
{
	auto thread_info = thread_storage.get(std::this_thread::get_id());

	if( !thread_info ) {
		throw STDERR_EXCEPTION( "cannot find my own thread in storage" );
	}

	return !thread_info->stop_thread;
}

os::Task_Interface::~Task_Interface()
{

}

void os::Task_Interface::delete_task()
{
	thread_storage.remove(id);
}

void os::Task_Interface::join()
{
	auto thread_info = thread_storage.get(id);

	if( thread_info ) {
		thread_info->join();
	}
}

void os::Task_Interface::notify()
{
	auto thread_info = thread_storage.get(id);
	thread_info->notifier.release();
}

void os::Task_Interface::register_task( std::thread & thread, std::thread::id id_, const char *name )
{
	id = id_;
	auto thread_info = thread_storage.create( id );
	thread_info->name = name;
	thread_info->thread = &thread;
}

void os::Task_Interface::task_ended( std::thread::id id )
{
	auto thread_info = thread_storage.get(id);

	if( thread_info->sync_thread ) {
		thread_info->stop_sync_thread = true;
		thread_info->sync_thread->join();
		thread_info->sync_thread.reset();
	}
}

void os::Task_Interface::entry_point_switched_stacks( os::Task_Interface *thread )
{
	try {
		thread->call_process();
	}
	catch (...)
	{
		while (true)
		{
		}
	}
}

#ifdef _WIN32
static bool allocate_stack_with_protected_areas( std::shared_ptr<ThreadStorage::ThreadInfo> & thread_info, const std::span<std::byte> & stack )
{
  static std::mutex m;
  std::scoped_lock lock(m);

  static int page_size = 0;

  if( page_size == 0 ) {
    SYSTEM_INFO si{};
    GetSystemInfo(&si);

    page_size = si.dwAllocationGranularity;
    //page_size = si.dwPageSize;
  }

  unsigned amount = stack.size_bytes() / page_size;

  if( (stack.size_bytes() % page_size) > 0 ) {
    amount++;
  }

  unsigned required_pages = amount;

  std::list<std::pair<void*,void*>> allocated_page_pairs;

  auto as_byte = []( void* p) { 
    return reinterpret_cast<std::byte*>(p); 
  };

  /**
   * Allocate a reserved page and a data page. The reserved page is used to not allow memory access there
   * The reserved page has to be exactly before the data page.
   */
  do {
    void *reserved_page = VirtualAlloc(nullptr, page_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

    if( reserved_page == nullptr ) {
      CPPDEBUG(Tools::format("VirtualAlloc failed: %s", GetErrorMessageA(GetLastError())));
      throw STDERR_EXCEPTION( "cannot allocate via VirtualAlloc" );
    }

    void *data_page = VirtualAlloc(nullptr, required_pages * page_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);  
    if( data_page == nullptr ) {
      CPPDEBUG(Tools::format("VirtualAlloc failed: %s", GetErrorMessageA(GetLastError())));
      throw STDERR_EXCEPTION( "cannot allocate via VirtualAlloc" );
    }

    std::byte* expected_data_page_start_adress = as_byte(reserved_page) + page_size;

    if( expected_data_page_start_adress == as_byte(data_page) ) {
      allocated_page_pairs.push_back(std::make_pair(reserved_page, data_page));
      break;
    } else {
      // CPPDEBUG("reserved_page not placed before data_page, retrying");
      // CPPDEBUG(Tools::format("expected_data_page_start_adress: %p, data_page: %p", expected_data_page_start_adress, as_byte(data_page)));
    }
  
  } while( true );

  


  thread_info->mem_thread_free_page = allocated_page_pairs.rbegin()->first; 
  void* vbase = allocated_page_pairs.rbegin()->second;
  allocated_page_pairs.pop_back();

  // deallocate the reserved page, so now every access to this page will cause a SIGSEGV
  // but do not free it here, because the next thread will get the same page, but should 
  // get another one
  if( !VirtualFree(thread_info->mem_thread_free_page, page_size, MEM_DECOMMIT) ) {
    CPPDEBUG(GetErrorMessageA(GetLastError()));
    throw STDERR_EXCEPTION( "cannot decommit memmory" );
  }

  
  std::byte* base = reinterpret_cast<std::byte*>( vbase );

  auto mapped_stack = std::span<std::byte>( base, required_pages * page_size );
  thread_info->stack = mapped_stack.subspan( 0, stack.size_bytes() );
  thread_info->mapped_stack = mapped_stack;
  thread_info->free_mapped_stack = true;

  while( !allocated_page_pairs.empty() ) {
    auto p = allocated_page_pairs.back();
    allocated_page_pairs.pop_back();
    
    if( !VirtualFree(p.first, 0, MEM_RELEASE) ) {
      CPPDEBUG(GetErrorMessageA(GetLastError()));
      throw STDERR_EXCEPTION("cannot release memmory");
    }

    if( !VirtualFree(p.second, 0, MEM_RELEASE)) {
      CPPDEBUG(GetErrorMessageA(GetLastError()));
      throw STDERR_EXCEPTION("cannot release memmory");
    }
  }

  return true;
}
#else
static bool allocate_stack_with_protected_areas( std::shared_ptr<ThreadStorage::ThreadInfo> & thread_info, const std::span<std::byte> & stack )
{
	static const int page_size = sysconf(_SC_PAGE_SIZE);
	unsigned amount = stack.size_bytes() / page_size;

	if( (stack.size_bytes() % page_size) > 0 ) {
		amount++;
	}

	unsigned required_pages = 2 + amount;

	void* vbase = mmap(nullptr,required_pages * page_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0 );

	if( vbase == MAP_FAILED ) {
		throw STDERR_EXCEPTION( "cannot allocate pages via mmap" );
	}

	// free first page, any access on these data will cause a SIGSEGV
	if( munmap( vbase, page_size ) != 0 ) {
		throw STDERR_EXCEPTION( "cannot free first page" );
	}

	std::byte* base = reinterpret_cast<std::byte*>( vbase );

	// free last page, any access on these data will cause a SIGSEGV
	if( munmap( base + (required_pages - 1) * page_size, page_size ) != 0 ) {
		throw STDERR_EXCEPTION( "cannot free last page" );
	}

	thread_info->mapped_stack = std::span<std::byte>( base + page_size, (required_pages - 2) * (page_size) );
	thread_info->stack = thread_info->mapped_stack.subspan( 0, stack.size_bytes() );
	thread_info->free_mapped_stack = true;

	return true;
}
#endif

#ifdef _WIN32

void os::Task_Interface::entry_point_switched_stacks_win32()
{
	os::Task_Interface *thread = thread_storage.get_active_thread_and_release();
	entry_point_switched_stacks(thread);
}

bool os::Task_Interface::switch_to_user_defined_stack( Task_Interface *thread, std::span<std::byte> stack )
{
#ifdef ENABLE_STACK_SWITCHING
	auto thread_info = thread_storage.get(thread->id);

	if( !thread_info ) {
		throw STDERR_EXCEPTION( "cannot find thread in storage" );
	}

	if( !allocate_stack_with_protected_areas( thread_info, stack ) ) {
		thread_info->stack = stack;
	}

	// FreeRTOS bit sign mask
	std::memset( thread_info->stack.data(), 0xa5, thread_info->stack.size_bytes() );


    // if we are using an allocated stack, start a thread that syncs the content from time
    // to time back to the origin stack. To make stack usage tools still work.

#ifdef ENABLE_SYNC_THREAD
    if( !thread_info->mapped_stack.empty() ) {
      // FreeRTOS bit sign mask
      std::memset( stack.data(), 0xa5, stack.size_bytes() );
      thread_info->start_sync_stack_thread( stack );
    }
#endif
  
    thread_storage.set_active_thread_and_lock(thread);
	cothread_t t = co_create_ex( thread_info->stack.data(), thread_info->stack.size_bytes(), entry_point_switched_stacks_win32 );
	co_switch( t );

	return true;
#else
	auto thread_info = thread_storage.get(thread->id);

	if( !thread_info ) {
		throw STDERR_EXCEPTION( "cannot find thread in storage" );
	}

	StackLimits limits = add_watermark_to_stack_and_get_limits();

	std::byte* b_low  = reinterpret_cast<std::byte*>(limits.LowLimit);
	std::byte* b_high = reinterpret_cast<std::byte*>(limits.HighLimit);

	thread_info->mapped_stack = std::span<std::byte>( b_low, b_high );
	thread_info->stack = thread_info->mapped_stack.subspan( 0, stack.size() );

	/*
	CPPDEBUG( Tools::format( "stack size: %d start: %p of stack '%s'",
			thread_info->stack.size_bytes(),
			thread_info->stack.data(),
			thread_info->name ));
	*/

	// FreeRTOS bit sign mask
	std::memset( stack.data(), 0xa5, stack.size_bytes() );

	/*
	{
		std::size_t size = used_size( thread_info->mapped_stack );
		CPPDEBUG( Tools::format( "used mapped stack size: %dB %dKB", size, size / 1024 ) );
	}

	{
		std::size_t size = used_size( thread_info->stack );
        CPPDEBUG( Tools::format( "used sub    stack size: %dB %dKB", size, size / 1024 ) );
	}


	{
		std::size_t size = used_size( stack );
        CPPDEBUG( Tools::format( "used origin stack size: %dB %dKB", size, size / 1024 ) );
	}
	*/

#ifdef ENABLE_SYNC_THREAD
	thread_info->start_sync_stack_thread( stack );
#endif

	return false;
#endif
}

#else
bool os::Task_Interface::switch_to_user_defined_stack( Task_Interface *thread, std::span<std::byte> stack )
{
	return false;
	auto thread_info = thread_storage.get(thread->id);

	if( !thread_info ) {
		throw STDERR_EXCEPTION( "cannot find thread in storage" );
	}

	if( !allocate_stack_with_protected_areas( thread_info, stack ) ) {
		thread_info->stack = stack;
	}

	// FreeRTOS bit sign mask
	std::memset( thread_info->stack.data(), 0xa5, thread_info->stack.size_bytes() );

	if( getcontext(&thread_info->uc_user_stack) != 0 ) {
		throw STDERR_EXCEPTION("getcontext failed");
	}

	thread_info->uc_user_stack.uc_stack.ss_sp = thread_info->stack.data();
	thread_info->uc_user_stack.uc_stack.ss_size = thread_info->stack.size_bytes();
	thread_info->uc_user_stack.uc_link = &thread_info->uc_default_thread; // return to the default thread stack when finished

	makecontext( &thread_info->uc_user_stack, (void(*)())entry_point_switched_stacks, 1, thread );

	// if we are using an allocated stack, start a thread that syncs the content from time
	// to time back to the origin stack. To make stack usage tools still work.

	if( !thread_info->mapped_stack.empty() ) {
		// FreeRTOS bit sign mask
		std::memset( stack.data(), 0xa5, stack.size_bytes() );
		// CPPDEBUG( static_format<100>( "Thread: '%s' stack size: %d", thread_info->name, stack.size_bytes() ));
#ifdef ENABLE_SYNC_THREAD
		thread_info->start_sync_stack_thread( stack );
#endif
	}

	if( swapcontext( &thread_info->uc_default_thread, &thread_info->uc_user_stack ) != 0 ) {
		throw STDERR_EXCEPTION("swapcontext failed");
	}

	return true;
}

#endif

void os::quit( int exit_code )
{
	os::for_each_thread( []( os::thread_accessor ac ){
		ac.set_keep_running(false);
		return true;
	});

	os::for_each_thread( []( os::thread_accessor ac ){
		CPPDEBUG( static_format<100>("waiting for thread '%s' to terminate", ac.get_name() ) );
		ac.join();
		return true;
	});

	CPPDEBUG( "all threads ended, calling exit" );

	//exit(0);
}


void os::for_each_thread( std::function<bool(os::thread_accessor)> func )
{
	thread_storage.for_each_thread( func );
}

bool os::thread_accessor::set_keep_running( bool state_ )
{
	auto thread_info = thread_storage.get(id);

	if( !thread_info ) {
		return false;
	}

	thread_info->stop_thread = !state_;
	thread_info->notifier.release(); // notify thread that something changed
	return true;
}

void os::thread_accessor::join()
{
	auto thread_info = thread_storage.get(id);

	if( !thread_info ) {
		return;
	}

	thread_info->join();
}

const char * os::thread_accessor::get_name()
{
	auto thread_info = thread_storage.get(id);

	if( !thread_info ) {
		return nullptr;
	}

	return thread_info->name;
}
