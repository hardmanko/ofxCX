#pragma once

#include "stdint.h"

//#define CX_NOT_USING_NAMESPACE_CX

#ifdef CX_NOT_USING_NAMESPACE_CX
#define CX_TLC_NAME
#else
#define CX_TLC_NAME CX_
#endif

#define __PASTE_2(a,b)		a##b
#define PASTE_2(a,b)	__PASTE_2(a,b)

#define CX_CLASS(name) PASTE_2(CX_TLC_NAME, name)

namespace CX {

	typedef uint64_t SwapUnit;

	typedef SwapUnit FrameNumber;
	typedef SwapUnit SampleFrame;

}