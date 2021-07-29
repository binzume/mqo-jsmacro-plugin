#pragma once
#include <vector>

#include "geometry.h"
namespace geom {

struct empty_t {};

template <typename T, typename O = empty_t>
class Polygon {
  using TPlane = PlaneT<T>;
  using TVertex = Vector3T<T>;

 public:
  std::vector<TVertex> vertices;
  TPlane plane;
  [[no_unique_address]] O opaque;

  Polygon(const std::vector<TVertex> &v, const O &o, const TPlane &p)
      : vertices(v), opaque(o), plane(p) {}

  Polygon(const std::vector<TVertex> &v, const O &o = O())
      : vertices(v), opaque(o) {
    plane = TPlane::fromPoints(v[0], v[1], v[2]);
  }

  // split this polygon by the plane.
  // refs: https://github.com/evanw/csg.js/
  int split(const TPlane &splane, std::vector<Polygon<T, O>> &coplanar_front,
            std::vector<Polygon<T, O>> &coplanar_back,
            std::vector<Polygon<T, O>> &front, std::vector<Polygon<T, O>> &back,
            T eps = 0) const {
    int type_sum = 0;
    std::vector<int> types(vertices.size());
    for (size_t i = 0; i < vertices.size(); i++) {
      types[i] = splane.check(vertices[i], eps);
      type_sum |= types[i];
    }
    switch (type_sum) {
      case TPlane::COPLANAR:
        (splane.normal.dot(plane.normal) > 0 ? coplanar_front : coplanar_back)
            .push_back(*this);
        break;
      case TPlane::FRONT:
        front.push_back(*this);
        break;
      case TPlane::BACK:
        back.push_back(*this);
        break;
      case TPlane::FRONT | TPlane::BACK:
        std::vector<TVertex> f, b;
        size_t n = vertices.size();
        for (size_t i = 0; i < n; i++) {
          size_t j = (i + 1) % n;
          int ti = types[i], tj = types[j];
          auto vi = vertices[i], vj = vertices[j];
          if (ti != TPlane::BACK) f.push_back(vi);
          if (ti != TPlane::FRONT) b.push_back(vi);
          if ((ti | tj) == (TPlane::FRONT | TPlane::BACK)) {
            T t =
                (splane.w - splane.normal.dot(vi)) / splane.normal.dot(vj - vi);
            auto v = vi.lerp(vj, t);
            f.push_back(v);
            b.push_back(v);
          }
        }
        if (f.size() >= 3) front.push_back(Polygon(f, opaque, plane));
        if (b.size() >= 3) back.push_back(Polygon(b, opaque, plane));
        break;
    }
    return type_sum;
  }
};
template <typename T, typename O>
static std::ostream &operator<<(std::ostream &ost, const Polygon<T, O> &p) {
  ost << "{N" << p.plane.normal << ",V[";
  for (const auto &v : p.vertices) ost << v;
  ost << "]}";
  return ost;
}

template <typename TPlane>
class BSPNodeT {
  BSPNodeT<TPlane> *front = nullptr;
  BSPNodeT<TPlane> *back = nullptr;
  TPlane plane;

 public:
  using TElement = TPlane::TElement;

  BSPNodeT() {}
  template <typename TPolygon>
  BSPNodeT(const std::vector<TPolygon> &polygons, TElement eps = 0) {
    build(polygons, eps);
  }

  template <typename TPolygon>
  void build(const std::vector<TPolygon> &polygons, TElement eps = 0) {
    // TODO: build() multiple times.
    if (!plane.isValid() && polygons.size() > 0) {
      plane = polygons[0].plane;
    }
    std::vector<TPolygon> f, b, unused;
    for (const auto &p : polygons) {
      p.split(plane, unused, unused, f, b, eps);
      unused.clear();
    }
    if (f.size() > 0) front = new BSPNodeT<TPlane>(f, eps);
    if (b.size() > 0) back = new BSPNodeT<TPlane>(b, eps);
  }

  // TODO: output_iterator<TPolygon>
  template <typename TPolygon>
  void splitPolygons(const std::vector<TPolygon> &polygons,
                     std::vector<TPolygon> &inner, std::vector<TPolygon> &outer,
                     TElement eps = 0) const {
    std::vector<TPolygon> tmp_f, tmp_b;
    std::vector<TPolygon> &f = front ? tmp_f : outer, &b = back ? tmp_b : inner;
    for (const auto &p : polygons) {
      p.split(plane, f, b, f, b, eps);
    }
    if (front && f.size() > 0) {
      front->splitPolygons(f, inner, outer, eps);
    }
    if (back && b.size() > 0) {
      back->splitPolygons(b, inner, outer, eps);
    }
  }

  bool raycast(const RayT<TElement> &ray, Vector3T<TElement> &intersection) {
    if (check(ray.origin, 0) != TPlane::FRONT) {
      intersection = ray.origin;
      return true;
    }

    // TODO
    // if (plane.raycast(ray, intersection)) {...}
    return false;
  }

  // returns TPlane::FRONT, BACK or COPLANAR
  int check(const Vector3T<TElement> &v, TElement eps = 0) {
    int fb = plane.check(v, eps);
    if (fb == TPlane::BACK) {
      return back ? back->check(v, eps) : fb;
    } else if (fb == TPlane::FRONT) {
      return front ? front->check(v, eps) : fb;
    }
    int f = front ? front->check(v, eps) : TPlane::FRONT;
    int b = back ? back->check(v, eps) : TPlane::BACK;
    return f == b ? f : fb;
  }

  ~BSPNodeT() {
    if (front) delete front;
    if (back) delete back;
  }
};

typedef BSPNodeT<Plane> BSPNode;

}  // namespace geom
