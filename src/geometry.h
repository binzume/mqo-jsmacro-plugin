#pragma once

#include <array>
#include <cmath>
#include <format>
#include <iostream>
#include <limits>
#include <string>

namespace geom {

template <typename T>
struct Vector3T {
  T x, y, z;

  Vector3T<T> operator+(const Vector3T<T> &v) const {
    return Vector3T<T>{x + v.x, y + v.y, z + v.z};
  }
  Vector3T<T> operator-(const Vector3T<T> &v) const {
    return Vector3T<T>{x - v.x, y - v.y, z - v.z};
  }
  Vector3T<T> operator*(const T v) const {
    return Vector3T<T>{x * v, y * v, z * v};
  }
  Vector3T<T> operator/(const T v) const {
    return Vector3T<T>{x / v, y / v, z / v};
  }
  Vector3T<T> operator-() const { return Vector3T<T>{-x, -y, -z}; }
  bool operator==(const Vector3T<T> &v) const {
    return x == v.x && y == v.y && z == v.z;
  }

  T length() const { return std::sqrt(x * x + y * y + z * z); }
  T lengthSqr() const { return x * x + y * y + z * z; }
  T dot(const Vector3T<T> &v) const { return x * v.x + y * v.y + z * v.z; }
  Vector3T<T> cross(const Vector3T<T> &v) const {
    return Vector3T<T>{y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x};
  }
  Vector3T<T> normalized() const { return *this / length(); }
  Vector3T<T> lerp(const Vector3T<T> &v, T t) const {
    return Vector3T<T>{*this + (v - *this) * t};
  }
  std::string to_string() const { return std::format("({},{},{})", x, y, z); }
};

template <typename T>
struct QuaternionT {
  T x = 0, y = 0, z = 0, w = 1;  // i,j,k,1

  QuaternionT<T> operator*(T v) const {
    return QuaternionT<T>{x * v, y * v, z * v, w / v};
  }
  QuaternionT<T> operator/(T v) const {
    return QuaternionT<T>{x / v, y / v, z / v, w / v};
  }
  QuaternionT<T> operator*(const QuaternionT<T> &q) const {
    return QuaternionT<T>{
        w * q.x + x * q.w + y * q.z - z * q.y,  // i
        w * q.y - x * q.z + y * q.w + z * q.x,  // j
        w * q.z + x * q.y - y * q.x + z * q.w,  // k
        w * q.w - x * q.x - y * q.y - z * q.z,  // 1
    };
  }
  bool operator==(const QuaternionT<T> &v) const {
    return x == v.x && y == v.y && z == v.z && w == v.w;
  }

  T length() const { return std::sqrt(x * x + y * y + z * z + w * w); }
  T lengthSqr() const { return x * x + y * y + z * z + w * w; }
  T dot(const QuaternionT<T> &v) const {
    return x * v.x + y * v.y + z * v.z + w * v.w;
  }
  QuaternionT<T> normalized() const { return *this / length(); }

  Vector3T<T> applyTo(const Vector3T<T> &v) const {
    T ix = w * v.x + y * v.z - z * v.y;
    T iy = w * v.y + z * v.x - x * v.z;
    T iz = w * v.z + x * v.y - y * v.x;
    T iw = -x * v.x - y * v.y - z * v.z;
    return Vector3T<T>{ix * w + iw * -x + iy * -z - iz * -y,
                       iy * w + iw * -y + iz * -x - ix * -z,
                       iz * w + iw * -z + ix * -y - iy * -x};
  }

  static QuaternionT<T> fromAxisAngle(const Vector3T<T> &ax, T rad) {
    auto s = std::sin(rad / 2);
    return QuaternionT<T>(ax.x * s, ax.y * s, ax.z * s, std::cos(rad / 2));
  }
  std::string to_string() const {
    return std::format("({},{},{},{})", x, y, z, w);
  }
};

template <typename T>
struct Matrix4T {
  std::array<T, 16> mat = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

  T operator[](int i) const { return mat[i]; }
  T &operator[](int i) { return mat[i]; }
  Matrix4T<T> operator*(const Matrix4T<T> &b) const {
    Matrix4T<T> r;
    auto &a = *this;
    r[0] = a[0] * b[0] + a[1] * b[4] + a[2] * b[8] + a[3] * b[12];
    r[1] = a[0] * b[1] + a[1] * b[5] + a[2] * b[9] + a[3] * b[13];
    r[2] = a[0] * b[2] + a[1] * b[6] + a[2] * b[10] + a[3] * b[14];
    r[3] = a[0] * b[3] + a[1] * b[7] + a[2] * b[11] + a[3] * b[15];
    r[4] = a[4] * b[0] + a[5] * b[4] + a[6] * b[8] + a[7] * b[12];
    r[5] = a[4] * b[1] + a[5] * b[5] + a[6] * b[9] + a[7] * b[13];
    r[6] = a[4] * b[2] + a[5] * b[6] + a[6] * b[10] + a[7] * b[14];
    r[7] = a[4] * b[3] + a[5] * b[7] + a[6] * b[11] + a[7] * b[15];
    r[8] = a[8] * b[0] + a[9] * b[4] + a[10] * b[8] + a[11] * b[12];
    r[9] = a[8] * b[1] + a[9] * b[5] + a[10] * b[9] + a[11] * b[13];
    r[10] = a[8] * b[2] + a[9] * b[6] + a[10] * b[10] + a[11] * b[14];
    r[11] = a[8] * b[3] + a[9] * b[7] + a[10] * b[11] + a[11] * b[15];
    r[12] = a[12] * b[0] + a[13] * b[4] + a[14] * b[8] + a[15] * b[12];
    r[13] = a[12] * b[1] + a[13] * b[5] + a[14] * b[9] + a[15] * b[13];
    r[14] = a[12] * b[2] + a[13] * b[6] + a[14] * b[10] + a[15] * b[14];
    r[15] = a[12] * b[3] + a[13] * b[7] + a[14] * b[11] + a[15] * b[15];
    return r;
  }
  bool operator==(const Matrix4T<T> &m) const { return mat == m.mat; }
  Vector3T<T> applyTo(const Vector3T<T> &v) const {
    return Vector3T<T>{
        mat[0] * v.x + mat[1] * v.y + mat[2] * v.z + mat[3] + mat[12],
        mat[4] * v.x + mat[5] * v.y + mat[6] * v.z + mat[7] + mat[13],
        mat[8] * v.x + mat[9] * v.y + mat[10] * v.z + mat[11] + mat[14]};
  }
  std::string to_string() const {
    return std::format("[{},{},{},{}, {},{},{},{}, {},{},{},{}, {},{},{},{}]",
                       mat[0], mat[1], mat[2], mat[3], mat[4], mat[5], mat[6],
                       mat[7], mat[8], mat[9], mat[10], mat[11], mat[12],
                       mat[13], mat[14], mat[15]);
  }
};

template <typename T>
struct PlaneT {
  typedef T TElement;
  static const int COPLANAR = 0;
  static const int FRONT = 1;
  static const int BACK = 2;

  Vector3T<T> normal = {0, 0, 0};  // NOTE: invalid normal.
  T w = 0;

  PlaneT() {}
  PlaneT(const Vector3T<T> &_n, const T &_w) : normal(_n), w(_w) {}

  PlaneT<T> flipped() const { return PlaneT<T>{-normal, -w}; }

  static PlaneT<T> fromPoints(const Vector3T<T> &a, const Vector3T<T> &b,
                              const Vector3T<T> &c) {
    auto n = (b - a).cross(c - a).normalized();
    return PlaneT<T>(n, n.dot(a));
  }
  int check(const Vector3T<T> &v, T eps = 0) const {
    T t = signedDistanceTo(v);
    return (t < -eps) ? BACK : (t > eps) ? FRONT : COPLANAR;
  }
  T distanceTo(const Vector3T<T> &v) const {
    return std::abs(signedDistanceTo(v));
  }
  T signedDistanceTo(const Vector3T<T> &v) const { return normal.dot(v) - w; }
  std::string to_string() const {
    return std::format("[{},{}]", normal.to_string(), w);
  }
  bool isValid() const { return normal.lengthSqr() > 0; }
};

template <typename T>
struct RayT {
  Vector3T<T> origin = {0, 0, 0};
  Vector3T<T> direction = {0, 0, 1};

  T distanceToSqr(const Vector3T<T> &point) const {
    T d = (point - origin).dot(direction);
    auto o = (d <= 0) ? origin : (origin + direction * d);
    return (point - o).lengthSqr();
  }
  T distanceTo(const Vector3T<T> &point) const {
    return std::sqrt(distanceToSqr(point));
  }
  T distanceTo(const PlaneT<T> &plane) const {
    T d = plane.normal.dot(direction);
    if (d == 0) {
      return std::numeric_limits<T>::lowest();  // TODO: -infinity
    }
    return -plane.signedDistanceTo(origin) / d;
  }
  bool intersects(const PlaneT<T> &plane, Vector3T<T> &result) const {
    T len = distanceTo(plane);
    if (len < 0) {
      return false;
    }
    result = direction * len + origin;
    return true;
  }
  std::string to_string() const {
    return std::format("[{},{}]", origin.to_string(), direction.to_string());
  }
};

template <typename T>
static std::ostream &operator<<(std::ostream &ost, const Vector3T<T> &v) {
  return ost << v.to_string();
}

typedef double FloatType;
typedef Vector3T<FloatType> Vector3;
typedef Matrix4T<FloatType> Matrix4;
typedef QuaternionT<FloatType> Quaternion;
typedef PlaneT<FloatType> Plane;
typedef RayT<FloatType> Ray;

}  // namespace geom
