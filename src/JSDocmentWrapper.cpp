
#define WIN32_LEAN_AND_MEAN
#include <vector>
#include <sstream>
#include <windows.h>

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
  return v.GetValue();
}

class VertexArray {
public:
  MQObject obj;
  VertexArray(MQObject obj) : obj(obj) {}

  static JSClassID class_id;
  static const JSCFunctionListEntry proto_funcs[];

  int Length() {
    return obj->GetVertexCount();
  }

  JSValue Append(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    MQPoint p;
    if (argc >= 1 && JS_IsObject(argv[0])) {
      ValueHolder v(ctx, argv[0], true);
      p.x = v["x"].to<float>();
      p.y = v["y"].to<float>();
      p.z = v["z"].to<float>();
    }
    else if (argc >= 3) {
      p.x = convert_jsvalue<float>(ctx, argv[0]);
      p.y = convert_jsvalue<float>(ctx, argv[1]);
      p.z = convert_jsvalue<float>(ctx, argv[2]);
    }

    //char s[256];
    //sprintf(s, "%f %f %f", p.x, p.y, p.z);
    //debug_log(s);

    return to_jsvalue(ctx, obj->AddVertex(p));
  }


  // ser from array_accessor2...
  uint32_t access_index = 0;
  void set_array_index(uint32_t i) {
    access_index = i;
  }

  JSValue GetVertex(JSContext* ctx) {
    uint32_t index = access_index;
    MQPoint p = obj->GetVertex(index);
    ValueHolder v(ctx);
    v.Set("x", p.x);
    v.Set("y", p.y);
    v.Set("z", p.z);
    v.Set("id", obj->GetVertexUniqueID(index));
    v.Set("refs", obj->GetVertexRefCount(index));
    return v.GetValue();
  }

  void SetVertex(JSContext* ctx, JSValue value) {
    uint32_t index = access_index;
    MQPoint p;
    ValueHolder v(ctx, value, true);
    p.x = v["x"].to<float>();
    p.y = v["y"].to<float>();
    p.z = v["z"].to<float>();
    obj->SetVertex(index, p);
  }
  bool DeleteVertex(JSContext* ctx, JSValue value) {
    return obj->DeleteVertex(convert_jsvalue<int>(ctx, value));
  }
};

template<auto method>
int delete_property_handler(JSContext* ctx,  JSValueConst obj, JSAtom prop) {
  JSValue v = JS_AtomToValue(ctx, prop);
  JSValue ret = invoke_method(method, ctx, obj, 1, &v);
  int reti = convert_jsvalue<int>(ctx, ret);
  JS_FreeValue(ctx, v);
  JS_FreeValue(ctx, ret);
  return reti;
}

static JSClassExoticMethods VertexArray_exotic{
  .get_own_property = array_accessor2<&VertexArray::GetVertex, &VertexArray::SetVertex>,
  .delete_property = delete_property_handler<&VertexArray::DeleteVertex>
};

JSClassID VertexArray::class_id;

const JSCFunctionListEntry VertexArray::proto_funcs[] = {
    function_entry_getset("length", method_wrapper_getter<&Length>),
    function_entry("append", 1, method_wrapper<&Append>),
};

JSValue NewVertexArray(JSContext* ctx, MQObject o) {
  JSValue obj = JS_NewObjectClass(ctx, VertexArray::class_id);
  if (JS_IsException(obj))
    return obj;
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

  int GetIndex() {
    return index;
  }
  int GetId() {
    return obj->GetFaceUniqueID(index);
  }
  int GetMaterial() {
    return obj->GetFaceMaterial(index);
  }
  void SetMaterial(int m) {
    obj->SetFaceMaterial(index, m);
  }
  bool GetVisible() {
    return obj->GetFaceVisible(index);
  }
  void SetVisible(bool v) {
    obj->SetFaceVisible(index, v);
  }
  JSValue GetPoints(JSContext* ctx) {
    ValueHolder points(ctx, JS_NewArray(ctx));
    int count = obj->GetFacePointCount(index);
    int *vv = new int[count];
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
  void Invert() {
    obj->InvertFace(index);
  }
};

JSClassID FaceWrapper::class_id;

const JSCFunctionListEntry FaceWrapper::proto_funcs[] = {
    function_entry_getset("index", method_wrapper_getter<&GetIndex>),
    function_entry_getset("id", method_wrapper_getter<&GetId>),
    function_entry_getset("material", method_wrapper_getter<&GetMaterial>, method_wrapper_setter<&SetMaterial>),
    function_entry_getset("visible", method_wrapper_getter<&GetVisible>, method_wrapper_setter<&SetVisible>),
    function_entry_getset("points", method_wrapper_getter<&GetPoints>),
    function_entry_getset("uv", method_wrapper_getter<&GetUV>),
    function_entry("invert", 0, method_wrapper<&Invert>),
};

JSValue NewFaceWrapper(JSContext* ctx, MQObject o, int index) {
  JSValue obj = JS_NewObjectClass(ctx, FaceWrapper::class_id);
  if (JS_IsException(obj))
    return obj;
  JS_SetOpaque(obj, new FaceWrapper(o, index));
  return obj;
}


class FaceArray {
public:
  MQObject obj;
  FaceArray(MQObject obj) : obj(obj) {}

  static JSClassID class_id;
  static const JSCFunctionListEntry proto_funcs[];

  int Length() {
    return obj->GetFaceCount();
  }

  int Append(JSContext* ctx, JSValue points, int mat) {
    ValueHolder v(ctx, points, true);
    uint32_t count = v.Length();
    int* indices = new int[count];
    for (uint32_t i = 0; i < count; i++) {
      indices[i] = v[i].to<int32_t>();
    }

    int f = obj->AddFace(count, indices);
    delete[] indices;
    obj->SetFaceMaterial(f, mat);
    return f;
  }

  // ser from array_accessor2...
  uint32_t access_index = 0;
  void set_array_index(uint32_t i) {
    access_index = i;
  }
  JSValue GetFace(JSContext* ctx) {
    if (obj->GetFacePointCount(access_index) == 0) {
      return JS_UNDEFINED;
    }
    return NewFaceWrapper(ctx, obj, access_index);
  }

  void SetFace(JSContext* ctx, JSValue value) {
    int index = access_index;
    ValueHolder face(ctx, value, true);
    auto points = face["points"];
    if (points.IsArray()) {
      obj->DeleteFace(index);
      uint32_t count = points.Length();
      int* pp = new int[count];
      for (uint32_t i = 0; i < count; i++) {
        pp[i] = points[i].to<int>();
      }
      obj->InsertFace(index, count, pp);
      delete[] pp;
    }
    auto mat = face["material"];
    if (!mat.IsUndefined()) {
      obj->SetFaceMaterial(index, mat.to<int>());
    }
  }
  bool DeleteFace(JSContext* ctx, JSValue value) {
    return obj->DeleteFace(convert_jsvalue<int>(ctx, value));
  }
};

static JSClassExoticMethods FaceArray_exotic{
  .get_own_property = array_accessor2<&FaceArray::GetFace, &FaceArray::SetFace>,
  .delete_property = delete_property_handler<&FaceArray::DeleteFace>
};

JSClassID FaceArray::class_id;

const JSCFunctionListEntry FaceArray::proto_funcs[] = {
    function_entry_getset("length", method_wrapper_getter<&Length>),
    function_entry("append", 2, method_wrapper<&Append>),
    // TODO: [Symbol.iterator] JS_ITERATOR_KIND_VALUE
};

JSValue NewFaceArray(JSContext* ctx, MQObject o) {
  JSValue obj = JS_NewObjectClass(ctx, FaceArray::class_id);
  if (JS_IsException(obj))
    return obj;
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
  MQObjectWrapper(JSContext* ctx) : index(-1), obj(MQ_CreateObject()) { }

  void Init(JSContext* ctx, JSValueConst this_obj, int argc, JSValueConst* argv) {
    JS_SetPropertyStr(ctx, this_obj, "verts", NewVertexArray(ctx, obj));
    JS_SetPropertyStr(ctx, this_obj, "faces", NewFaceArray(ctx, obj));
    if (argc > 0 && JS_IsString(argv[0])) {
      obj->SetName(utf8ToSjis(convert_jsvalue<std::string>(ctx, argv[0])).c_str());
    }
  }

  void Reset() {
    obj = MQ_CreateObject();
    index = -1;
  }

  static JSClassID class_id;
  static const JSCFunctionListEntry proto_funcs[];

  int GetIndex(JSContext* ctx) {
    return index;
  }
  int GetId() {
    return obj->GetUniqueID();
  }
  int GetType() {
    return obj->GetType();
  }
  void SetType(int t) {
    obj->SetType(t);
  }
  bool Selected() {
    return obj->GetSelected();
  }
  void SetSelected(bool s) {
    obj->SetSelected(s);
  }
  bool Visible() {
    return obj->GetVisible();
  }
  void SetVisible(bool v) {
    obj->SetVisible(v);
  }
  std::string GetName(JSContext* ctx) {
    //char buf[256];
    //obj->GetName(buf, sizeof(buf));
    //return sjisToUtf8(buf);
    return sjisToUtf8(obj->GetName());
  }
  JSValue SetName(JSContext* ctx, std::string name) {
    obj->SetName(name.c_str());
    return JS_UNDEFINED;
  }
  void Compact() {
    obj->Compact();
  }
  void Freeze(int flags) {
    obj->Freeze(flags);
  }
  void Merge(JSValue src) {
    MQObjectWrapper* o = (MQObjectWrapper*)JS_GetOpaque(src, MQObjectWrapper::class_id);
    if (o == nullptr) {
      return;
    }
    obj->Merge(o->obj);
  }
  JSValue Clone(JSContext* ctx, bool reg) {
    return NewMQObject(ctx, obj->Clone(), -1);
  }
};

JSClassID MQObjectWrapper::class_id;

const JSCFunctionListEntry MQObjectWrapper::proto_funcs[] = {
    function_entry_getset("index", method_wrapper_getter<&GetIndex>),
    function_entry_getset("id", method_wrapper_getter<&GetId>),
    function_entry_getset("name", method_wrapper_getter<&GetName>, method_wrapper_setter<&SetName>),
    function_entry_getset("type", method_wrapper_getter<&GetType>, method_wrapper_setter<&SetType>),
    function_entry_getset("visible",  method_wrapper_getter<&Visible>, method_wrapper_setter<&SetVisible>),
    function_entry_getset("selected", method_wrapper_getter<&Selected>, method_wrapper_setter<&SetSelected>),
    function_entry("compact", 0, method_wrapper<&Compact>),
    function_entry("freeze", 1, method_wrapper<&Freeze>),
    function_entry("merge", 1, method_wrapper<&Merge>),
    function_entry("clone", 1, method_wrapper<&Clone>),
};

JSValue NewMQObject(JSContext* ctx, MQObject o, int index) {
  JSValue obj = JS_NewObjectClass(ctx, MQObjectWrapper::class_id);
  if (JS_IsException(obj))
    return obj;
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
  MQMaterialWrapper(JSContext* ctx) : index(-1), mat(MQ_CreateMaterial()) {}

  static JSClassID class_id;
  static const JSCFunctionListEntry proto_funcs[];

  void Init(JSContext* ctx, JSValueConst this_obj, int argc, JSValueConst* argv) {
    if (argc > 0 && JS_IsString(argv[0])) {
      mat->SetName(utf8ToSjis(convert_jsvalue<std::string>(ctx, argv[0])).c_str());
    }
  }

  void Reset() {
    mat = MQ_CreateMaterial();
    index = -1;
  }

  int GetIndex(JSContext* ctx) {
    return index;
  }
  int GetId(JSContext* ctx) {
    return mat->GetUniqueID();
  }
  bool GetSelected() {
    return mat->GetSelected();
  }
  void SetSelected(bool s) {
    mat->SetSelected(s);
  }
  std::string GetName(JSContext* ctx) {
    char buf[256];
    mat->GetName(buf, sizeof(buf));
    return sjisToUtf8(buf);
  }
  JSValue SetName(JSContext* ctx, std::string name) {
    mat->SetName(name.c_str());
    return JS_UNDEFINED;
  }
  JSValue GetColor(JSContext* ctx) {
    MQColor c = mat->GetColor();
    ValueHolder v(ctx);
    v.Set("r", c.r);
    v.Set("g", c.g);
    v.Set("b", c.b);
    v.Set("a", mat->GetAlpha());
    return v.GetValue();
  }
  void SetColor(JSContext* ctx, JSValue v) {
    ValueHolder col(ctx, v, true);
    MQColor c;
    c.r = col["r"].to<float>();
    c.g = col["g"].to<float>();
    c.b = col["b"].to<float>();
    mat->SetColor(c);
    ValueHolder a = col["a"];
    if (!a.IsUndefined()) {
      mat->SetAlpha(a.to<float>());
    }
  }
};

JSClassID MQMaterialWrapper::class_id;

const JSCFunctionListEntry MQMaterialWrapper::proto_funcs[] = {
    function_entry_getset("index", method_wrapper_getter<&GetIndex>),
    function_entry_getset("id",  method_wrapper_getter<&GetId>),
    function_entry_getset("name", method_wrapper_getter<&GetName>, method_wrapper_setter<&SetName>),
    function_entry_getset("color",  method_wrapper_getter<&GetColor>, method_wrapper_setter<&SetColor>),
    function_entry_getset("selected",  method_wrapper_getter < &GetSelected > , method_wrapper_setter<&SetSelected>),
};

JSValue NewMQMaterial(JSContext* ctx, MQMaterial mat, int index) {
  JSValue obj = JS_NewObjectClass(ctx, MQMaterialWrapper::class_id);
  if (JS_IsException(obj))
    return obj;
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
  JSValue GetCameraLookAt(JSContext* ctx) {
    return NewVec3(ctx, scene->GetLookAtPosition());
  }
  JSValue GetRotationCenter(JSContext* ctx) {
    return NewVec3(ctx, scene->GetRotationCenter());
  }
  float GetZoom(JSContext* ctx) {
    return scene->GetZoom();
  }
  float GetFOV(JSContext* ctx) {
    return scene->GetFOV();
  }
  JSValue GetCameraAngle(JSContext* ctx) {
    MQAngle angle = scene->GetCameraAngle();
    ValueHolder v(ctx);
    v.Set("bank", angle.bank);
    v.Set("head", angle.head);
    v.Set("pitch", angle.pitch);
    return v.GetValue();
  }
};

JSClassID MQSceneWrapper::class_id;

const JSCFunctionListEntry MQSceneWrapper::proto_funcs[] = {
    function_entry_getset("cameraPosition", method_wrapper_getter<&GetCameraPosition>),
    function_entry_getset("cameraLookAt", method_wrapper_getter<&GetCameraLookAt>),
    function_entry_getset("cameraAngle", method_wrapper_getter<&GetCameraAngle>),
    function_entry_getset("rotationCenter", method_wrapper_getter<&GetRotationCenter>),
    function_entry_getset("zoom", method_wrapper_getter<&GetZoom>),
    function_entry_getset("fov", method_wrapper_getter<&GetFOV>),
};

JSValue NewMQScene(JSContext* ctx, MQDocument doc) {
  JSValue obj = JS_NewObjectClass(ctx, MQSceneWrapper::class_id);
  if (JS_IsException(obj))
    return obj;
  JS_SetOpaque(obj, new MQSceneWrapper(doc->GetScene(0)));
  return obj;
}


//---------------------------------------------------------------------------------------------------------------------
// MQDocument
//---------------------------------------------------------------------------------------------------------------------

class MQDocumentWrapper {
public:
  MQDocument doc;
  static JSClassID class_id;
  static const JSCFunctionListEntry proto_funcs[];

  MQDocumentWrapper(MQDocument doc) : doc(doc) {}

  void Init(JSContext* ctx, JSValueConst this_obj, int argc, JSValueConst* argv) {
  }

  JSValue Compact() {
    doc->Compact();
    return JS_UNDEFINED;
  }

  void ClearSelect() {
    doc->ClearSelect(MQDOC_CLEARSELECT_ALL);
  }

  bool IsVertexSelected(int o, int v) {
    return doc->IsSelectVertex(o, v);
  }
  bool SetVertexSelected(int o, int v, bool s) {
    if (s) {
      return doc->AddSelectVertex(o, v);
    }
    else {
      return doc->DeleteSelectVertex(o, v);
    }
  }
  bool IsFaceSelected(int o, int f) {
    return  doc->IsSelectFace(o, f);
  }
  bool SetFaceSelected(int o, int f, bool s) {
    if (s) {
      return doc->AddSelectFace(o, f);
    }
    else {
      return doc->DeleteSelectFace(o, f);
    }
  }

  JSValue GetScene(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    return NewMQScene(ctx, doc);
  }


  JSValue GetObjects(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    ValueHolder ret(ctx, JS_NewArray(ctx));
    for (int i = 0; i < doc->GetObjectCount(); i++) {
      auto o = doc->GetObject(i);
      if (o != nullptr) {
        ret.Set(i, NewMQObject(ctx, o, i));
      }
    }
    JSValue data[]{ this_val };
    ret.Set("append", JS_NewCFunctionData(ctx, method_wrapper_bind<&MQDocumentWrapper::AddObject>, 1, 0, 1, data));
    ret.Set("remove", JS_NewCFunctionData(ctx, method_wrapper_bind<&MQDocumentWrapper::RemoveObject>, 1, 0, 1, data));
    return ret.GetValue();
  }

  JSValue AddObject(JSContext* ctx, JSValue obj) {
    MQObjectWrapper* o = (MQObjectWrapper*)JS_GetOpaque2(ctx, obj, MQObjectWrapper::class_id);
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
    MQObjectWrapper* o = (MQObjectWrapper*)JS_GetOpaque2(ctx, obj, MQObjectWrapper::class_id);
    if (o == nullptr) {
      return JS_EXCEPTION;
    }
    //doc->RemoveObject(o->obj);
    doc->DeleteObject(o->index);
    o->Reset();
    return JS_UNDEFINED;
  }

  JSValue GetMaterials(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    ValueHolder ret(ctx, JS_NewArray(ctx));
    for (int i = 0; i < doc->GetMaterialCount(); i++) {
      auto o = doc->GetMaterial(i);
      if (o != nullptr) {
        ret.Set(i, NewMQMaterial(ctx, o, i));
      }
    }
    JSValue data[]{ this_val };
    ret.Set("append", JS_NewCFunctionData(ctx, method_wrapper_bind<&MQDocumentWrapper::AddMaterial>, 1, 0, 1, data));
    ret.Set("remove", JS_NewCFunctionData(ctx, method_wrapper_bind<&MQDocumentWrapper::DeleteMaterial>, 1, 0, 1, data));
    return ret.GetValue();
  }

  JSValue AddMaterial(JSContext* ctx, JSValue obj) {
    MQMaterialWrapper* o = (MQMaterialWrapper*)JS_GetOpaque2(ctx, obj, MQMaterialWrapper::class_id);
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
    MQMaterialWrapper* o = (MQMaterialWrapper*)JS_GetOpaque2(ctx, obj, MQMaterialWrapper::class_id);
    if (o == nullptr) {
      return JS_EXCEPTION;
    }
    doc->DeleteMaterial(o->GetIndex(ctx));
    o->Reset();
    return JS_UNDEFINED;
  }

  JSValue Triangulate(JSContext* ctx, JSValue arg) {
    ValueHolder poly(ctx, arg, true);

    ValueHolder result(ctx, JS_NewArray(ctx));
    uint32_t len = poly.Length();
    if (len < 3) {
      return result.GetValue();
    }

    std::vector<MQPoint> points(len);
    for (uint32_t i = 0; i < len; i++) {
      points[i].x = poly[i]["x"].to<float>();
      points[i].y = poly[i]["y"].to<float>();
      points[i].z = poly[i]["z"].to<float>();
    }
    std::vector<int> indices((size_t)(len - 2) * 3);
    doc->Triangulate(&points[0], (int)points.size(), &indices[0], (int)indices.size());

    for (int i = 0; i < indices.size(); i++) {
      result.Set(i, indices[i]);
    }
    return result.GetValue();
  }
};

JSClassID MQDocumentWrapper::class_id;

const JSCFunctionListEntry MQDocumentWrapper::proto_funcs[] = {
    function_entry_getset("objects", method_wrapper_getter<&GetObjects>),
    function_entry_getset("materials", method_wrapper_getter<&GetMaterials>),
    function_entry_getset("scene", method_wrapper_getter<&GetScene>),
    function_entry("isVertexSelected", 2, method_wrapper<&IsVertexSelected>),
    function_entry("setVertexSelected", 3, method_wrapper<&SetVertexSelected>),
    function_entry("isFaceSelected", 2, method_wrapper<&IsFaceSelected>),
    function_entry("setFaceSelected", 3, method_wrapper<&SetFaceSelected>),
    function_entry("clearSelect", 0, method_wrapper<&ClearSelect>),
    function_entry("compact", 0, method_wrapper<&Compact>),
    function_entry("triangulate", 1, method_wrapper<&Triangulate>),
};


static int DocumentModuleInit(JSContext* ctx, JSModuleDef* m) {
  JSValue meta = JS_GetImportMeta(ctx, m);
  MQDocument doc = (MQDocument)JS_GetOpaque(meta, 0);
  JS_FreeValue(ctx, meta);

  return 0;
}

JSValue NewMQDocument(JSContext* ctx, MQDocumentWrapper* doc) {
  JSValue obj = JS_NewObjectClass(ctx, MQDocumentWrapper::class_id);
  if (JS_IsException(obj))
    return obj;
  JS_SetOpaque(obj, doc);
  doc->Init(ctx, obj, 0, nullptr);
  return obj;
}

void InstallMQDocument(JSContext* ctx, MQDocument doc) {
  NewClassProto<MQSceneWrapper>(ctx, "MQScene");
  NewClassProto<FaceWrapper>(ctx, "Face");
  NewClassProto<VertexArray>(ctx, "VertexArray", &VertexArray_exotic);
  NewClassProto<FaceArray>(ctx, "FaceArray", &FaceArray_exotic);
  JSValue proto = NewClassProto<MQDocumentWrapper>(ctx, "MQDocument");


  ValueHolder global(ctx, JS_GetGlobalObject(ctx));
  global.Set("document", NewMQDocument(ctx, new MQDocumentWrapper(doc)));
  global.Set("MQObject", newClassConstructor<MQObjectWrapper>(ctx, "MQObject"));
  global.Set("MQMaterial", newClassConstructor<MQMaterialWrapper>(ctx, "MQMaterial"));
}
