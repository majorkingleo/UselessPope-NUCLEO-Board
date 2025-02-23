#pragma once

#include <uC_DMA.hpp>

namespace BSP
{
  uC::DMA_Buffer_Allocator_Interface& get_dma_buffer_allocator();
}

