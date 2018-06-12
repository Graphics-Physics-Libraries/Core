#pragma once

#include <new>
#include <unordered_map>
#include "../common/assert.h"
#include "../common/types.h"
#include "../render/Renderable.h"
#include "../scene/Object3D.h"
#include "Vector3.h"
#include "Vector2.h"
#include "../color/Color.h"
#include "AttributeArray.h"
#include "../material/StandardAttributes.h"
#include "Box3.h"

namespace Core {

  class Mesh : public Renderable<Mesh>  {
  public:

    virtual ~Mesh();

    virtual void init();

    UInt32 getSize() const;

    virtual AttributeArray<Vector3rs>* getVertexPositions() = 0;
    virtual AttributeArray<ColorS>* getVertexColors() = 0;
    virtual AttributeArray<Vector2rs>* getVertexUVs() = 0;

    virtual Bool initVertexPositions(UInt32 size) = 0;
    virtual Bool initVertexColors(UInt32 size) = 0;
    virtual Bool initVertexUVs(UInt32 size) = 0;

    void enableAttribute(StandardAttributes attribute);
    void disableAttribute(StandardAttributes attribute);
    Bool isAttributeEnabled(StandardAttributes attribute);

    virtual void setIndices(UInt32 * indices)  = 0;
    Bool isIndexed();

    void calculateBoundingBox();
    const Box3& getBoundingBox() const;

  protected:
    Mesh(UInt32 size, Bool indexed);
    void initAttributes();
    virtual void initIndices() = 0;

    Bool initialized;
    Bool enabledAttributes[(UInt32)StandardAttributes::_Count];
    UInt32 size;
    Bool indexed;
    Box3 boundingBox;
  };

}
