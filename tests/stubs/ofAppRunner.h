#pragma once

#ifdef OF_SDK_AVAILABLE
#include <app/ofAppRunner.h>
#else

inline float ofGetFrameRate() { return 60.0f; }

#endif
