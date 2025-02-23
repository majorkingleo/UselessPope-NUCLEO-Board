
#include <malloc.h>
#include <os.hpp>
#include "retarget_malloc.hpp"

/**
 * https://sourceware.org/newlib/libc.html#g_t_005f_005fmalloc_005flock
 *
 * The malloc family of routines call these functions when they need to lock the memory pool.
 * The version of these routines supplied in the library use the lock API defined in sys/lock.h.
 * If multiple threads of execution can call malloc, or if malloc can be called reentrantly,
 * then you need to define your own versions of these functions in order to safely lock the
 * memory pool during a call. If you do not, the memory pool may become corrupted.
 *
 * A call to malloc may call __malloc_lock recursively; that is, the sequence of calls may
 * go __malloc_lock, __malloc_lock, __malloc_unlock, __malloc_unlock. Any implementation
 * of these routines must be careful to avoid causing a thread to wait for a lock that
 * it already holds.
 */

#define __PROGRAM_START
#include <cmsis_compiler.h>  // needed for __get_IPSR __get_PRIMASK and __get_BASEPRI
#define __undef

 #ifndef IS_IRQ_MODE
   #define IS_IRQ_MODE() (__get_IPSR() != 0U)
 #endif
 #ifndef IS_IRQ_MASKED
   #define IS_IRQ_MASKED() ((__get_PRIMASK() != 0U) || (__get_BASEPRI() != 0U))
 #endif

static std::optional<os::mutex> malloc_mutex;

extern "C" __attribute__((used)) void __malloc_lock(struct _reent *_r)
{
  // mutex may not be used before OS is initialized
  // or we are in interrupt context
  if ((os::running() == false) || (IS_IRQ_MODE() || IS_IRQ_MASKED())) {
    return;
  }

  if (!malloc_mutex) {
    malloc_mutex.emplace();
  }

  malloc_mutex->lock();
}

extern "C" __attribute__((used)) void __malloc_unlock(struct _reent *_r)
{
  // mutex may not be used before OS is initialized
  // or we are in interrupt context
  if ((os::running() == false) || (__get_IPSR() != 0)) {
    return;
  }
  malloc_mutex->unlock();
}

std::uintptr_t os::internal::init_retarget_malloc()
{
	std::uintptr_t p1 = reinterpret_cast<uintptr_t>(__malloc_lock );
	std::uintptr_t p2 = reinterpret_cast<uintptr_t>(__malloc_unlock);

	return p1+p2;
}
