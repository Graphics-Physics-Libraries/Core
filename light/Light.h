#pragma once

#include "../scene/Object3DComponent.h"
#include "../color/Color.h"
#include "../geometry/Vector3.h"
#include "../base/BitMask.h"
#include "LightType.h"
#include "LightCullType.h"

namespace Core {

    // forward declarations
    class Engine;

    class Light : public Object3DComponent {
        friend class Engine;

    public:
        virtual ~Light() = 0;

        const Color& getColor() const;
        void setColor(const Color& color);
        void setColor(Real r, Real g, Real b, Real a);

        void setIntensity(Real intensity);
        Real getIntensity() const;

        void setShadowsEnabled(Bool enabled);
        Bool getShadowsEnabled() const;

        void setCullingMask(IntMask mask);
        IntMask getCullingMask() const;

    protected:
        Light(WeakPointer<Object3D> owner);

        Color color;
        Real intensity;
        Bool shadowsEnabled;
        IntMask cullingMask;
    };
}
