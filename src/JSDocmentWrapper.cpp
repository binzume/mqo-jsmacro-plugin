
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <sstream>
#include <vector>

#include "MQWidget.h"
#include "Utils.h"
#include "qjsutils.h"

class IndexedIteratable {
 public:
  virtual JSValue IteratorGet(JSContext* ctx, int index) = 0;
};

class IndexIterator {
  JSContext* ctx;
  JSValue hold;
  IndexedIteratable* target;
  int pos;
  int count;

 public:
  IndexIterator(JSContext* ctx, IndexedIteratable* t, int idx, int _count,
                JSValue hold)
      : target(t), pos(idx), ctx(ctx), hold(hold), count(_count) {
    JS_DupValue(ctx, hold);
  }
  ~IndexIterator() { JS_FreeValue(ctx, hold); }

  static JSClassID class_id;
  static const JSCFunctionListEntry proto_funcs[];

  JSValue Next(JSContext* ctx) {
    ValueHolder ret(ctx);
    if (pos < count) {
      ret.Set("value", target->IteratorGet(ctx, pos));
    } else {
      ret.Set("value", JS_UNDEFINED);
    }
    pos++;
    ret.Set("done", pos >= count);
    return ret.GetValue();
  }
};

JSClassID IndexIterator::class_id;

const JSCFunctionListEntry IndexIterator::proto_funcs[] = {
    function_entry<&Next>("next"),
};

JSValue NewIndexIterator(JSContext* ctx, IndexedIteratable* t, int start,
                         int end, JSValue hold) {
  JSValue obj = JS_NewObjectClass(ctx, IndexIterator::class_id);
  if (JS_IsException(obj)) return obj;
  JS_SetOpaque(obj, new IndexIterator(ctx, t, start, end, hold));
  return obj;
}

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

class VertexArray : public IndexedIteratable {
 public:
  MQObject obj;
  VertexArray(MQObject obj) : obj(obj) {}

  static JSClassID class_id;
  static const JSCFunctionListEntry proto_funcs[];

  int Length() { return obj->GetVertexCount(); }

  JSValue Append(JSContext* ctx, JSValueConst this_val, int argc,
                 JSValueConst* argv) {
    MQPoint p;
    if (argc >= 1 && JS_IsObject(argv[0])) {
      ValueHolder v(ctx, argv[0], true);
      p.x = v["x"].To<float>();
      p.y = v["y"].To<float>();
      p.z = v["z"].To<float>();
    } else if (argc >= 3) {
      p.x = convert_jsvalue<float>(ctx, argv[0]);
      p.y = convert_jsvalue<float>(ctx, argv[1]);
      p.z = convert_jsvalue<float>(ctx, argv[2]);
    }

    // char s[256];
    // sprintf(s, "%f %f %f", p.x, p.y, p.z);
    // debug_log(s);

    return to_jsvalue(ctx, obj->AddVertex(p));
  }

  JSValue GetVertex(JSContext* ctx, int index) {
    MQPoint p = obj->GetVertex(index);
    ValueHolder v(ctx);
    v.Set("x", p.x);
    v.Set("y", p.y);
    v.Set("z", p.z);
    v.Set("id", obj->GetVertexUniqueID(index));
    v.Set("refs", obj->GetVertexRefCount(index));
    return v.GetValue();
  }

  void SetVertex(JSContext* ctx, JSValue value, int index) {
    MQPoint p;
    ValueHolder v(ctx, value, true);
    p.x = v["x"].To<float>();
    p.y = v["y"].To<float>();
    p.z = v["z"].To<float>();
    obj->SetVertex(index, p);
  }
  bool DeleteVertex(JSContext* ctx, JSValue value) {
    return obj->DeleteVertex(convert_jsvalue<int>(ctx, value));
  }

  JSValue GetIterator(JSContext* ctx, JSValueConst this_val, int argc,
                      JSValueConst* argv) {
    return NewIndexIterator(ctx, this, 0, obj->GetFaceCount(), this_val);
  }
  JSValue IteratorGet(JSContext* ctx, int index) {
    return GetVertex(ctx, index);
  }
};

template <auto method>
int delete_property_handler(JSContext* ctx, JSValueConst obj, JSAtom prop) {
  JSValue v = JS_AtomToValue(ctx, prop);
  JSValue ret = invoke_method(method, ctx, obj, 1, &v);
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
    function_entry<&GetIterator>("[Symbol.iterator]"),
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

class FaceArray : public IndexedIteratable {
 public:
  MQObject obj;
  FaceArray(MQObject obj) : obj(obj) {}

  static JSClassID class_id;
  static const JSCFunctionListEntry proto_funcs[];

  int Length() { return obj->GetFaceCount(); }

  int AddFace(JSContext* ctx, JSValue points, int mat) {
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

  void SetFace(JSContext* ctx, JSValue value, int index) {
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
  bool DeleteFace(JSContext* ctx, JSValue value) {
    return obj->DeleteFace(convert_jsvalue<int>(ctx, value));
  }

  JSValue GetIterator(JSContext* ctx, JSValueConst this_val, int argc,
                      JSValueConst* argv) {
    return NewIndexIterator(ctx, this, 0, obj->GetFaceCount(), this_val);
  }
  JSValue IteratorGet(JSContext* ctx, int index) { return GetFace(ctx, index); }
};

static JSClassExoticMethods FaceArray_exotic{
    .get_own_property =
        indexed_propery_handler<&FaceArray::GetFace, &FaceArray::SetFace>,
    .delete_property = delete_property_handler<&FaceArray::DeleteFace>};

JSClassID FaceArray::class_id;

const JSCFunctionListEntry FaceArray::proto_funcs[] = {
    function_entry_getset<&Length>("length"),
    function_entry<&AddFace>("append"),
    function_entry<&GetIterator>("[Symbol.iterator]"),
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
  MQObjectWrapper(JSContext* ctx) : index(-1), obj(MQ_CreateObject()) {}

  void Init(JSContext* ctx, JSValueConst this_obj, int argc,
            JSValueConst* argv) {
    JS_SetPropertyStr(ctx, this_obj, "verts", NewVertexArray(ctx, obj));
    JS_SetPropertyStr(ctx, this_obj, "faces", NewFaceArray(ctx, obj));
    if (argc > 0 && JS_IsString(argv[0])) {
      obj->SetName(
          utf8ToSjis(convert_jsvalue<std::string>(ctx, argv[0])).c_str());
    }
  }

  void Reset() {
    obj = MQ_CreateObject();
    index = -1;
  }

  static JSClassID class_id;
  static const JSCFunctionListEntry proto_funcs[];

  int GetIndex(JSContext* ctx) { return index; }
  int GetId() { return obj->GetUniqueID(); }
  int GetType() { return obj->GetType(); }
  void SetType(int t) { obj->SetType(t); }
  bool Selected() { return obj->GetSelected(); }
  void SetSelected(bool s) { obj->SetSelected(s); }
  bool Visible() { return obj->GetVisible(); }
  void SetVisible(bool v) { obj->SetVisible(v); }
  std::string GetName(JSContext* ctx) {
    // char buf[256];
    // obj->GetName(buf, sizeof(buf));
    // return sjisToUtf8(buf);
    return sjisToUtf8(obj->GetName());
  }
  JSValue SetName(JSContext* ctx, std::string name) {
    obj->SetName(name.c_str());
    return JS_UNDEFINED;
  }
  void Compact() { obj->Compact(); }
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
};

JSClassID MQObjectWrapper::class_id;

const JSCFunctionListEntry MQObjectWrapper::proto_funcs[] = {
    function_entry_getset<&GetIndex>("index"),
    function_entry_getset<&GetId>("id"),
    function_entry_getset<&GetName, &SetName>("name"),
    function_entry_getset<&GetType, &SetType>("type"),
    function_entry_getset<&Visible, &SetVisible>("visible"),
    function_entry_getset<&Selected, &SetSelected>("selected"),
    function_entry<&Compact>("compact"),
    function_entry<&Freeze>("freeze"),
    function_entry<&Merge>("merge"),
    function_entry<&Clone>("clone"),
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
  MQMaterialWrapper(JSContext* ctx) : index(-1), mat(MQ_CreateMaterial()) {}

  static JSClassID class_id;
  static const JSCFunctionListEntry proto_funcs[];

  void Init(JSContext* ctx, JSValueConst this_obj, int argc,
            JSValueConst* argv) {
    if (argc > 0 && JS_IsString(argv[0])) {
      mat->SetName(
          utf8ToSjis(convert_jsvalue<std::string>(ctx, argv[0])).c_str());
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
    c.r = col["r"].To<float>();
    c.g = col["g"].To<float>();
    c.b = col["b"].To<float>();
    mat->SetColor(c);
    ValueHolder a = col["a"];
    if (!a.IsUndefined()) {
      mat->SetAlpha(a.To<float>());
    }
  }
};

JSClassID MQMaterialWrapper::class_id;

const JSCFunctionListEntry MQMaterialWrapper::proto_funcs[] = {
    function_entry_getset<&GetIndex>("index"),
    function_entry_getset<&GetId>("id"),
    function_entry_getset<&GetName, &SetName>("name"),
    function_entry_getset<&GetColor, &SetColor>("color"),
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
  JSValue GetCameraLookAt(JSContext* ctx) {
    return NewVec3(ctx, scene->GetLookAtPosition());
  }
  JSValue GetRotationCenter(JSContext* ctx) {
    return NewVec3(ctx, scene->GetRotationCenter());
  }
  float GetZoom(JSContext* ctx) { return scene->GetZoom(); }
  float GetFOV(JSContext* ctx) { return scene->GetFOV(); }
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
    function_entry_getset<&GetCameraPosition>("cameraPosition"),
    function_entry_getset<&GetCameraLookAt>("cameraLookAt"),
    function_entry_getset<&GetCameraAngle>("cameraAngle"),
    function_entry_getset<&GetRotationCenter>("rotationCenter"),
    function_entry_getset<&GetZoom>("zoom"),
    function_entry_getset<&GetFOV>("fov"),
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
  static JSClassID class_id;
  static const JSCFunctionListEntry proto_funcs[];

  MQDocumentWrapper(MQDocument doc) : doc(doc) {}

  void Init(JSContext* ctx, JSValueConst this_obj, int argc,
            JSValueConst* argv) {}

  JSValue Compact() {
    doc->Compact();
    return JS_UNDEFINED;
  }

  void ClearSelect() { doc->ClearSelect(MQDOC_CLEARSELECT_ALL); }

  bool IsVertexSelected(int o, int v) { return doc->IsSelectVertex(o, v); }
  bool SetVertexSelected(int o, int v, bool s) {
    if (s) {
      return doc->AddSelectVertex(o, v);
    } else {
      return doc->DeleteSelectVertex(o, v);
    }
  }
  bool IsFaceSelected(int o, int f) { return doc->IsSelectFace(o, f); }
  bool SetFaceSelected(int o, int f, bool s) {
    if (s) {
      return doc->AddSelectFace(o, f);
    } else {
      return doc->DeleteSelectFace(o, f);
    }
  }

  JSValue GetScene(JSContext* ctx, JSValueConst this_val, int argc,
                   JSValueConst* argv) {
    return NewMQScene(ctx, doc);
  }

  JSValue GetObjects(JSContext* ctx, JSValueConst this_val, int argc,
                     JSValueConst* argv) {
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
    // doc->RemoveObject(o->obj);
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

  JSValue Triangulate(JSContext* ctx, JSValue arg) {
    ValueHolder poly(ctx, arg, true);

    ValueHolder result(ctx, JS_NewArray(ctx));
    uint32_t len = poly.Length();
    if (len < 3) {
      return result.GetValue();
    }

    std::vector<MQPoint> points(len);
    for (uint32_t i = 0; i < len; i++) {
      points[i].x = poly[i]["x"].To<float>();
      points[i].y = poly[i]["y"].To<float>();
      points[i].z = poly[i]["z"].To<float>();
    }
    std::vector<int> indices((size_t)(len - 2) * 3);
    doc->Triangulate(&points[0], (int)points.size(), &indices[0],
                     (int)indices.size());

    for (int i = 0; i < indices.size(); i++) {
      result.Set(i, indices[i]);
    }
    return result.GetValue();
  }
};

JSClassID MQDocumentWrapper::class_id;

const JSCFunctionListEntry MQDocumentWrapper::proto_funcs[] = {
    function_entry_getset<&GetObjects>("objects"),
    function_entry_getset<&GetMaterials>("materials"),
    function_entry_getset<&GetScene>("scene"),
    function_entry<&IsVertexSelected>("isVertexSelected"),
    function_entry<&SetVertexSelected>("setVertexSelected"),
    function_entry<&IsFaceSelected>("isFaceSelected"),
    function_entry<&SetFaceSelected>("setFaceSelected"),
    function_entry<&ClearSelect>("clearSelect"),
    function_entry<&Compact>("compact"),
    function_entry<&Triangulate>("triangulate"),
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
  doc->Init(ctx, obj, 0, nullptr);
  return obj;
}

void InstallMQDocument(JSContext* ctx, MQDocument doc) {
  NewClassProto<MQSceneWrapper>(ctx, "MQScene");
  NewClassProto<FaceWrapper>(ctx, "Face");
  NewClassProto<VertexArray>(ctx, "VertexArray", &VertexArray_exotic);
  NewClassProto<FaceArray>(ctx, "FaceArray", &FaceArray_exotic);
  NewClassProto<IndexIterator>(ctx, "IndexIterator");
  JSValue proto = NewClassProto<MQDocumentWrapper>(ctx, "MQDocument");

  ValueHolder global(ctx, JS_GetGlobalObject(ctx));
  global.SetFree("document", NewMQDocument(ctx, new MQDocumentWrapper(doc)));
  global.SetFree("MQObject",
                 newClassConstructor<MQObjectWrapper>(ctx, "MQObject"));
  global.SetFree("MQMaterial",
                 newClassConstructor<MQMaterialWrapper>(ctx, "MQMaterial"));
}
