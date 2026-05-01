#pragma once

#ifdef OF_SDK_AVAILABLE
#include <graphics/ofMesh.h>
#else

class ofMesh {
public:
    void draw() const {}
};

#endif
