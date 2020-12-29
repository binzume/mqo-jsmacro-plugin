
#include <windows.h>

#include <map>
#include <sstream>
#include <vector>

#include "MQWidget.h"
#include "Utils.h"
#include "qjsutils.h"

//---------------------------------------------------------------------------------------------------------------------
// Vertics
//---------------------------------------------------------------------------------------------------------------------

JSValue NewVec3(JSContext* ctx, MQPoint p) {
  ValueHolder v(ctx);
  v.Set("x", p.x);
  v.Set("y", p.y);
  v.Set("z", p.z);
  return unwrap(std::move(v));
}

MQPoint ToMQPoint(JSContext* ctx, JSValueConst value) {
  ValueHolder v(ctx, value, true);
  if (v.IsArray() && v.Length() == 3) {
    return MQPoint(v[0U].To<float>(), v[1U].To<float>(), v[2U].To<float>());
  }
  return MQPoint(v["x"].To<float>(), v["y"].To<float>(), v["z"].To<float>());
}

class VertexArray {
 public:
  MQObject obj;
  VertexArray(MQObject obj) : obj(obj) {}

  static JSClassID class_id;
  static const JSCFunctionListEntry proto_funcs[];

  int Length() { return obj->GetVertexCount(); }

  int Append(JSContext* ctx, JSValueConst arg0, JSValueConst arg1,
             JSValueConst arg2) {
    MQPoint p;
    if (JS_IsNumber(arg0)) {
      p.x = convert_jsvalue<float>(ctx, arg0);
      p.y = convert_jsvalue<float>(ctx, arg1);
      p.z = convert_jsvalue<float>(ctx, arg2);
    } else {
      p = ToMQPoint(ctx, arg0);
    }
    return obj->AddVertex(p);
  }

  JSValue GetVertex(JSContext* ctx, int index) {
    MQPoint p = obj->GetVertex(index);
    ValueHolder v(ctx);
    v.Set("x", p.x);
    v.Set("y", p.y);
    v.Set("z", p.z);
    v.Set("id", obj->GetVertexUniqueID(index));
    v.Set("refs", obj->GetVertexRefCount(index));
    return unwrap(std::move(v));
  }

  void SetVertex(JSContext* ctx, JSValueConst value, int index) {
    obj->SetVertex(index, ToMQPoint(ctx, value));
  }
  bool DeleteVertex(JSContext* ctx, JSValueConst value) {
    obj->DeleteVertex(convert_jsvalue<int>(ctx, value));
    return true;
  }
};

template <auto method>
int delete_property_handler(JSContext* ctx, JSValueConst obj, JSAtom prop) {
  JSValue v = JS_AtomToValue(ctx, prop);
  JSValue ret = invoke_function(method, ctx, obj, 1, &v);
  int reti = convert_jsvalue<int>(ctx, ret);
  JS_FreeValue(ctx, v);
  JS_FreeValue(ctx, ret);
  return reti;
}

static JSClassExoticMethods VertexArray_exotic{
    .get_own_property = indexed_propery_handler<&VertexArray::GetVertex,
                                                &VertexArray::SetVertex>,
    .delete_property = delete_property_handler<&VertexArray::DeleteVertex>};

JSClassID VertexArray::class_id;

const JSCFunctionListEntry VertexArray::proto_funcs[] = {
    function_entry_getset<&Length>("length"),
    function_entry<&Append>("append"),
    function_entry<&Append>("push"),
};

JSValue NewVertexArray(JSContext* ctx, MQObject o) {
  JSValue obj = JS_NewObjectClass(ctx, VertexArray::class_id);
  if (JS_IsException(obj)) return obj;
  JS_SetOpaque(obj, new VertexArray(o));
  return obj;
}

//---------------------------------------------------------------------------------------------------------------------
// Faces
//---------------------------------------------------------------------------------------------------------------------

class FaceWrapper {
 public:
  MQObject obj;
  int index;
  FaceWrapper(MQObject obj, int idx) : obj(obj), index(idx) {}

  static JSClassID class_id;
  static const JSCFunctionListEntry proto_funcs[];

  int GetIndex() { return index; }
  int GetId() { return obj->GetFaceUniqueID(index); }
  int GetMaterial() { return obj->GetFaceMaterial(index); }
  void SetMaterial(int m) { obj->SetFaceMaterial(index, m); }
  bool GetVisible() { return obj->GetFaceVisible(index); }
  void SetVisible(bool v) { obj->SetFaceVisible(index, v); }
  JSValue GetPoints(JSContext* ctx) {
    ValueHolder points(ctx, JS_NewArray(ctx));
    int count = obj->GetFacePointCount(index);
    int* vv = new int[count];
    obj->GetFacePointArray(index, vv);
    for (int i = 0; i < count; i++) {
      points.Set(i, vv[i]);
    }
    delete[] vv;
    return points.GetValue();
  }
  JSValue GetUV(JSContext* ctx) {
    ValueHolder points(ctx, JS_NewArray(ctx));
    int count = obj->GetFacePointCount(index);
    MQCoordinate* uva = new MQCoordinate[count];
    obj->GetFaceCoordinateArray(index, uva);
    for (int i = 0; i < count; i++) {
      ValueHolder u(ctx);
      u.Set("u", uva[i].u);
      u.Set("v", uva[i].v);
      points.Set(i, u);
    }
    delete[] uva;
    return points.GetValue();
  }
  void Invert() { obj->InvertFace(index); }
};

JSClassID FaceWrapper::class_id;

const JSCFunctionListEntry FaceWrapper::proto_funcs[] = {
    function_entry_getset<&GetIndex>("index"),
    function_entry_getset<&GetId>("id"),
    function_entry_getset<&GetMaterial, &SetMaterial>("material"),
    function_entry_getset<&GetVisible, &SetVisible>("visible"),
    function_entry_getset<&GetPoints>("points"),
    function_entry_getset<&GetUV>("uv"),
    function_entry<&Invert>("invert"),
};

JSValue NewFaceWrapper(JSContext* ctx, MQObject o, int index) {
  JSValue obj = JS_NewObjectClass(ctx, FaceWrapper::class_id);
  if (JS_IsException(obj)) return obj;
  JS_SetOpaque(obj, new FaceWrapper(o, index));
  return obj;
}

class FaceArray {
 public:
  MQObject obj;
  FaceArray(MQObject obj) : obj(obj) {}

  static JSClassID class_id;
  static const JSCFunctionListEntry proto_funcs[];

  int Length() { return obj->GetFaceCount(); }

  int AddFace(JSContext* ctx, JSValueConst points, int mat) {
    ValueHolder v(ctx, points, true);
    uint32_t count = v.Length();
    int* indices = new int[count];
    for (uint32_t i = 0; i < count; i++) {
      indices[i] = v[i].To<int32_t>();
    }

    int f = obj->AddFace(count, indices);
    delete[] indices;
    obj->SetFaceMaterial(f, mat);
    return f;
  }

  JSValue GetFace(JSContext* ctx, int index) {
    if (obj->GetFacePointCount(index) == 0) {
      return JS_UNDEFINED;
    }
    return NewFaceWrapper(ctx, obj, index);
  }

  void SetFace(JSContext* ctx, JSValueConst value, int index) {
    ValueHolder face(ctx, value, true);
    auto points = face["points"];
    if (points.IsArray()) {
      obj->DeleteFace(index);
      uint32_t count = points.Length();
      int* pp = new int[count];
      for (uint32_t i = 0; i < count; i++) {
        pp[i] = points[i].To<int>();
      }
      obj->InsertFace(index, count, pp);
      delete[] pp;
    }
    auto mat = face["material"];
    if (!mat.IsUndefined()) {
      obj->SetFaceMaterial(index, mat.To<int>());
    }
  }
  bool DeleteFace(JSContext* ctx, JSValueConst value) {
    return obj->DeleteFace(convert_jsvalue<int>(ctx, value));
  }
};

static JSClassExoticMethods FaceArray_exotic{
    .get_own_property =
        indexed_propery_handler<&FaceArray::GetFace, &FaceArray::SetFace>,
    .delete_property = delete_property_handler<&FaceArray::DeleteFace>};

JSClassID FaceArray::class_id;

const JSCFunctionListEntry FaceArray::proto_funcs[] = {
    function_entry_getset<&Length>("length"),
    function_entry<&AddFace>("append"),
    function_entry<&AddFace>("push"),
};

JSValue NewFaceArray(JSContext* ctx, MQObject o) {
  JSValue obj = JS_NewObjectClass(ctx, FaceArray::class_id);
  if (JS_IsException(obj)) return obj;
  JS_SetOpaque(obj, new FaceArray(o));
  return obj;
}

//---------------------------------------------------------------------------------------------------------------------
// MQObject
//---------------------------------------------------------------------------------------------------------------------
JSValue NewMQObject(JSContext* ctx, MQObject o, int index);

class MQObjectWrapper {
 public:
  int index;
  MQObject obj;
  MQObjectWrapper(MQObject obj, int index) : index(index), obj(obj) {}
  MQObjectWrapper() : index(-1), obj(MQ_CreateObject()) {}
  ~MQObjectWrapper() {
    if (index < 0) {
      obj->DeleteThis();
    }
  }

  void Init(JSContext* ctx, JSValueConst this_obj, int argc,
            JSValueConst* argv) {
    JS_SetPropertyStr(ctx, this_obj, "verts", NewVertexArray(ctx, obj));
    JS_SetPropertyStr(ctx, this_obj, "faces", NewFaceArray(ctx, obj));
    if (argc > 0 && JS_IsString(argv[0])) {
      SetName(convert_jsvalue<std::string>(ctx, argv[0]));
    }
  }

  void Reset() {
    obj = MQ_CreateObject();
    index = -1;
  }

  static JSClassID class_id;
  static const JSCFunctionListEntry proto_funcs[];

  int GetIndex() { return index; }
  int GetId() { return obj->GetUniqueID(); }
  int GetType() { return obj->GetType(); }
  void SetType(int t) { obj->SetType(t); }
  bool Selected() { return obj->GetSelected(); }
  void SetSelected(bool s) { obj->SetSelected(s); }
  bool Visible() { return obj->GetVisible(); }
  void SetVisible(bool v) { obj->SetVisible(v); }
  bool Locked() { return obj->GetLocking(); }
  void SetLocked(bool v) { obj->SetLocking(v); }
  std::string GetName() {
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
    return converter.to_bytes(obj->GetNameW());
  }
  JSValue SetName(const std::string &name) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
    obj->SetName(converter.from_bytes(name).c_str());
    return JS_UNDEFINED;
  }
  void Compact() { obj->Compact(); }
  void Clear() { obj->Clear(); }
  void Freeze(int flags) {
    obj->Freeze(flags == 0 ? MQOBJECT_FREEZE_ALL : flags);
  }
  void Merge(JSValue src) {
    MQObjectWrapper* o =
        (MQObjectWrapper*)JS_GetOpaque(src, MQObjectWrapper::class_id);
    if (o == nullptr) {
      return;
    }
    obj->Merge(o->obj);
  }
  JSValue Clone(JSContext* ctx, bool reg) {
    return NewMQObject(ctx, obj->Clone(), -1);
  }
  void OptimizeVertex(float distance) {
    obj->OptimizeVertex(distance, nullptr);
  }
};

JSClassID MQObjectWrapper::class_id;

const JSCFunctionListEntry MQObjectWrapper::proto_funcs[] = {
    function_entry_getset<&GetIndex>("index"),
    function_entry_getset<&GetId>("id"),
    function_entry_getset<&GetName, &SetName>("name"),
    function_entry_getset<&GetType, &SetType>("type"),
    function_entry_getset<&Visible, &SetVisible>("visible"),
    function_entry_getset<&Selected, &SetSelected>("selected"),
    function_entry_getset<&Locked, &SetLocked>("locked"),
    function_entry<&Compact>("compact"),
    function_entry<&Clear>("clear"),
    function_entry<&Freeze>("freeze"),
    function_entry<&Merge>("merge"),
    function_entry<&Clone>("clone"),
    function_entry<&Clone>("optimizeVertex"),
};

JSValue NewMQObject(JSContext* ctx, MQObject o, int index) {
  JSValue obj = JS_NewObjectClass(ctx, MQObjectWrapper::class_id);
  if (JS_IsException(obj)) return obj;
  MQObjectWrapper* p = new MQObjectWrapper(o, index);
  JS_SetOpaque(obj, p);
  p->Init(ctx, obj, 0, nullptr);
  return obj;
}

//---------------------------------------------------------------------------------------------------------------------
// MQMaterial
//---------------------------------------------------------------------------------------------------------------------

class MQMaterialWrapper {
 public:
  int index;
  MQMaterial mat;
  MQMaterialWrapper(MQMaterial mat, int index) : index(index), mat(mat) {}
  MQMaterialWrapper() : index(-1), mat(MQ_CreateMaterial()) {}
  ~MQMaterialWrapper() {
    if (index < 0) {
      mat->DeleteThis();
    }
  }

  static JSClassID class_id;
  static const JSCFunctionListEntry proto_funcs[];

  void Init(JSContext* ctx, JSValueConst this_obj, int argc,
            JSValueConst* argv) {
    if (argc > 0 && JS_IsString(argv[0])) {
      SetName(convert_jsvalue<std::string>(ctx, argv[0]));
    }
  }

  void Reset() {
    mat = MQ_CreateMaterial();
    index = -1;
  }

  int GetIndex(JSContext* ctx) { return index; }
  int GetId(JSContext* ctx) { return mat->GetUniqueID(); }
  bool GetSelected() { return mat->GetSelected(); }
  void SetSelected(bool s) { mat->SetSelected(s); }
  std::string GetName(JSContext* ctx) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
    return converter.to_bytes(mat->GetNameW());
  }
  JSValue SetName(std::string name) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
    mat->SetName(converter.from_bytes(name).c_str());
    return JS_UNDEFINED;
  }
  JSValue GetColor(JSContext* ctx) {
    ValueHolder v = FromMQColor(ctx, mat->GetColor());
    v.Set("a", mat->GetAlpha());
    return v.GetValue();
  }
  void SetColor(JSContext* ctx, JSValueConst v) {
    ValueHolder col(ctx, v, true);
    MQColor c = ToMQColor(col);
    mat->SetColor(c);
    ValueHolder a = col["a"];
    if (!a.IsUndefined()) {
      mat->SetAlpha(a.To<float>());
    }
  }
  JSValue GetAmbientColor(JSContext* ctx) {
    return unwrap(FromMQColor(ctx, mat->GetAmbientColor()));
  }
  void SetAmbientColor(JSContext* ctx, JSValueConst col) {
    mat->SetAmbientColor(ToMQColor(ValueHolder(ctx, col, true)));
  }
  JSValue GetEmissionColor(JSContext* ctx) {
    return unwrap(FromMQColor(ctx, mat->GetEmissionColor()));
  }
  void SetEmissionColor(JSContext* ctx, JSValueConst col) {
    mat->SetEmissionColor(ToMQColor(ValueHolder(ctx, col, true)));
  }
  JSValue GetSpecularColor(JSContext* ctx) {
    return unwrap(FromMQColor(ctx, mat->GetSpecularColor()));
  }
  void SetSpecularColor(JSContext* ctx, JSValueConst col) {
    mat->SetSpecularColor(ToMQColor(ValueHolder(ctx, col, true)));
  }

  float GetPower() { return mat->GetPower(); }
  void SetPower(float v) { mat->SetPower(v); }
  float GetAmbient() { return mat->GetAmbient(); }
  void SetAmbient(float v) { mat->SetAmbient(v); }
  float GetEmission() { return mat->GetEmission(); }
  void SetEmission(float v) { mat->SetEmission(v); }
  float GetSpecular() { return mat->GetSpecular(); }
  void SetSpecular(float v) { mat->SetSpecular(v); }
  float GetReflection() { return mat->GetReflection(); }
  void SetReflection(float v) { mat->SetReflection(v); }
  float GetRefraction() { return mat->GetRefraction(); }
  void SetRefraction(float v) { mat->SetRefraction(v); }
  bool GetDoubleSided() { return mat->GetDoubleSided(); }
  void SetDoubleSided(bool v) { mat->SetDoubleSided(v); }

 private:
  MQColor ToMQColor(ValueHolder col) {
    MQColor c;
    c.r = col["r"].To<float>();
    c.g = col["g"].To<float>();
    c.b = col["b"].To<float>();
    return c;
  }
  ValueHolder FromMQColor(JSContext* ctx, MQColor c) {
    ValueHolder v(ctx);
    v.Set("r", c.r);
    v.Set("g", c.g);
    v.Set("b", c.b);
    return v;
  }
};

JSClassID MQMaterialWrapper::class_id;

const JSCFunctionListEntry MQMaterialWrapper::proto_funcs[] = {
    function_entry_getset<&GetIndex>("index"),
    function_entry_getset<&GetId>("id"),
    function_entry_getset<&GetName, &SetName>("name"),
    function_entry_getset<&GetColor, &SetColor>("color"),
    function_entry_getset<&GetAmbientColor, &SetAmbientColor>("ambientColor"),
    function_entry_getset<&GetEmissionColor, &SetEmissionColor>(
        "emissionColor"),
    function_entry_getset<&GetSpecularColor, &SetSpecularColor>(
        "specularColor"),
    function_entry_getset<&GetPower, &SetPower>("power"),
    function_entry_getset<&GetAmbient, &SetAmbient>("ambient"),
    function_entry_getset<&GetEmission, &SetEmission>("emission"),
    function_entry_getset<&GetSpecular, &SetSpecular>("specular"),
    function_entry_getset<&GetReflection, &SetReflection>("reflection"),
    function_entry_getset<&GetRefraction, &SetRefraction>("refraction"),
    function_entry_getset<&GetDoubleSided, &SetDoubleSided>("doubleSided"),
    function_entry_getset<&GetSelected, &SetSelected>("selected"),
};

JSValue NewMQMaterial(JSContext* ctx, MQMaterial mat, int index) {
  JSValue obj = JS_NewObjectClass(ctx, MQMaterialWrapper::class_id);
  if (JS_IsException(obj)) return obj;
  MQMaterialWrapper* p = new MQMaterialWrapper(mat, index);
  JS_SetOpaque(obj, p);
  p->Init(ctx, obj, 0, nullptr);
  return obj;
}

//---------------------------------------------------------------------------------------------------------------------
// MQScene
//---------------------------------------------------------------------------------------------------------------------

class MQSceneWrapper {
  MQScene scene;

 public:
  MQSceneWrapper(MQScene scene) : scene(scene) {}

  static JSClassID class_id;
  static const JSCFunctionListEntry proto_funcs[];

  JSValue GetCameraPosition(JSContext* ctx) {
    return NewVec3(ctx, scene->GetCameraPosition());
  }
  void SetCameraPosition(JSContext* ctx, JSValue v) {
    scene->SetCameraPosition(ToMQPoint(ctx, v));
  }
  JSValue GetCameraLookAt(JSContext* ctx) {
    return NewVec3(ctx, scene->GetLookAtPosition());
  }
  void SetCameraLookAt(JSContext* ctx, JSValue v) {
    scene->SetLookAtPosition(ToMQPoint(ctx, v));
  }
  JSValue GetRotationCenter(JSContext* ctx) {
    return NewVec3(ctx, scene->GetRotationCenter());
  }
  void SetRotationCenter(JSContext* ctx, JSValue v) {
    scene->SetRotationCenter(ToMQPoint(ctx, v));
  }
  float GetZoom() { return scene->GetZoom(); }
  void SetZoom(float v) { scene->SetZoom(v); }
  float GetFOV() { return scene->GetFOV(); }
  void SetFOV(float v) { scene->SetFOV(v); }
  JSValue GetCameraAngle(JSContext* ctx) {
    MQAngle angle = scene->GetCameraAngle();
    ValueHolder v(ctx);
    v.Set("head", angle.head);
    v.Set("pitch", angle.pitch);
    v.Set("bank", angle.bank);
    return v.GetValue();
  }
  void SetCameraAngle(JSContext* ctx, JSValue v) {
    ValueHolder angle(ctx, v, true);
    scene->SetCameraAngle(MQAngle(angle["head"].To<float>(),
                                  angle["pitch"].To<float>(),
                                  angle["bank"].To<float>()));
  }
};

JSClassID MQSceneWrapper::class_id;

const JSCFunctionListEntry MQSceneWrapper::proto_funcs[] = {
    function_entry_getset<&GetCameraPosition, &SetCameraPosition>(
        "cameraPosition"),
    function_entry_getset<&GetCameraLookAt, &SetCameraLookAt>("cameraLookAt"),
    function_entry_getset<&GetCameraAngle, &SetCameraAngle>("cameraAngle"),
    function_entry_getset<&GetRotationCenter, &SetRotationCenter>(
        "rotationCenter"),
    function_entry_getset<&GetZoom, &SetZoom>("zoom"),
    function_entry_getset<&GetFOV, &SetFOV>("fov"),
};

JSValue NewMQScene(JSContext* ctx, MQDocument doc) {
  JSValue obj = JS_NewObjectClass(ctx, MQSceneWrapper::class_id);
  if (JS_IsException(obj)) return obj;
  JS_SetOpaque(obj, new MQSceneWrapper(doc->GetScene(0)));
  return obj;
}

//---------------------------------------------------------------------------------------------------------------------
// MQDocument
//---------------------------------------------------------------------------------------------------------------------

class MQDocumentWrapper {
 public:
  MQDocument doc;
  std::map<std::string, std::string>* pluginKeyValue;
  static JSClassID class_id;
  static const JSCFunctionListEntry proto_funcs[];

  MQDocumentWrapper(MQDocument doc,
                    std::map<std::string, std::string>* keyValue)
      : doc(doc) {
    pluginKeyValue = keyValue;
  }

  void Compact() { doc->Compact(); }
  int GetCurrentObjectIndex() { return doc->GetCurrentObjectIndex(); }
  void SetCurrentObjectIndex(int i) { doc->SetCurrentObjectIndex(i); }
  void ClearSelect(int flags) {
    doc->ClearSelect(flags ? flags : MQDOC_CLEARSELECT_ALL);
  }

  bool IsVertexSelected(int o, int v) { return doc->IsSelectVertex(o, v); }
  bool SetVertexSelected(int o, int v, bool s) {
    return s ? doc->AddSelectVertex(o, v) : doc->DeleteSelectVertex(o, v);
  }
  bool IsFaceSelected(int o, int f) { return doc->IsSelectFace(o, f); }
  bool SetFaceSelected(int o, int f, bool s) {
    return s ? doc->AddSelectFace(o, f) : doc->DeleteSelectFace(o, f);
  }

  JSValue GetScene(JSContext* ctx) { return NewMQScene(ctx, doc); }

  JSValue GetObjects(JSContext* ctx, JSValueConst this_val, int argc,
                     JSValueConst* argv) {
    // TODO: cache
    ValueHolder ret(ctx, JS_NewArray(ctx));
    for (int i = 0; i < doc->GetObjectCount(); i++) {
      auto o = doc->GetObject(i);
      if (o != nullptr) {
        ret.SetFree(i, NewMQObject(ctx, o, i));
      }
    }
    JSValue data[]{this_val};
    ret.SetFree("append",
                JS_NewCFunctionData(
                    ctx, method_wrapper_bind<&MQDocumentWrapper::AddObject>, 1,
                    0, 1, data));
    ret.SetFree("remove",
                JS_NewCFunctionData(
                    ctx, method_wrapper_bind<&MQDocumentWrapper::RemoveObject>,
                    1, 0, 1, data));
    return ret.GetValue();
  }

  JSValue AddObject(JSContext* ctx, JSValue obj) {
    MQObjectWrapper* o =
        (MQObjectWrapper*)JS_GetOpaque2(ctx, obj, MQObjectWrapper::class_id);
    if (o == nullptr) {
      return JS_EXCEPTION;
    }
    o->index = doc->AddObject(o->obj);
    return to_jsvalue(ctx, o->index);
  }

  JSValue RemoveObject(JSContext* ctx, JSValue obj) {
    if (JS_IsNumber(obj)) {
      doc->DeleteObject(convert_jsvalue<int>(ctx, obj));
      return JS_UNDEFINED;
    }
    MQObjectWrapper* o =
        (MQObjectWrapper*)JS_GetOpaque2(ctx, obj, MQObjectWrapper::class_id);
    if (o == nullptr) {
      return JS_EXCEPTION;
    }
    doc->DeleteObject(o->index);
    o->Reset();
    return JS_UNDEFINED;
  }

  JSValue GetMaterials(JSContext* ctx, JSValueConst this_val, int argc,
                       JSValueConst* argv) {
    ValueHolder ret(ctx, JS_NewArray(ctx));
    for (int i = 0; i < doc->GetMaterialCount(); i++) {
      auto o = doc->GetMaterial(i);
      if (o != nullptr) {
        ret.SetFree(i, NewMQMaterial(ctx, o, i));
      }
    }
    JSValue data[]{this_val};
    ret.SetFree("append",
                JS_NewCFunctionData(
                    ctx, method_wrapper_bind<&MQDocumentWrapper::AddMaterial>,
                    1, 0, 1, data));
    ret.SetFree(
        "remove",
        JS_NewCFunctionData(
            ctx, method_wrapper_bind<&MQDocumentWrapper::DeleteMaterial>, 1, 0,
            1, data));
    return ret.GetValue();
  }

  JSValue AddMaterial(JSContext* ctx, JSValue obj) {
    MQMaterialWrapper* o = (MQMaterialWrapper*)JS_GetOpaque2(
        ctx, obj, MQMaterialWrapper::class_id);
    if (o == nullptr) {
      return JS_EXCEPTION;
    }
    o->index = doc->AddMaterial(o->mat);
    return to_jsvalue(ctx, o->index);
  }

  JSValue DeleteMaterial(JSContext* ctx, JSValue obj) {
    if (JS_IsNumber(obj)) {
      doc->DeleteMaterial(convert_jsvalue<int>(ctx, obj));
      return JS_UNDEFINED;
    }
    MQMaterialWrapper* o = (MQMaterialWrapper*)JS_GetOpaque2(
        ctx, obj, MQMaterialWrapper::class_id);
    if (o == nullptr) {
      return JS_EXCEPTION;
    }
    doc->DeleteMaterial(o->GetIndex(ctx));
    o->Reset();
    return JS_UNDEFINED;
  }

  void SetPluginData(std::string key, std::string value) {
    if (value == "") {
      pluginKeyValue->erase(key);
    } else {
      (*pluginKeyValue)[key] = value;
    }
  }
  std::string GetPluginData(std::string key) {
    if (pluginKeyValue->count(key) > 0) {
      return (*pluginKeyValue)[key];
    } else {
      return "";
    }
  }

  JSValue Triangulate(JSContext* ctx, JSValue arg) {
    ValueHolder poly(ctx, arg, true);

    ValueHolder result(ctx, JS_NewArray(ctx));
    uint32_t len = poly.Length();
    if (len < 3) {
      return unwrap(std::move(result));
    }

    std::vector<MQPoint> points(len);
    for (uint32_t i = 0; i < len; i++) {
      points[i] = ToMQPoint(ctx, unwrap(poly[i]));
    }
    std::vector<int> indices((size_t)(len - 2) * 3);
    doc->Triangulate(&points[0], (int)points.size(), &indices[0],
                     (int)indices.size());

    for (int i = 0; i < indices.size(); i++) {
      result.Set(i, indices[i]);
    }
    return unwrap(std::move(result));
  }
};

JSClassID MQDocumentWrapper::class_id;

const JSCFunctionListEntry MQDocumentWrapper::proto_funcs[] = {
    function_entry_getset<&GetObjects>("objects"),
    function_entry_getset<&GetMaterials>("materials"),
    function_entry_getset<&GetScene>("scene"),
    function_entry_getset<&GetCurrentObjectIndex, &SetCurrentObjectIndex>(
        "currentObjectIndex"),
    function_entry<&IsVertexSelected>("isVertexSelected"),
    function_entry<&SetVertexSelected>("setVertexSelected"),
    function_entry<&IsFaceSelected>("isFaceSelected"),
    function_entry<&SetFaceSelected>("setFaceSelected"),
    function_entry<&ClearSelect>("clearSelect"),
    function_entry<&Compact>("compact"),
    function_entry<&Triangulate>("triangulate"),
    function_entry<&GetPluginData>("getPluginData"),
    function_entry<&SetPluginData>("setPluginData"),
};

static int DocumentModuleInit(JSContext* ctx, JSModuleDef* m) {
  JSValue meta = JS_GetImportMeta(ctx, m);
  MQDocument doc = (MQDocument)JS_GetOpaque(meta, 0);
  JS_FreeValue(ctx, meta);

  return 0;
}

JSValue NewMQDocument(JSContext* ctx, MQDocumentWrapper* doc) {
  JSValue obj = JS_NewObjectClass(ctx, MQDocumentWrapper::class_id);
  if (JS_IsException(obj)) return obj;
  JS_SetOpaque(obj, doc);
  return obj;
}

void InstallMQDocument(JSContext* ctx, MQDocument doc,
                       std::map<std::string, std::string>* keyValue) {
  JSValue arrayObj = JS_NewArray(ctx);
  NewClassProto<MQSceneWrapper>(ctx, "MQScene");
  NewClassProto<FaceWrapper>(ctx, "Face");
  JSValue vertexArrayProto =
      NewClassProto<VertexArray>(ctx, "VertexArray", &VertexArray_exotic);
  JS_SetPrototype(ctx, vertexArrayProto, arrayObj);
  JSValue faceArrayProto =
      NewClassProto<FaceArray>(ctx, "FaceArray", &FaceArray_exotic);
  JS_SetPrototype(ctx, faceArrayProto, arrayObj);
  JS_FreeValue(ctx, arrayObj);

  JSValue proto = NewClassProto<MQDocumentWrapper>(ctx, "MQDocument");
  ValueHolder global(ctx, JS_GetGlobalObject(ctx));
  global.SetFree("document",
                 NewMQDocument(ctx, new MQDocumentWrapper(doc, keyValue)));
  global.SetFree("MQObject",
                 newClassConstructor<MQObjectWrapper>(ctx, "MQObject"));
  global.SetFree("MQMaterial",
                 newClassConstructor<MQMaterialWrapper>(ctx, "MQMaterial"));
}
