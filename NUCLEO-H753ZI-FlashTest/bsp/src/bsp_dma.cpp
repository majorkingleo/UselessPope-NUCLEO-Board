#include <bsp_dma.hpp>

uC::DMA_Buffer_Allocator_Interface& BSP::get_dma_buffer_allocator()
{
	using T = uC::DMA_Buffer_Allocator;
	static std::byte __attribute__((section(".reserved_for_DMA_BUFFER"))) buffer[1024 * 150]{};
	static T                                                              obj(buffer);
	return obj;
}



