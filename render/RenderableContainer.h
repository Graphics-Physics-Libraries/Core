#pragma once

#include "../common/types.h"
#include "../math/Matrix4x4.h"
#include "../scene/Object3D.h"
#include "../geometry/Vector3.h"
#include "ObjectRenderer.h"
#include "BaseRenderableContainer.h"
#include "Renderable.h"

namespace Core {
  template <typename T>
  class RenderableContainer : public BaseRenderableContainer {

    std::vector<std::shared_ptr<T>> renderables;

  public:

    RenderableContainer() {
      
    }
    
    void addRenderable(std::shared_ptr<T> renderable) {
      renderables.push_back(renderable);
    }

    const std::vector<std::shared_ptr<T>> getRenderables() {
      return renderables;
    }

    void setRenderer(std::shared_ptr<ObjectRenderer<T>> renderer) {
      this->renderer = renderer;
    }

    std::shared_ptr<ObjectRenderer<T>> getRenderer() {
      return renderer;
    }
  };
}
