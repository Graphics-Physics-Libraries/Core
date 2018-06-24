#include "../util/WeakPointer.h"
#include "Object3D.h"

namespace Core {

    Transform::Transform(const Object3D& target) : target(target) {
        this->localMatrix.setIdentity();
        this->worldMatrix.setIdentity();
    }

    Transform::Transform(const Object3D& target, const Matrix4x4& matrix) : target(target) {
        this->localMatrix.copy(matrix);
    }

    Transform::~Transform() {
    }

    Matrix4x4& Transform::getLocalMatrix() {
        return this->localMatrix;
    }

    Matrix4x4& Transform::getWorldMatrix() {
        return this->worldMatrix;
    }

    const Matrix4x4& Transform::getConstLocalMatrix() const {
        return this->localMatrix;
    }

    const Matrix4x4& Transform::getConstWorldMatrix() const {
        return this->worldMatrix;
    }

    /*
     * Copy this Transform object's local matrix into [dest].
     */
    void Transform::toLocalMatrix(Matrix4x4& dest) const {
        dest.copy(this->localMatrix);
    }

    /*
     * Copy this Transform object's world matrix into [dest].
     */
    void Transform::toWorldMatrix(Matrix4x4& dest) const {
        dest.copy(this->worldMatrix);
    }

    void Transform::setTo(const Matrix4x4& mat, Bool updateWorld) {
        this->localMatrix.copy(mat);
        if (updateWorld) {
            this->updateWorldMatrix();
        }
    }

    void Transform::transform(Vector4<Real>& vector) const {
        this->worldMatrix.transform(vector);
    }

    void Transform::transform(Vector3Base<Real>& vector) const {
        this->worldMatrix.transform(vector);
    }

    void Transform::getWorldTransformation(WeakPointer<Object3D> target, Matrix4x4& result) {
        result.setIdentity();
        if (!target.isValid()) return;
        result.copy(target->getTransform().getLocalMatrix());
        WeakPointer<Object3D> parent = target->getParent();
        while (parent.isValid()) {
            result.preMultiply(parent->getTransform().getLocalMatrix());
            parent = parent->getParent();
        }
    }

    void Transform::getAncestorWorldTransformation(Matrix4x4& result) {
        Transform::getWorldTransformation(const_cast<Object3D&>(this->target).getParent(), result);
    }

    void Transform::getWorldTransformation(Matrix4x4& result) {
        Transform::getWorldTransformation(const_cast<Object3D&>(this->target).getParent(), result);
        result.multiply(this->localMatrix);
    }

    void Transform::updateWorldMatrix() {
        this->getWorldTransformation(this->worldMatrix);
    }

    /*
     * This method plays a critical part of performing transformations on scene objects in world space. In order to perform
     * these kinds of transformations, it is necessary to take into account each local transformation of each ancestor of
     * the scene object. If we wanted to apply a world transformation to a single matrix, we would simply pre-multiply that
     * matrix with the desired transformation. With scene objects that are part of a scene hierarchy, we can't do that since
     * the pre-multiplication would have to occur at the top of the hierarchy, and therefore quite likely to a different scene
     * object than the one in question (we only want to modify the transform of the target scene object).
     *
     * We solve this problem by doing some arithmetic to find the equivalent transformation in the scene object's local space
     * that accomplishes the same effect as the world space transformation that would occur on the scene object at the top of
     * the hierarchy:
     *
     *   S = The target scene object.
     *   A = Aggregate/concatenation of all ancestors of S.
     *   L = The local transformation of S.
     *   nWorld = The world space transformation.
     *   nLocal = The transformation in the local space of S.
     *
     *   F = The concatenation of A & L -> A * L
     *   FI = The inverse of F.
     *
     *   We can easily derive a desired world-space transformation that is suited for pre-multiplication. To apply that transformation,
     *   we could simply do: nWorld * F. The problem there is that we'd have to apply that transformation to the top of the hierarchy,
     *   which we cannot do as it would likely affect other scene objects. We find the equivalent transformation in the local space of S (nLocal) by:
     *
     *   	  nWorld * F = F * nLocal
     *   FI * nWorld * F = FI * F * nLocal
     *   			     = nLocal
     *
     *  Therefore the equivalent transformation in the local space of S is: FI * nWorld * F. This method takes in nWorld [worldTransformation]
     *  and produces (FI * nWorld * F) in [localTransformation].
     */
    void Transform::getLocalTransformationFromWorldTransformation(const Matrix4x4& worldTransformation, Matrix4x4& localTransformation) {
        Matrix4x4 fullInverse;
        Transform::getWorldTransformation(localTransformation);
        fullInverse.copy(localTransformation);
        fullInverse.invert();
        localTransformation.preMultiply(worldTransformation);
        localTransformation.preMultiply(fullInverse);
    }

    void Transform::lookAt(const Point3r& target) {
        Point3r cameraPos;
        this->updateWorldMatrix();
        this->transform(cameraPos);

        Vector3r toTarget = target - cameraPos;
        toTarget.normalize();

        Vector3r vUp(0, 1, 0);
        Vector3r vRight;

        Vector3r::cross(toTarget, vUp, vRight);
        vRight.normalize();

        Vector3r::cross(vRight, toTarget, vUp);
        vUp.normalize();

        Matrix4x4 full = this->getLocalMatrix();
        auto fullMat = full.getData();

        fullMat[0] = vRight.x;
        fullMat[1] = vRight.y;
        fullMat[2] = vRight.z;
        fullMat[3] = 0.0f;

        fullMat[4] = vUp.x;
        fullMat[5] = vUp.y;
        fullMat[6] = vUp.z;
        fullMat[7] = 0.0f;

        fullMat[8] = -toTarget.x;
        fullMat[9] = -toTarget.y;
        fullMat[10] = -toTarget.z;
        fullMat[11] = 0.0f;

        fullMat[12] = cameraPos.x;
        fullMat[13] = cameraPos.y;
        fullMat[14] = cameraPos.z;
        fullMat[15] = 1.0f;

        WeakPointer<Object3D> parent = const_cast<Object3D&>(this->target).getParent();

        if (parent.isValid()) {
            parent->getTransform().updateWorldMatrix();
            Matrix4x4 parentMat = parent->getTransform().getWorldMatrix();
            parentMat.invert();
            full.preMultiply(parentMat);
        }

        this->getLocalMatrix().copy(fullMat);
    }

    void Transform::transformBy(const Matrix4x4& mat, TransformationSpace transformationSpace) {
        if (transformationSpace == TransformationSpace::Local) {
            this->localMatrix.multiply(mat);
        }
        else if (transformationSpace == TransformationSpace::PreLocal) {
            this->localMatrix.preMultiply(mat);
        }
        else {
            //Matrix4x4 localTransformation;
            //this->getLocalTransformationFromWorldTransformation(mat, localTransformation);
            //this->localMatrix.multiply(localTransformation); 
            this->localMatrix.preMultiply(mat);
        }
    }

    void Transform::translate(const Vector3<Real>& dir, TransformationSpace transformationSpace) {
        this->translate(dir.x, dir.y, dir.z, transformationSpace);
    }

    void Transform::translate(Real x, Real y, Real z, TransformationSpace transformationSpace) {
        if (transformationSpace == TransformationSpace::Local) {
            this->localMatrix.translate(x, y, z);
        }
        else if (transformationSpace == TransformationSpace::PreLocal) {
            this->localMatrix.preTranslate(x, y, z);
        }
        else {
            Matrix4x4 localTransformation;
            Matrix4x4 worldTransformation;
            worldTransformation.translate(x, y, z);
            this->getLocalTransformationFromWorldTransformation(worldTransformation, localTransformation);
            this->localMatrix.multiply(localTransformation);
            
        }
    }
}
