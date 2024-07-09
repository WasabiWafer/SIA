#pragma once

#include "SIA/internals/types.hpp"
#include "SIA/internals/tags.hpp"

#if defined(SIA_OS_WINDOW)
// doc list
// https://learn.microsoft.com/en-us/windows/win32/procthread/processes-and-threads
// https://learn.microsoft.com/en-us/windows/win32/api/_processthreadsapi/
// window include...
#pragma comment(lib, "Kernel32")
#include "processthreadsapi.h"


//linux functions...


#elif defined(SIA_OS_LINUX)
// doc list
//linux include...



//window functions...


#endif
