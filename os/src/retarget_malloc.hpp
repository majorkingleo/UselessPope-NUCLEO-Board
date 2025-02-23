#pragma once

#include <cstdint>

namespace os::internal {

// does nothing, just tell the linker to do not throw away
// the malloc wrappers
std::uintptr_t init_retarget_malloc();

} // namespace os::internal
