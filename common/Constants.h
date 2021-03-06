#pragma once

#include "types.h"

namespace Core {

    class Constants {
    public:
        static const UInt32 MaxDirectionalCascades = 3;
        static const UInt32 MaxShaderPointLights = 3;
        static const UInt32 MaxShaderDirectionalLights = 1;
        static const UInt32 MaxShaderLights = MaxShaderPointLights + MaxShaderDirectionalLights;
        static const UInt32 MaxIBLLODLevels = 6;
        static const UInt32 DefaultMaxMipLevels = 4;
        static const UInt32 MaxBonesPerVertex = 4;
        static const UInt32 MaxBones = 128;
        #ifdef CORE_USE_PRIVATE_INCLUDES
        static constexpr UInt32 TempRenderTargetSize = 4096;
        #endif
    };
}