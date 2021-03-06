#pragma once

#include <type_traits>

#include "../util/ValueIterator.h"
#include "../common/types.h"
#include "../geometry/Vector3.h"
#include "../math/Matrix4x4.h"
#include "ObjectRenderer.h"
#include "BaseRenderableContainer.h"

namespace Core {

    // forward declaration
    class Engine;
    template <typename T> class Renderable;
    template <typename T> class ObjectRenderer;

    template <typename T, typename Enable = void>
    class RenderableContainer;

    template <typename T>
    class RenderableContainer<T, Core::enable_if_t<std::is_base_of<Renderable<T>, T>::value>> : public BaseRenderableContainer {
        friend class Engine;

    public:
        void addRenderable(WeakPointer<T> renderable) {
            renderables.push_back(renderable);
        }

        const std::vector<PersistentWeakPointer<T>>& getRenderables() {
            return renderables;
        }

        WeakPointer<ObjectRenderer<T>> getRenderer() {
            return this->localRendererRef;
        }

        ValueIterator<typename std::vector<WeakPointer<T>>::iterator> begin() {
            return this->renderables.begin();
        }

        ValueIterator<typename std::vector<WeakPointer<T>>::iterator> end() {
            return this->renderables.end();
        }

    protected:
        RenderableContainer() {
        }

        void setRenderer(std::shared_ptr<ObjectRenderer<T>> renderer) {
            this->setBaseRenderer(std::static_pointer_cast<BaseObjectRenderer>(renderer));
            this->localRendererRef = renderer;
        }

        PersistentWeakPointer<ObjectRenderer<T>> localRendererRef;
        std::vector<PersistentWeakPointer<T>> renderables;
    };
}
