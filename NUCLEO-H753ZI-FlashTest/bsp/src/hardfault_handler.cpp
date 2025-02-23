#include <bsp.hpp>
#include <static_format.h>
#include <stm32h753xx.h>

using namespace Tools;

namespace {

template<unsigned N>
void StrCpy( char (&target)[N], const auto & source ) {
	strncpy( target, source.c_str(), N );
	target[N-1] = 0;
}

} // namespace

extern "C" void HardFault_Handler()
{
	wlib::StringSink_Interface& sink = BSP::get_uart_output_debug();

	static uint32_t CFSRValue;
	static char  msg[80];

	sink("In Hard Fault Handler\n");
	StrCpy( msg, static_format<100>("SCB->HFSR = 0x%08x\n", (unsigned int)SCB->HFSR) );
	sink(msg);
	if ((SCB->HFSR & (1UL << 30)) != 0)
	{
	  sink("Forced Hard Fault\n");

	  StrCpy( msg, static_format<100>("SCB->CFSR = 0x%08x\n", (unsigned int)SCB->CFSR));
	  sink(msg);

	  if ((SCB->CFSR & 0xFFFF0000UL) != 0)
	  {
		sink("Usage fault: \n");

		CFSRValue = (SCB->CFSR >> 16);  // right shift to lsb
		if ((CFSRValue & (1 << 0)) != 0)
		{
		  sink("Undefined instruction usage fault\n");
		}
		if ((CFSRValue & (1 << 1)) != 0)
		{
		  sink("Invalid state Usage Fault\n");
		}
		if ((CFSRValue & (1 << 2)) != 0)
		{
		  sink("Invalid PC load Usage Fault\n");
		}
		if ((CFSRValue & (1 << 3)) != 0)
		{
		  sink("No coprocessor Usage Fault\n");
		}
		if ((CFSRValue & (1 << 8)) != 0)
		{
		  sink("Unaligned access Usage Fault\n");
		}
		if ((CFSRValue & (1 << 9)) != 0)
		{
		  sink("Divide by zero Usage Fault\n");
		}
	  }
	  if ((SCB->CFSR & 0xFF00) != 0)
	  {
		sink("Bus fault: \n");
		CFSRValue = ((SCB->CFSR & 0x0000FF00UL) >> 8);  // mask and right shift to
														// lsb
		if ((CFSRValue & (1 << 0)) != 0)
		{
		  sink("Instruction bus error\n");
		}
		if ((CFSRValue & (1 << 1)) != 0)
		{
		  sink("Precise data bus error\n");
		}
		if ((CFSRValue & (1 << 2)) != 0)
		{
		  sink("Imprecise data bus error\n");
		}
		if ((CFSRValue & (1 << 3)) != 0)
		{
		  sink("on unstacking for return\n");
		}
		if ((CFSRValue & (1 << 4)) != 0)
		{
		  sink("on stacking for exception entry\n");
		}
		if ((CFSRValue & (1 << 5)) != 0)
		{
		  sink("during floating point lazy state prevention\n");
		}
		if ((CFSRValue & (1 << 7)) != 0)
		{
		  sink("Addres Register valid flag\n");
		  StrCpy( msg, static_format<100>( "SCB->BFAR = 0x%08x\n", (unsigned int)SCB->BFAR) );
		  sink(msg);
		}
	  }
	  if ((SCB->CFSR & 0xFF) != 0)
	  {
		sink("Memory Management fault: \n");
		CFSRValue = (SCB->CFSR & 0x000000FFUL);  // mask just mem faults
		if ((CFSRValue & (1 << 0)) != 0)
		{
		  sink("Instruction Access violation\n");
		}
		if ((CFSRValue & (1 << 1)) != 0)
		{
		  sink("Data Access violation\n");
		}
		if ((CFSRValue & (1 << 3)) != 0)
		{
		  sink("unstacking for exception return fault\n");
		}
		if ((CFSRValue & (1 << 4)) != 0)
		{
		  sink("stacking for exception entry fault\n");
		}
		if ((CFSRValue & (1 << 5)) != 0)
		{
		  sink("floating point lazy state preservation\n");
		}
		if ((CFSRValue & (1 << 7)) != 0)
		{
		  sink("Memory  Management Fault Address register\n");
		  StrCpy( msg, static_format<100>( "SCB->MMFAR = 0x%08x\n", (unsigned int)SCB->MMFAR) );
		  sink(msg);
		}
	  }
	}

	sink("\n");

	__asm("bkpt 255");
	__asm("bx lr");
}
