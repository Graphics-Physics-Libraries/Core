#pragma once

#include "../util/WeakPointer.h"
#include "Material.h"
#include "../color/Color.h"

namespace Core {

    // forward declarations
    class Engine;

    class OutlineMaterial: public Material {
        friend class Engine;

    public:
        virtual Bool build() override;
        virtual Int32 getShaderLocation(StandardAttribute attribute, UInt32 offset = 0) override;
        virtual Int32 getShaderLocation(StandardUniform uniform, UInt32 offset = 0) override;
        virtual void sendCustomUniformsToShader() override;
        virtual WeakPointer<Material> clone() override;

        void setColor(Color color);
        void setZOffset(Real offset);

    private:
        OutlineMaterial(WeakPointer<Graphics> graphics);
        void bindShaderVarLocations();

        Color color;
        Real zOffset;

        Int32 positionLocation;
        Int32 colorLocation;
        Int32 projectionMatrixLocation;
        Int32 viewMatrixLocation;
        Int32 modelMatrixLocation;
    };
}
