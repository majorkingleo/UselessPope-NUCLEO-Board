#include <FreeRTOS.h>
#include <bslib.hpp>
#include <functional>
#include <task.h>
#include "retarget_malloc.hpp"
#include <wlib.hpp>

namespace BSP
{
  void                         init();
  void                         enter_save_state();
  wlib::StringSink_Interface& get_uart_output_debug();
}    // namespace BSP

int                        main();
extern std::span<uint64_t> stack_main;

namespace
{
  class MainTask: public os::Static_Task<0>
  {
  protected:
    using func_t = std::function<void()>;
    const func_t func;

  public:
    MainTask(const std::span<uint64_t>& stack)
        : Static_Task(stack, "main")
    {
      this->start();
    }

    void process() override
    {
      BSP::init();
      wlib::StringSink_Interface& sink = BSP::get_uart_output_debug();
      try
      {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
        main();
#pragma GCC diagnostic pop
      }
      catch (std::exception const& ex)
      {
        sink(ex.what());
      }
      catch (char const* ex)
      {
        sink(ex);
      }
      catch (...)
      {
        sink("unknown Error");
      }
      while (true)
      {
      }
    }
  };
  MainTask task_main(stack_main);
}    // namespace

extern "C" void start_runtime()
{
  // dummy call for using __malloc_lock function hooks
  os::internal::init_retarget_malloc();

  vTaskStartScheduler();
  while (true)
  {
  }
}
