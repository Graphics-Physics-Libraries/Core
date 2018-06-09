#pragma once

#include "../common/types.h"
#include "../math/Matrix4x4.h"
#include "../scene/Object3D.h"
#include "../geometry/Vector3.h"

namespace Core {

  // forward declarations
  class Engine;

  class Camera : public Object3D {
    friend class Engine;

  public:
    static const UInt32 DEFAULT_FOV;
    static const UInt32 DEFAULT_WIDTH;
    static const UInt32 DEFAULT_HEIGHT;
    static const Real DEFAULT_RATIO;
    static const Real DEFAULT_NEARP;
    static const Real DEFAULT_FARP;

    void setAspectRatio(Real ratio);
    void setAspectRatioFromDimensions(UInt32 width, UInt32 height);
    void updateProjection(Real fov, Real ratio, Real nearP, Real farP);
    const Matrix4x4& getProjectionMatrix() const;
    void lookAt(const Point3r& target);
    void project(Vector3Base<Real>& vec);
    void unProject(Vector3Base<Real>& vec);

    static void buildPerspectiveProjectionMatrix(Real fov, Real ratio,
                                                 Real nearP, Real farP, Matrix4x4& out);
private: 
    Camera();
    Camera(Real fov, Real ratio, Real nearP, Real farP);

    Real fov;
    Real aspectRatio;
    Real nearP;
    Real farP;
    Matrix4x4 projectionMatrix;

  };
}
