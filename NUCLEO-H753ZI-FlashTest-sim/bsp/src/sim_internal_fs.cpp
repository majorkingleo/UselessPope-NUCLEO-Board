#include <bsp_internal_fs.hpp>
#include <H7TwoFace.h>
#include <SimSTM32InternalFlashPc.h>
#include <H7TwoFaceConfig.h>

void BSP::init_internal_fs()
{
	static SimpleFlashFs::SimPc::SimSTM32InternalFlashPc mem_mapped1(".flash_page1.bin",SFF_MAX_SIZE);
	static SimpleFlashFs::SimPc::SimSTM32InternalFlashPc mem_mapped2(".flash_page2.bin",SFF_MAX_SIZE);

	H7TwoFace::set_memory_interface( &mem_mapped1, &mem_mapped2 );
}



