#pragma once

#include "../common/types.h"
#include "../base/VectorStorage.h"
#include "Vector4Components.h"

namespace Core {

  #define VECTOR4_COMPONENT_COUNT 4

  template <typename T, bool customStorage = false, typename Enable = void> class Vector4;

  template <typename T, bool customStorage>
  class Vector4<T, customStorage, Core::enable_if_t<Core::is_numeric<T>::value>> : public VectorStorage<T, VECTOR4_COMPONENT_COUNT, customStorage>, public Vector4Components<T> {

  public:

    Vector4(): Vector4(0.0, 0.0, 0.0, 0.0) {}
    Vector4(const T& x, const T& y, const T& z, const T& w): Vector4Components<T>(this->data, x, y, z, w) {}
    Vector4(T* storage): Vector4(storage, 0.0, 0.0, 0.0, 0.0) {}
    Vector4(T* storage, const T& x, const T& y, const T& z, const T& w):
    VectorStorage<T, VECTOR4_COMPONENT_COUNT, true>(storage), Vector4Components<T>(this->data, x, y, z, w) {}

  };

  typedef Vector4<Real, false> Vector4r;
  typedef Vector4<Real, true> Vector4rs;

  typedef Vector4<Int32, false> Vector4i;
  typedef Vector4<Int32, true> Vector4is;

  typedef Vector4<UInt32, false> Vector4u;
  typedef Vector4<UInt32, true> Vector4us;
}