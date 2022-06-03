
#include <windows.h>

#include <map>
#include <sstream>
#include <vector>

#include "MQBasePlugin.h"
#include "MQWidget.h"
#include "Utils.h"
#include "qjsutils.h"

//---------------------------------------------------------------------------------------------------------------------
// Vertics
//---------------------------------------------------------------------------------------------------------------------

JSValue NewVec3(JSContext* ctx, const MQPoint& p) {
  JSValue g = JS_GetGlobalObject(ctx);
  JSValue vec = JS_GetPropertyStr(ctx, g, "_vector3class");
  JS_FreeValue(ctx, g);
  if (vec != JS_UNDEFINED) {
    JSValue args[] = { JS_NewFloat64(ctx, p.x), JS_NewFloat64(ctx, p.y),
                      JS_NewFloat64(ctx, p.z) };
    JSValue v = JS_CallConstructor(ctx, vec, 3, args);
    JS_FreeValue(ctx, args[0]);
    JS_FreeValue(ctx, args[1]);
    JS_FreeValue(ctx, args[2]);
    JS_FreeValue(ctx, vec);
    return v;
  }
  ValueHolder v(ctx);
  v.Set("x", p.x);
  v.Set("y", p.y);
  v.Set("z", p.z);
  return unwrap(std::move(v));
}

JSValue ToJSValue(JSContext* ctx, const MQAngle& a) {
  ValueHolder v(ctx);
  v.Set("head", a.head);
  v.Set("pitch", a.pitch);
  v.Set("bank", a.bank);
  return unwrap(std::move(v));
}

JSValue ToJSValue(JSContext* ctx, const MQMatrix& mat) {
  ValueHolder ret(ctx, JS_NewArray(ctx));
  for (int i = 0; i < 16; i++) {
    ret.Set(i, mat.t[i]);
  }
  return unwrap(std::move(ret));
}

MQPoint ToMQPoint(JSContext* ctx, JSValueConst value) {
  ValueHolder v(ctx, value, true);
  if (v.IsArray() && v.Length() == 3) {
    return MQPoint(v[0U].To<float>(), v[1U].To<float>(), v[2U].To<float>());
  }
  return MQPoint(v["x"].To<float>(), v["y"].To<float>(), v["z"].To<float>());
}

MQAngle ToMQAngle(JSContext* ctx, JSValueConst value) {
  ValueHolder angle(ctx, value, true);
  return MQAngle(angle["head"].To<float>(), angle["pitch"].To<float>(),
    angle["bank"].To<float>());
}

class VertexArray : public JSClassBase<VertexArray> {
 public:
  MQObject obj;
  VertexArray(MQObject obj) : obj(obj) {}

  //  static JSClassID class_id;
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
    ValueHolder v(ctx, NewVec3(ctx, p));
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
    .delete_property = delete_property_handler<&VertexArray::DeleteVertex> };

// JSClassID VertexArray::class_id;

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
    std::vector<int> indices(count);
    obj->GetFacePointArray(index, indices.data());
    for (int i = 0; i < count; i++) {
      points.Set(i, indices[i]);
    }
    return points.GetValue();
  }
  JSValue GetUV(JSContext* ctx) {
    ValueHolder points(ctx, JS_NewArray(ctx));
    int count = obj->GetFacePointCount(index);
    std::vector<MQCoordinate> uva(count);
    obj->GetFaceCoordinateArray(index, uva.data());
    for (int i = 0; i < count; i++) {
      ValueHolder u(ctx);
      u.Set("u", uva[i].u);
      u.Set("v", uva[i].v);
      points.Set(i, u);
    }
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
    if (count == 0) {
      mat = v["material"].To<int>();
      v = v["points"];
    }
    std::vector<int> indices(count);
    for (uint32_t i = 0; i < count; i++) {
      indices[i] = v[i].To<int32_t>();
    }

    int f = obj->AddFace(count, indices.data());
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
      std::vector<int> indices(count);
      for (uint32_t i = 0; i < count; i++) {
        indices[i] = points[i].To<int>();
      }
      obj->InsertFace(index, count, indices.data());
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
    .delete_property = delete_property_handler<&FaceArray::DeleteFace> };

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
// MQObjectTransform
//---------------------------------------------------------------------------------------------------------------------
JSValue NewObjectTransform(JSContext* ctx, MQObject o);

class ObjectTransform : public JSClassBase<ObjectTransform> {
 public:
  static const JSCFunctionListEntry proto_funcs[];

  MQObject obj;
  ObjectTransform(MQObject obj) : obj(obj) {}

  JSValue GetScale(JSContext* ctx) { return NewVec3(ctx, obj->GetScaling()); }
  void SetScale(JSContext* ctx, JSValue v) {
    obj->SetScaling(ToMQPoint(ctx, v));
  }
  JSValue GetPosition(JSContext* ctx) {
    return NewVec3(ctx, obj->GetTranslation());
  }
  void SetPosition(JSContext* ctx, JSValue v) {
    obj->SetTranslation(ToMQPoint(ctx, v));
  }
  JSValue GetRotation(JSContext* ctx) {
    return ToJSValue(ctx, obj->GetRotation());
  }
  void SetRotation(JSContext* ctx, JSValue v) {
    obj->SetRotation(ToMQAngle(ctx, v));
  }
  JSValue GetMatrix(JSContext* ctx) {
    return ToJSValue(ctx, obj->GetLocalMatrix());
  }
  JSValue SetMatrix(JSContext* ctx, JSValue v) {
    ValueHolder matarray(ctx, v, true);
    if (!matarray.IsArray()) {
      return JS_EXCEPTION;
    }
    MQMatrix mat;
    for (int i = 0; i < 16; i++) {
      mat.t[i] = matarray[i].To<float>();
    }
    obj->SetLocalMatrix(mat);
    return JS_UNDEFINED;
  }
  JSValue GetInverseMatrix(JSContext* ctx) {
    return ToJSValue(ctx, obj->GetLocalInverseMatrix());
  }
};

const JSCFunctionListEntry ObjectTransform::proto_funcs[] = {
    function_entry_getset<&GetScale, &SetScale>("scale"),
    function_entry_getset<&GetPosition, &SetPosition>("position"),
    function_entry_getset<&GetRotation, &SetRotation>("rotation"),
    function_entry_getset<&GetMatrix, &SetMatrix>("matrix"),
    function_entry_getset<&GetInverseMatrix>("matrixInverse"),
};

JSValue NewObjectTransform(JSContext* ctx, MQObject o) {
  JSValue obj = JS_NewObjectClass(ctx, ObjectTransform::class_id);
  if (JS_IsException(obj)) return obj;
  JS_SetOpaque(obj, new ObjectTransform(o));
  return obj;
}

//---------------------------------------------------------------------------------------------------------------------
// MQObject
//---------------------------------------------------------------------------------------------------------------------
JSValue NewMQObject(JSContext* ctx, MQObject o, MQDocument doc = nullptr,
                    int index = -1, bool drawingObject = false);

class MQObjectWrapper : public JSClassBase<MQObjectWrapper> {
 public:
  MQObject obj;
  MQDocument doc = nullptr;
  int index = -1;
  bool isDrawingObject = false;
  MQObjectWrapper(MQObject obj, MQDocument _doc, int index,
                  bool drawingObject = false)
      : obj(obj), doc(_doc), index(index), isDrawingObject(drawingObject) {}
  MQObjectWrapper() : obj(MQ_CreateObject()) {}
  ~MQObjectWrapper() {
    if (isDrawingObject) {
      auto plugin = dynamic_cast<MQStationPlugin*>(GetPluginClass());
      if (doc && plugin) {
        plugin->DeleteDrawingObject(doc, obj);
      } else {
        debug_log("cannot delete drawingObject", 2);
      }
    } else if (index < 0) {
      obj->DeleteThis();
    }
  }

  void Init(JSContext* ctx, JSValueConst this_obj, int argc,
    JSValueConst* argv) {
    JS_SetPropertyStr(ctx, this_obj, "verts", NewVertexArray(ctx, obj));
    JS_SetPropertyStr(ctx, this_obj, "faces", NewFaceArray(ctx, obj));
    JS_SetPropertyStr(ctx, this_obj, "transform", NewObjectTransform(ctx, obj));
    if (argc > 0 && JS_IsString(argv[0])) {
      SetName(convert_jsvalue<std::string>(ctx, argv[0]));
    }
  }

  void Reset() {
    obj = MQ_CreateObject();
    index = -1;
    doc = nullptr;
    isDrawingObject = false;
  }

  static const JSCFunctionListEntry proto_funcs[];

  int GetIndex() { return index; }
  int GetId() { return obj->GetUniqueID(); }
  int GetType() { return obj->GetType(); }
  void SetType(int t) { obj->SetType(t); }
  bool Selected() { return obj->GetSelected(); }
  void SetSelected(bool s) { obj->SetSelected(s); }
  bool Visible() { return obj->GetVisible(); }
  void SetVisible(bool v) { obj->SetVisible(v ? 0xFFFFFFFF : 0); }
  bool Locked() { return obj->GetLocking(); }
  void SetLocked(bool v) { obj->SetLocking(v); }
  int GetDepth() { return obj->GetDepth(); }
  void SetDepth(int v) { obj->SetDepth(v); }
  std::string GetName() {
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
    return converter.to_bytes(obj->GetNameW());
  }
  JSValue SetName(const std::string& name) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
    obj->SetName(converter.from_bytes(name).c_str());
    return JS_UNDEFINED;
  }
  void AddRenderFlag(int flag) {
    obj->AddRenderFlag((MQOBJECT_RENDER_FLAG)flag);
  }
  void RemoveRenderFlag(int flag) {
    obj->RemoveRenderFlag((MQOBJECT_RENDER_FLAG)flag);
  }
  JSValue GetWireframe() { return JS_UNDEFINED; }
  void SetWireframe(bool on) {
    if (on) {
      obj->AddRenderFlag(MQOBJECT_RENDER_LINE);
      obj->AddRenderEraseFlag(MQOBJECT_RENDER_POINT);
      obj->AddRenderEraseFlag(MQOBJECT_RENDER_FACE);
    } else {
      obj->RemoveRenderFlag(MQOBJECT_RENDER_LINE);
      obj->RemoveRenderEraseFlag(MQOBJECT_RENDER_POINT);
      obj->RemoveRenderEraseFlag(MQOBJECT_RENDER_FACE);
    }
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
  JSValue Clone(JSContext* ctx) { return NewMQObject(ctx, obj->Clone()); }
  void OptimizeVertex(float distance) {
    obj->OptimizeVertex(distance, nullptr);
  }
};

const JSCFunctionListEntry MQObjectWrapper::proto_funcs[] = {
    function_entry_getset<&GetIndex>("index"),
    function_entry_getset<&GetId>("id"),
    function_entry_getset<&GetName, &SetName>("name"),
    function_entry_getset<&GetType, &SetType>("type"),
    function_entry_getset<&GetDepth, &SetDepth>("depth"),
    function_entry_getset<&Visible, &SetVisible>("visible"),
    function_entry_getset<&Selected, &SetSelected>("selected"),
    function_entry_getset<&Locked, &SetLocked>("locked"),
    function_entry<&AddRenderFlag>("addRenderFlag"),
    function_entry<&RemoveRenderFlag>("removeRenderFlag"),
    function_entry<&Compact>("compact"),
    function_entry<&Clear>("clear"),
    function_entry<&Freeze>("freeze"),
    function_entry<&Merge>("merge"),
    function_entry<&Clone>("clone"),
    function_entry<&OptimizeVertex>("optimizeVertex"),
    function_entry_getset<&GetWireframe, &SetWireframe>("wireframe"),
};

JSValue NewMQObject(JSContext* ctx, MQObject o, MQDocument doc, int index,
  bool drawingObject) {
  JSValue obj = JS_NewObjectClass(ctx, MQObjectWrapper::class_id);
  if (JS_IsException(obj)) return obj;
  MQObjectWrapper* p = new MQObjectWrapper(o, doc, index, drawingObject);
  JS_SetOpaque(obj, p);
  p->Init(ctx, obj, 0, nullptr);
  return obj;
}

//---------------------------------------------------------------------------------------------------------------------
// MQMaterial
//---------------------------------------------------------------------------------------------------------------------

class MQMaterialWrapper : public JSClassBase<MQMaterialWrapper> {
 public:
  MQMaterial mat;
  MQDocument doc = nullptr;
  int index = -1;
  bool isDrawingMaterial = false;
  MQMaterialWrapper(MQMaterial mat, MQDocument _doc, int index,
                    bool drawingMaterial)
      : index(index), mat(mat), doc(_doc), isDrawingMaterial(drawingMaterial) {}
  MQMaterialWrapper() : mat(MQ_CreateMaterial()) {}
  ~MQMaterialWrapper() {
    if (isDrawingMaterial) {
      auto plugin = dynamic_cast<MQStationPlugin*>(GetPluginClass());
      if (mat && plugin) {
        plugin->DeleteDrawingMaterial(doc, mat);
      } else {
        debug_log("cannot delete drawingMaterial", 2);
      }
    } else if (index < 0) {
      mat->DeleteThis();
    }
  }

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
    doc = nullptr;
    isDrawingMaterial = false;
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

  std::string GetTextureName(JSContext* ctx) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
    wchar_t path[MAX_PATH] = { L'\0' };
    mat->GetTextureNameW(path, MAX_PATH);
    return converter.to_bytes(path);
  }
  JSValue SetTextureName(std::string name) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
    mat->SetTextureName(converter.from_bytes(name).c_str());
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

  int GetShader() { return mat->GetShader(); }
  void SetShader(int v) { mat->SetShader(v); }

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

const JSCFunctionListEntry MQMaterialWrapper::proto_funcs[] = {
    function_entry_getset<&GetIndex>("index"),
    function_entry_getset<&GetId>("id"),
    function_entry_getset<&GetName, &SetName>("name"),
    function_entry_getset<&GetTextureName, &SetTextureName>("texture"),
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
    function_entry_getset<&GetShader, &SetShader>("shaderType"),
};

JSValue NewMQMaterial(JSContext* ctx, MQMaterial mat, MQDocument doc = nullptr,
  int index = -1, bool drawingMaterial = false) {
  JSValue obj = JS_NewObjectClass(ctx, MQMaterialWrapper::class_id);
  if (JS_IsException(obj)) return obj;
  MQMaterialWrapper* p =
    new MQMaterialWrapper(mat, doc, index, drawingMaterial);
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
    return ToJSValue(ctx, scene->GetCameraAngle());
  }
  void SetCameraAngle(JSContext* ctx, JSValue v) {
    scene->SetCameraAngle(ToMQAngle(ctx, v));
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
  JSValueConst self = JS_UNDEFINED;
  static JSClassID class_id;
  static const JSCFunctionListEntry proto_funcs[];

  MQDocumentWrapper(MQDocument doc,
    std::map<std::string, std::string>* keyValue)
    : doc(doc) {
    pluginKeyValue = keyValue;
  }
  MQDocumentWrapper() : doc(nullptr), pluginKeyValue(nullptr) {}

  void Compact() { doc->Compact(); }
  int GetCurrentObjectIndex() { return doc->GetCurrentObjectIndex(); }
  void SetCurrentObjectIndex(int i) { doc->SetCurrentObjectIndex(i); }
  int GetCurrentMaterialIndex() { return doc->GetCurrentMaterialIndex(); }
  void SetCurrentMaterialIndex(int i) { doc->SetCurrentMaterialIndex(i); }
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

  ValueHolder GetObjcetCache(JSContext* ctx, JSValueConst this_val) {
    ValueHolder t(ctx, this_val, true);
    if (!t.Has("_objectCache")) {
      t.Define("_objectCache", JS_NewObject(ctx));
    }
    return t["_objectCache"];
  }

  JSValue GetObjects(JSContext* ctx, JSValueConst this_val, int argc,
    JSValueConst* argv) {
    ValueHolder objectCache = GetObjcetCache(ctx, this_val);
    ValueHolder ret(ctx, JS_NewArray(ctx));
    for (int i = 0; i < doc->GetObjectCount(); i++) {
      auto o = doc->GetObject(i);
      if (o != nullptr) {
        JSValue obj = objectCache[o->GetUniqueID()].GetValue();
        auto ow = MQObjectWrapper::Unwrap(obj);
        if (ow && ow->obj == o) {
          ow->index = i;  // update object index
        } else {
          JS_FreeValue(ctx, obj);
          obj = NewMQObject(ctx, o, doc, i);
          objectCache.Set(o->GetUniqueID(), JS_DupValue(ctx, obj));
        }
        ret.Set(i, obj);
      }
    }
    JSValue data[]{ this_val };
    ret.Define("append",
      JS_NewCFunctionData(
        ctx, method_wrapper_bind<&MQDocumentWrapper::AddObject>, 1,
        0, 1, data));
    ret.Define("remove",
      JS_NewCFunctionData(
        ctx, method_wrapper_bind<&MQDocumentWrapper::RemoveObject>,
        1, 0, 1, data));
    ret.Define("insert",
      JS_NewCFunctionData(
        ctx, method_wrapper_bind<&MQDocumentWrapper::InsertObject>, 2,
        0, 1, data));
    return ret.GetValue();
  }

  JSValue AddObject(JSContext* ctx, JSValueConst this_val, int argc,
    JSValueConst* argv, MQObjectWrapper* o) {
    if (o == nullptr || o->isDrawingObject) {
      return JS_EXCEPTION;
    }
    o->index = doc->AddObject(o->obj);
    o->doc = doc;
    ValueHolder objectCache = GetObjcetCache(ctx, this_val);
    objectCache.Set(o->GetId(), JS_DupValue(ctx, argv[0]));
    return to_jsvalue(ctx, o->index);
  }

  JSValue RemoveObject(JSContext* ctx, JSValueConst this_val, int argc,
    JSValueConst* argv, JSValue obj) {
    if (JS_IsNumber(obj)) {
      doc->DeleteObject(convert_jsvalue<int>(ctx, obj));
      return JS_UNDEFINED;
    }
    MQObjectWrapper* o = MQObjectWrapper::Unwrap(ctx, obj);
    if (o == nullptr) {
      return JS_EXCEPTION;
    }
    ValueHolder objectCache = GetObjcetCache(ctx, this_val);
    objectCache.Delete(o->GetId());
    doc->DeleteObject(o->index);
    o->Reset();
    return JS_UNDEFINED;
  }

  JSValue InsertObject(JSContext* ctx, JSValueConst this_val, int argc,
    JSValueConst* argv, MQObjectWrapper* o, MQObjectWrapper* before) {
    if (o == nullptr || o->isDrawingObject  || before->doc == nullptr) {
      return JS_EXCEPTION;
    }
    o->index = doc->InsertObject(o->obj, before->obj);
    o->doc = doc;
    ValueHolder objectCache = GetObjcetCache(ctx, this_val);
    objectCache.Set(o->GetId(), JS_DupValue(ctx, argv[0]));
    return to_jsvalue(ctx, o->index);
  }

  JSValue GetMaterials(JSContext* ctx, JSValueConst this_val, int argc,
    JSValueConst* argv) {
    ValueHolder ret(ctx, JS_NewArray(ctx));
    for (int i = 0; i < doc->GetMaterialCount(); i++) {
      auto o = doc->GetMaterial(i);
      if (o != nullptr) {
        ret.SetFree(i, NewMQMaterial(ctx, o, doc, i));
      }
    }
    JSValue data[]{ this_val };
    ret.Define("append",
      JS_NewCFunctionData(
        ctx, method_wrapper_bind<&MQDocumentWrapper::AddMaterial>, 1,
        0, 1, data));
    ret.Define("remove",
      JS_NewCFunctionData(
        ctx, method_wrapper_bind<&MQDocumentWrapper::DeleteMaterial>,
        1, 0, 1, data));
    return ret.GetValue();
  }

  JSValue AddMaterial(JSContext* ctx, MQMaterialWrapper* o) {
    if (o == nullptr || o->isDrawingMaterial) {
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
    MQMaterialWrapper* o = MQMaterialWrapper::Unwrap(ctx, obj);
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

  JSValue GetGlobalMatrix(JSContext* ctx, MQObjectWrapper* o) {
    if (!o) {
      return JS_EXCEPTION;
    }
    MQMatrix mat;
    doc->GetGlobalMatrix(o->obj, mat);
    return ToJSValue(ctx, mat);
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

  JSValue CreateDrawingObject(JSContext* ctx, JSValue arg) {
    auto plugin = dynamic_cast<MQStationPlugin*>(GetPluginClass());
    if (!plugin) {
      return JS_EXCEPTION;
    }
    MQObject obj = plugin->CreateDrawingObject(
      doc, MQStationPlugin::DRAW_OBJECT_LINE, FALSE);
    return NewMQObject(ctx, obj, doc, -1, true);
  }

  JSValue CreateDrawingMaterial(JSContext* ctx, JSValue arg) {
    auto plugin = dynamic_cast<MQStationPlugin*>(GetPluginClass());
    if (!plugin) {
      return JS_EXCEPTION;
    }
    int index = -1;
    MQMaterial mat = plugin->CreateDrawingMaterial(doc, index, FALSE);
    return NewMQMaterial(ctx, mat, doc, index, true);
  }
};

JSClassID MQDocumentWrapper::class_id;

static JSValue SetDrawProxyObject(JSContext* ctx, JSValue obj, JSValue proxy,
  bool sync_select) {
  MQBasePlugin* plugin = GetPluginClass();
  if (!plugin) {
    return JS_EXCEPTION;
  }
  MQObjectWrapper* mqobj = MQObjectWrapper::Unwrap(obj);
  MQObjectWrapper* mqobjproxy = MQObjectWrapper::Unwrap(proxy);
  if (!mqobj) {
    return JS_EXCEPTION;
  }

  plugin->SetDrawProxyObject(mqobj->obj, mqobjproxy ? mqobjproxy->obj : nullptr,
    sync_select);
  JS_SetPropertyStr(ctx, obj, "_drawProxyObject", JS_DupValue(ctx, proxy));
  return JS_UNDEFINED;
}

const JSCFunctionListEntry MQDocumentWrapper::proto_funcs[] = {
    function_entry_getset<&GetObjects>("objects"),
    function_entry_getset<&GetMaterials>("materials"),
    function_entry_getset<&GetScene>("scene"),
    function_entry_getset<&GetCurrentObjectIndex, &SetCurrentObjectIndex>(
        "currentObjectIndex"),
    function_entry_getset<&GetCurrentMaterialIndex, &SetCurrentMaterialIndex>(
        "currentMaterialIndex"),
    function_entry<&IsVertexSelected>("isVertexSelected"),
    function_entry<&SetVertexSelected>("setVertexSelected"),
    function_entry<&IsFaceSelected>("isFaceSelected"),
    function_entry<&SetFaceSelected>("setFaceSelected"),
    function_entry<&ClearSelect>("clearSelect"),
    function_entry<&Compact>("compact"),
    function_entry<&Triangulate>("triangulate"),
    function_entry<&GetGlobalMatrix>("getGlobalMatrix"),
    function_entry<&GetPluginData>("getPluginData"),
    function_entry<&SetPluginData>("setPluginData"),
    function_entry<&CreateDrawingObject>("createDrawingObject"),
    function_entry<&CreateDrawingMaterial>("createDrawingMaterial"),
    function_entry("setDrawProxyObject", 3, method_wrapper<SetDrawProxyObject>),
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
  doc->self = obj;
  JS_SetOpaque(obj, doc);
  return obj;
}

void InstallMQDocument(JSContext* ctx, MQDocument doc,
  std::map<std::string, std::string>* keyValue = nullptr) {
  JSValue arrayObj = JS_NewArray(ctx);
  NewClassProto<MQSceneWrapper>(ctx, "MQScene");
  NewClassProto<FaceWrapper>(ctx, "Face");
  NewClassProto<ObjectTransform>(ctx, "ObjectTransform");
  JSValue vertexArrayProto =
    VertexArray::NewClassProto(ctx, "VertexArray", &VertexArray_exotic);
  JS_SetPrototype(ctx, vertexArrayProto, arrayObj);
  JSValue faceArrayProto =
    NewClassProto<FaceArray>(ctx, "FaceArray", &FaceArray_exotic);
  JS_SetPrototype(ctx, faceArrayProto, arrayObj);
  JS_FreeValue(ctx, arrayObj);

  ValueHolder global(ctx, JS_GetGlobalObject(ctx));
  global.Set("MQObject", newClassConstructor<MQObjectWrapper>(ctx, "MQObject"));
  global.Set("MQMaterial",
    newClassConstructor<MQMaterialWrapper>(ctx, "MQMaterial"));
  global.Set("MQDocument",
    newClassConstructor<MQDocumentWrapper>(ctx, "MQDocument"));
  global.Set("mqdocument",
    NewMQDocument(ctx, new MQDocumentWrapper(doc, keyValue)));
}
