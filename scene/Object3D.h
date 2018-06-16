#pragma once

#include <vector>
#include <memory>
#include <typeinfo>
#include <unordered_map>

#include "Object3DComponent.h"
#include "../common/assert.h"
#include "Transform.h"
#include "../render/BaseRenderable.h"
#include "../render/Renderable.h"
#include "../render/ObjectRenderer.h"
#include "../render/ObjectRenderers.h"

namespace Core {

  // forward declarations
  class Mesh;
  class MeshRenderer;
  class Engine;

  class Object3D {
    friend class Engine;

  public:
    virtual ~Object3D();

    Transform& getTransform();
    std::vector<std::shared_ptr<Object3D>>::iterator beginIterateChildren();
    std::vector<std::shared_ptr<Object3D>>::iterator endIterateChildren();
    std::vector<WeakPointer<Object3DComponent>>::iterator beginIterateComponents();
    std::vector<WeakPointer<Object3DComponent>>::iterator endIterateComponents();
    UInt32 size() const;
    void addChild(WeakPointer<Object3D> object);
    void removeChild(WeakPointer<Object3D> object);
    WeakPointer<Object3D> getParent();
    Bool addComponent(WeakPointer<Object3DComponent> component);
  
  protected:
    Object3D();
    
    Transform transform;
    std::vector<std::shared_ptr<Object3D>> children;
    WeakPointer<Object3D> parent;
    std::vector<WeakPointer<Object3DComponent>> components;
  
  };

}
