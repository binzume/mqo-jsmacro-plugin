

#include < unordered_map>
#include <functional>
#include <vector>

#include "Utils.h"
#include "bsptree.h"
#include "qjsutils.h"

using namespace std;

const double MIN_EPSILON = 1e-10;
const double DEFAULT_EPSILON = 1e-6;
typedef geom::Polygon<double, JSValue> JSPolygon;

template <>
struct std::hash<geom::Vector3> {
  std::size_t operator()(const geom::Vector3& v) const {
    std::hash<double> dh;
    std::hash<size_t> sh;
    return sh(sh(dh(v.x)) ^ dh(v.y)) ^ dh(v.y);
  }
};

JSValue ToJSValue(JSContext* ctx, const geom::Vector3& v) {
  ValueHolder obj(ctx);
  obj.Set("x", v.x);
  obj.Set("y", v.y);
  obj.Set("z", v.z);
  return unwrap(std::move(obj));
}

JSValue ToJSValue(JSContext* ctx, const JSPolygon& p,
                  unordered_map<geom::Vector3, JSValue>& vcache) {
  ValueHolder obj(ctx);
  ValueHolder vertices(ctx, JS_NewArray(ctx));
  for (uint32_t i = 0; i < p.vertices.size(); i++) {
    auto c = vcache.find(p.vertices[i]);
    if (c == vcache.end()) {
      JSValue v = ToJSValue(ctx, p.vertices[i]);
      vcache[p.vertices[i]] = v;
      vertices.Set(i, v);
    } else {
      vertices.Set(i, JS_DupValue(ctx, c->second));
    }
  }
  // TODO: shared, plane
  obj.Set("vertices", vertices);
  obj.Set("src", JS_DupValue(ctx, p.opaque));
  return unwrap(std::move(obj));
}

geom::Vector3 ToVector3(ValueHolder&& v) {
  return geom::Vector3(v["x"].To<double>(), v["y"].To<double>(),
                       v["z"].To<double>());
}

geom::Plane ToPlane(ValueHolder&& v) {
  return geom::Plane(ToVector3(v["normal"]), v["w"].To<double>());
}

JSPolygon ToPolygon(ValueHolder&& v) {
  auto vv = v["vertices"];
  uint32_t sz = vv.Length();
  vector<geom::Vector3> vertices(sz);
  for (uint32_t i = 0; i < sz; i++) {
    vertices[i] = ToVector3(vv[i]);
  }

  JSValue o = v.GetValueNoDup();  // JS_DupValue in ToJSValue
  return JSPolygon(vertices, o, ToPlane(v["plane"]));
}

void ToPolygons(ValueHolder&& v, vector<JSPolygon>& polygons) {
  uint32_t sz = v.Length();
  for (uint32_t i = 0; i < sz; i++) {
    polygons.push_back(ToPolygon(v[i]));
  }
}

void ToJSArray(const vector<JSPolygon>& polygons, ValueHolder&& v,
               unordered_map<geom::Vector3, JSValue>& vcache) {
  uint32_t offset = v.Length();
  for (uint32_t i = 0; i < polygons.size(); i++) {
    v.Set(i + offset, ToJSValue(v.ctx, polygons[i], vcache));
  }
}

class JSBSPTree : public JSClassBase<JSBSPTree> {
 public:
  static const JSCFunctionListEntry proto_funcs[];

  geom::BSPNode node;
  JSBSPTree(JSContext* ctx, JSValueConst this_val, int argc,
            JSValueConst* argv) {
    if (argc > 0) {
      Build(ctx, argv[0], DEFAULT_EPSILON);
    }
  }

  void Build(JSContext* ctx, JSValueConst src, double eps) {
    vector<JSPolygon> polygons;
    eps = fmax(std::isnan(eps) ? DEFAULT_EPSILON : eps, MIN_EPSILON);
    ToPolygons(ValueHolder(ctx, src, true), polygons);
    node.build(polygons, eps);
  }

  JSValue SplitPolygons(JSContext* ctx, JSValueConst src, JSValueConst in,
                        JSValueConst out, double eps) {
    if (!JS_IsArray(ctx, src)) {
      return JS_EXCEPTION;
    }
    eps = fmax(std::isnan(eps) ? DEFAULT_EPSILON : eps, MIN_EPSILON);

    vector<JSPolygon> polygons;
    ToPolygons(ValueHolder(ctx, src, true), polygons);
    vector<JSPolygon> inner;
    vector<JSPolygon> outer;
    node.splitPolygons(polygons, inner, outer, eps);
    unordered_map<geom::Vector3, JSValue> vcache;
    if (JS_IsArray(ctx, in)) {
      ToJSArray(inner, ValueHolder(ctx, in, true), vcache);
    }
    if (JS_IsArray(ctx, out)) {
      ToJSArray(outer, ValueHolder(ctx, out, true), vcache);
    }
    return JS_UNDEFINED;
  }

  JSValue ClipPolygons(JSContext* ctx, JSValueConst src, bool returnInner,
                       double eps) {
    if (!JS_IsArray(ctx, src)) {
      return JS_EXCEPTION;
    }
    eps = fmax(std::isnan(eps) ? DEFAULT_EPSILON : eps, MIN_EPSILON);

    ValueHolder pp(ctx, src, true);
    uint32_t sz = pp.Length();
    JSValue arr = JS_NewArray(ctx);
    ValueHolder ret(ctx, arr);
    unordered_map<geom::Vector3, JSValue> vcache;
    for (uint32_t i = 0; i < sz; i++) {
      vector<JSPolygon> inner;
      vector<JSPolygon> outer;
      vector<JSPolygon> polygons{ToPolygon(pp[i])};
      node.splitPolygons(polygons, inner, outer, eps);
      if ((returnInner ? outer : inner).size() == 0) {
        ret.Set(ret.Length(), pp[i]);
      } else {
        ToJSArray(returnInner ? inner : outer, ValueHolder(ctx, arr, true),
                  vcache);
      }
    }
    return unwrap(std::move(ret));
  }

  JSValue Raycast(JSContext* ctx, JSValueConst rayobj, double eps) {
    if (!JS_IsObject(rayobj)) {
      return JS_EXCEPTION;
    }
    eps = fmax(std::isnan(eps) ? DEFAULT_EPSILON : eps, MIN_EPSILON);
    ValueHolder r(ctx, rayobj, true);
    geom::Ray ray(ToVector3(r["origin"]), ToVector3(r["direction"]));
    geom::Vector3 result;
    if (node.raycast(ray, result, eps)) {
      return ToJSValue(ctx, result);
    }
    return JS_NULL;
  }

  // returns 0:coplanar, 1:out, 2:in
  int ClassifyPoint(JSContext* ctx, JSValueConst v, double eps) {
    return node.classifyPoint(ToVector3(ValueHolder(ctx, v, true)), eps);
  }
};

const JSCFunctionListEntry JSBSPTree::proto_funcs[] = {
    function_entry<&Build>("build"),
    function_entry<&ClassifyPoint>("classifyPoint"),
    function_entry<&SplitPolygons>("splitPolygons"),
    function_entry<&ClipPolygons>("clipPolygons"),
    function_entry<&Raycast>("raycast"),
};

static int ModuleInit(JSContext* ctx, JSModuleDef* m) {
  return JS_SetModuleExport(ctx, m, "BSPTree",
                            newClassConstructor<JSBSPTree>(ctx, "BSPTree"));
}

JSModuleDef* InitBSPTreeModule(JSContext* ctx) {
  JSModuleDef* m;
  m = JS_NewCModule(ctx, "bsptree", ModuleInit);
  if (!m) {
    return NULL;
  }
  JS_AddModuleExport(ctx, m, "BSPTree");
  return m;
}
