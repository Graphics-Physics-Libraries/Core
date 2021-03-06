#include "Object3D.h"
#include "../Engine.h"
#include "Transform.h"
#include "Object3DComponent.h"
#include "../light/Light.h"
#include "../render/Camera.h"
#include "../render/ReflectionProbe.h"

namespace Core {

    UInt64 Object3D::_nextID = 0;

    Object3D::Object3D() : transform(*this), active(true) {
        this->id = Object3D::getNextID();
    }

    Object3D::~Object3D() {
        for (UInt32 i = 0; i < this->children.size(); i ++) {
            WeakPointer<Object3D> child = this->children[i];
             Engine::safeReleaseObject(child);
        }
        for (UInt32 i = 0; i < this->components.size(); i ++) {
            WeakPointer<Object3DComponent> component = this->components[i];
            WeakPointer<Camera> cameraComponent = WeakPointer<Object3DComponent>::dynamicPointerCast<Camera>(component);
            WeakPointer<Light> lightComponent = WeakPointer<Object3DComponent>::dynamicPointerCast<Light>(component);
            WeakPointer<Mesh> meshComponent = WeakPointer<Object3DComponent>::dynamicPointerCast<Mesh>(component);
            WeakPointer<BaseObjectRenderer> rendererComponent = WeakPointer<Object3DComponent>::dynamicPointerCast<BaseObjectRenderer>(component);
            WeakPointer<ReflectionProbe> reflectionProbeComponent = WeakPointer<Object3DComponent>::dynamicPointerCast<ReflectionProbe>(component);
            if (lightComponent || cameraComponent || meshComponent || rendererComponent || reflectionProbeComponent) {
                 Engine::safeReleaseObject(component);
            }
        }
    }

    UInt64 Object3D::getID() const {
        return this->id;
    }

    UInt64 Object3D::getNextID() {
        return _nextID++;
    }

    Transform& Object3D::getTransform() {
        return this->transform;
    }

    SceneObjectIterator<Object3D> Object3D::beginIterateChildren() {
        return  SceneObjectIterator<Object3D>(this->children.begin());
    }

    SceneObjectIterator<Object3D> Object3D::endIterateChildren() {
        return SceneObjectIterator<Object3D>(this->children.end());
    }

    SceneObjectIterator<Object3DComponent> Object3D::beginIterateComponents() {
        return SceneObjectIterator<Object3DComponent>(this->components.begin());
    }

    SceneObjectIterator<Object3DComponent> Object3D::endIterateComponents() {
        return SceneObjectIterator<Object3DComponent>(this->components.end());
    }

    UInt32 Object3D::childCount() const {
        return this->children.size();
    }

    void Object3D::addChild(WeakPointer<Object3D> object) {
        
        if (object->parent.isValid()) {
            object->parent->removeChild(object);
        }

        Transform& worldTransform = this->getTransform();
        worldTransform.updateWorldMatrix();
        Matrix4x4 worldInverse = worldTransform.getWorldMatrix();
        worldInverse.invert();

        object->getTransform().getLocalMatrix().preMultiply(worldInverse);

        this->children.push_back(object);
        object->parent = this->_self;
    }

    void Object3D::removeChild(WeakPointer<Object3D> object) {
        auto end = this->endIterateChildren();
        auto result = end;
        for (SceneObjectIterator<Object3D> itr = this->beginIterateChildren(); itr != end; ++itr) {
            if (*itr == object) {
                result = itr;
                break;
            }
        }
        if (result != end) {
            Transform& transform = object->getTransform();
            transform.updateWorldMatrix();
            transform.getLocalMatrix().copy(transform.getWorldMatrix());
            this->children.erase(result.getSrc());
            object->parent = PersistentWeakPointer<Object3D>::nullPtr();
        }
    }

    WeakPointer<Object3D> Object3D::getParent() const {
        return this->parent;
    }

    Bool Object3D::addComponent(WeakPointer<Object3DComponent> component) {
        SceneObjectIterator<Object3DComponent> end = this->endIterateComponents();
        SceneObjectIterator<Object3DComponent> result = end;
        for(SceneObjectIterator<Object3DComponent> itr = this->beginIterateComponents(); itr != end; ++itr) {
            // don't add component if it already is present in list
            if (component == *itr) {
                return false;
            }
        }
        this->components.push_back(component);
        return true;
    }

    void Object3D::setActive(Bool active) {
        this->active = active;
    }

    Bool Object3D::isActive() const {
        return this->active;
    }

    void Object3D::setStatic(Bool objStatic) {
        this->objStatic = objStatic;
    }

    Bool Object3D::isStatic() const {
        return this->objStatic;
    }

    void Object3D::setName(const std::string& name) {
        this->name = name;
    }
    
    const std::string& Object3D::getName() const {
        return this->name;
    }

    UInt32 Object3D::childCount() {
        return this->children.size();
    }

    WeakPointer<Object3D> Object3D::getChild(UInt32 index) {
        return this->children[index];
    }
}