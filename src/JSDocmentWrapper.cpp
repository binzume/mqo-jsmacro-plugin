

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <codecvt>
#include "MQBasePlugin.h"
#include "MQ3DLib.h"

#include "libplatform/libplatform.h"
#include "v8.h"

//---------------------------------------------------------------------------------------------------------------------
// Utils
//---------------------------------------------------------------------------------------------------------------------

#define UTF8(s) v8::String::NewFromUtf8(isolate, s, v8::NewStringType::kNormal).ToLocalChecked()

void debug_log(const std::string s, int tag = 1);

static std::string utf8ToSjis(const std::string &str) {
	std::locale sjis(".932", std::locale::ctype);
	typedef std::codecvt<wchar_t, char, std::mbstate_t> mbCvt;
	const mbCvt& cvt = std::use_facet<mbCvt>(sjis);
	std::wstring_convert<mbCvt, wchar_t> sjisConverter(&cvt);
	std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> utf8Converter;
	return sjisConverter.to_bytes(utf8Converter.from_bytes(str));
}

static std::string sjisToUtf8(const std::string &str) {
	std::locale sjis(".932", std::locale::ctype);
	typedef std::codecvt<wchar_t, char, std::mbstate_t> mbCvt;
	const mbCvt& cvt = std::use_facet<mbCvt>(sjis);
	std::wstring_convert<mbCvt, wchar_t> sjisConverter(&cvt);
	std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> utf8Converter;
	return utf8Converter.to_bytes(sjisConverter.from_bytes(str));
}

template<typename T>
struct Holder {
	v8::Persistent<v8::Object> ref;
	T obj;
	Holder(v8::Isolate *isolate, v8::Handle<v8::Object> target, T ptr) : ref(isolate, target), obj(ptr) {
		ref.SetWeak<Holder<T>>(this, Holder<T>::Dispose, WeakCallbackType::kParameter);
		ref.MarkIndependent();
	}

	static void Dispose(const v8::WeakCallbackInfo<Holder<T>>& data) {
		if (data.GetParameter()->obj->GetUniqueID() == 0) {
			data.GetParameter()->obj->DeleteThis();
		}
		delete data.GetParameter();
	}
};

template<typename T, typename T0>
static inline T GetInternal(const v8::FunctionCallbackInfo<T0>& args) {
	v8::Local<v8::Object> self = args.Holder();
	return static_cast<T>(self->GetInternalField(0).As<v8::External>()->Value());
}
template<typename T, typename T0>
static inline T GetInternal(const v8::PropertyCallbackInfo<T0>& info) {
	v8::Local<v8::Object> self = info.Holder();
	return static_cast<T>(self->GetInternalField(0).As<v8::External>()->Value());
}

using namespace v8;

//---------------------------------------------------------------------------------------------------------------------
// MQObject
//---------------------------------------------------------------------------------------------------------------------

static void GetVertCount(const Local<String> property, const PropertyCallbackInfo<Value>& info) {
	Isolate* isolate = info.GetIsolate();
	Local<Object> self = info.Holder();
	Local<Object> _obj = Local<Object>::Cast(self->Get(UTF8("_obj")));
	MQObject obj = static_cast<MQObject>(_obj->GetInternalField(0).As<External>()->Value());

	info.GetReturnValue().Set(obj->GetVertexCount());
}

static void GetVertex(uint32_t index, const PropertyCallbackInfo<Value>& info) {
	Isolate* isolate = info.GetIsolate();
	Local<Object> self = info.Holder();
	Local<Object> _obj = Local<Object>::Cast(self->Get(UTF8("_obj")));
	MQObject obj = static_cast<MQObject>(_obj->GetInternalField(0).As<External>()->Value());

	MQPoint p = obj->GetVertex(index);

	Handle<Array> vert = Array::New(isolate, 3);
	if (vert.IsEmpty())
		return info.GetReturnValue().Set(Handle<Array>());

	vert->Set(UTF8("refs"), Integer::New(isolate, obj->GetVertexRefCount(index)));
	vert->Set(0, Number::New(isolate, p.x));
	vert->Set(1, Number::New(isolate, p.y));
	vert->Set(2, Number::New(isolate, p.z));
	info.GetReturnValue().Set(vert);
}

static void SetVertex(uint32_t index, Local<Value> value, const PropertyCallbackInfo<Value>& info) {
	Isolate* isolate = info.GetIsolate();
	Local<Object> self = info.Holder();
	Local<Object> _obj = Local<Object>::Cast(self->Get(UTF8("_obj")));
	MQObject obj = static_cast<MQObject>(_obj->GetInternalField(0).As<External>()->Value());
	Local<Array> coord = value.As<Array>();

	MQPoint p;
	p.x = (float)coord->Get(0).As<Number>()->Value();
	p.y = (float)coord->Get(1).As<Number>()->Value();
	p.z = (float)coord->Get(2).As<Number>()->Value();

	obj->SetVertex(index, p);
	info.GetReturnValue().Set(value);
}

static void AddVertex(const FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = args.GetIsolate();
	Local<Object> self = args.Holder();
	Local<Object> _obj = self->Get(UTF8("_obj")).As<Object>();
	MQObject obj = static_cast<MQObject>(_obj->GetInternalField(0).As<External>()->Value());

	if (args.kArgsLength >= 3) {
		MQPoint p;
		p.x = (float)args[0].As<Number>()->Value();
		p.y = (float)args[1].As<Number>()->Value();
		p.z = (float)args[2].As<Number>()->Value();
		int index = obj->AddVertex(p);
		args.GetReturnValue().Set(index);
	}
}

static void DeleteVertex(uint32_t index, const PropertyCallbackInfo<Boolean>& info) {
	Isolate* isolate = info.GetIsolate();
	Local<Object> self = info.Holder();
	Local<Object> _obj = self->Get(UTF8("_obj")).As<Object>();
	MQObject obj = static_cast<MQObject>(_obj->GetInternalField(0).As<External>()->Value());

	obj->DeleteVertex(index);
}

static void GetVerts(const Local<Name> property, const PropertyCallbackInfo<Value>& info) {
	Isolate* isolate = info.GetIsolate();
	Local<Object> self = info.Holder();
	MQObject obj = static_cast<MQObject>(self->GetInternalField(0).As<External>()->Value());

	auto vertst = ObjectTemplate::New(isolate);
	vertst->SetIndexedPropertyHandler(GetVertex, SetVertex, nullptr, DeleteVertex);
	vertst->SetAccessor(UTF8("length"), GetVertCount);
	vertst->Set(UTF8("append"), FunctionTemplate::New(isolate, AddVertex));
	Local<Object> verts = vertst->NewInstance();
	verts->Set(UTF8("_obj"), self);
	verts->SetPrototype(Array::New(isolate));

	info.GetReturnValue().Set(verts);
}

static void GetFaceCount(const Local<String> property, const PropertyCallbackInfo<Value>& info) {
	Isolate* isolate = info.GetIsolate();
	Local<Object> self = info.Holder();
	Local<Object> _obj = Local<Object>::Cast(self->Get(UTF8("_obj")));
	MQObject obj = static_cast<MQObject>(_obj->GetInternalField(0).As<External>()->Value());

	info.GetReturnValue().Set(obj->GetFaceCount());
}

static void InvertFace(const FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = args.GetIsolate();
	Local<Object> self = args.Holder();
	Local<Object> _obj = self->Get(UTF8("_obj")).As<Object>();
	MQObject obj = static_cast<MQObject>(_obj->GetInternalField(0).As<External>()->Value());
	uint32_t index = self->Get(UTF8("index")).As<Integer>()->Uint32Value();
	obj->InvertFace(index);
}

static void GetFace(uint32_t index, const PropertyCallbackInfo<Value>& info) {
	Isolate* isolate = info.GetIsolate();
	Local<Object> self = info.Holder();
	Local<Object> _obj = self->Get(UTF8("_obj")).As<Object>();
	MQObject obj = static_cast<MQObject>(_obj->GetInternalField(0).As<External>()->Value());

	const int count = obj->GetFacePointCount(index);
	if (count == 0) {
		info.GetReturnValue().SetUndefined();
		return;
	}

	auto face = Object::New(isolate);
	face->Set(UTF8("_obj"), _obj);
	face->Set(UTF8("index"), Integer::New(isolate, index));
	int *points = new int[count];
	obj->GetFacePointArray(index, points);

	Handle<Array> array = Array::New(isolate, count);
	for (int i = 0; i < count; i++) {
		array->Set(i, Integer::New(isolate, points[i]));
	}
	delete[] points;

	face->Set(UTF8("points"), array);
	face->Set(UTF8("material"), Integer::New(isolate, obj->GetFaceMaterial(index)));
	face->Set(UTF8("invert"), Function::New(isolate, InvertFace));

	info.GetReturnValue().Set(face);
}

static void AddFace(const FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = args.GetIsolate();
	Local<Object> self = args.Holder();
	Local<Object> _obj = self->Get(UTF8("_obj")).As<Object>();
	MQObject obj = static_cast<MQObject>(_obj->GetInternalField(0).As<External>()->Value());

	if (args.kArgsLength >= 1) {
		Local<Array> poly = args[0].As<Array>();
		int count = poly->Length();
		int *points = new int[count];
		for (int i = 0; i < count; i++) {
			points[i] = (int)poly->Get(i).As<Integer>()->Value();
		}
		int index = obj->AddFace(count, points);
		delete[] points;
		if (args.kArgsLength >= 2) {
			obj->SetFaceMaterial(index, args[1].As<Integer>()->Int32Value());
		}
		args.GetReturnValue().Set(index);
	}
}

static void DeleteFace(uint32_t index, const PropertyCallbackInfo<Boolean>& info) {
	Isolate* isolate = info.GetIsolate();
	Local<Object> self = info.Holder();
	Local<Object> _obj = self->Get(UTF8("_obj")).As<Object>();
	MQObject obj = static_cast<MQObject>(_obj->GetInternalField(0).As<External>()->Value());

	obj->DeleteFace(index);
}

static void GetFaces(const Local<Name> property, const PropertyCallbackInfo<Value>& info) {
	Isolate* isolate = info.GetIsolate();
	MQObject obj = GetInternal<MQObject>(info);

	Local<Object> self = info.Holder();
	auto facest = ObjectTemplate::New(isolate);
	facest->SetIndexedPropertyHandler(GetFace, nullptr, nullptr, DeleteFace);
	facest->SetAccessor(UTF8("length"), GetFaceCount);
	facest->Set(UTF8("append"), FunctionTemplate::New(isolate, AddFace));
	Local<Object> faces = facest->NewInstance();
	faces->Set(UTF8("_obj"), self);
	faces->SetPrototype(Array::New(isolate));
	info.GetReturnValue().Set(faces);
}


static void GetObjectId(const Local<String> property, const PropertyCallbackInfo<Value>& info) {
	Isolate* isolate = info.GetIsolate();
	MQObject obj = GetInternal<MQObject>(info);

	info.GetReturnValue().Set(obj->GetUniqueID());
}

static void GetObjectName(const Local<String> property, const PropertyCallbackInfo<Value>& info) {
	Isolate* isolate = info.GetIsolate();
	MQObject obj = GetInternal<MQObject>(info);

	info.GetReturnValue().Set(UTF8(sjisToUtf8(obj->GetName()).c_str()));
}

static void SetObjectName(const Local<String> property, Local<Value> value, const PropertyCallbackInfo<void>& info) {
	Isolate* isolate = info.GetIsolate();
	MQObject obj = GetInternal<MQObject>(info);

	obj->SetName(utf8ToSjis(*String::Utf8Value(value)).c_str());
}

static void GetObjectVisible(const Local<String> property, const PropertyCallbackInfo<Value>& info) {
	Isolate* isolate = info.GetIsolate();
	MQObject obj = GetInternal<MQObject>(info);

	info.GetReturnValue().Set(obj->GetVisible() != 0);
}

static void SetObjectVisible(const Local<String> property, Local<Value> value, const PropertyCallbackInfo<void>& info) {
	Isolate* isolate = info.GetIsolate();
	MQObject obj = GetInternal<MQObject>(info);

	obj->SetVisible(value.As<Boolean>()->Value() ? 0xFFFFFFFF : 0);
}

static void GetObjectSelected(const Local<String> property, const PropertyCallbackInfo<Value>& info) {
	Isolate* isolate = info.GetIsolate();
	MQObject obj = GetInternal<MQObject>(info);

	info.GetReturnValue().Set(obj->GetSelected() != 0);
}

static void SetObjectSelected(const Local<String> property, Local<Value> value, const PropertyCallbackInfo<void>& info) {
	Isolate* isolate = info.GetIsolate();
	MQObject obj = GetInternal<MQObject>(info);

	obj->SetSelected(value.As<Boolean>()->Value());
}

static void FreezeObject(const FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = args.GetIsolate();
	MQObject obj = GetInternal<MQObject>(args);

	int flag;
	if (args.kArgsLength > 0) {
		flag = args[0].As<Integer>()->Int32Value();
	} else {
		flag = MQOBJECT_FREEZE_ALL;
	}
	obj->Freeze(flag);
}

static void CompactObject(const FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = args.GetIsolate();
	MQObject obj = GetInternal<MQObject>(args);

	obj->Compact();
}

static void MergeObject(const FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = args.GetIsolate();
	MQObject obj = GetInternal<MQObject>(args);

	MQObject src = static_cast<MQObject>(args[0].As<Object>()->GetInternalField(0).As<External>()->Value());
	obj->Merge(src);
}

static Handle<Object> MQObjectWrap(Isolate* isolate, MQObject o) {
	Local<Value> args[] = { External::New(isolate, o) };
	return isolate->GetCurrentContext()->Global()
		->Get(UTF8("MQObject")).As<Function>()->CallAsConstructor(1, args).As<Object>();
}

static void CloneObject(const FunctionCallbackInfo<Value>& info) {
	Isolate* isolate = info.GetIsolate();
	MQObject obj = GetInternal<MQObject>(info);

	MQObject newObject = obj->Clone();

	info.GetReturnValue().Set(MQObjectWrap(isolate, newObject));
}

static void InitMQObjectTemplate(Isolate* isolate, Local<ObjectTemplate> objt) {
	objt->SetInternalFieldCount(1);
	objt->SetAccessor(UTF8("id"), GetObjectId);
	objt->SetAccessor(UTF8("name"), GetObjectName, SetObjectName);
	objt->SetAccessor(UTF8("visible"), GetObjectVisible, SetObjectVisible);
	objt->SetAccessor(UTF8("selected"), GetObjectSelected, SetObjectSelected);
	objt->Set(UTF8("clone"), FunctionTemplate::New(isolate, CloneObject));
	objt->Set(UTF8("freeze"), FunctionTemplate::New(isolate, FreezeObject));
	objt->Set(UTF8("compact"), FunctionTemplate::New(isolate, CompactObject));
	objt->Set(UTF8("merge"), FunctionTemplate::New(isolate, MergeObject));
	objt->SetLazyDataProperty(UTF8("verts"), GetVerts);
	objt->SetLazyDataProperty(UTF8("faces"), GetFaces);
}

static void MQObjectConstructor(const FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = args.GetIsolate();
	Local<Object> self = args.This();
	if (args.kArgsLength >= 1 && args[0]->IsExternal()) {
		self->SetInternalField(0, args[0]);
	} else {
		MQObject obj = MQ_CreateObject();
		self->SetInternalField(0, External::New(isolate, obj));
		if (args.kArgsLength >= 1 && args[0]->IsString()) {
			obj->SetName(utf8ToSjis(*String::Utf8Value(args[0])).c_str());
		} else {
			MQDocument doc = static_cast<MQDocument>(args.Data().As<External>()->Value());
			char name[256];
			doc->GetUnusedObjectName(name, sizeof(name));
			obj->SetName(name);
		}
	}

	MQObject obj = static_cast<MQObject>(self->GetInternalField(0).As<External>()->Value());

	new Holder<MQObject>(isolate, self, obj); // delete when GC.

	args.GetReturnValue().Set(self);
}

static void AddObject(const FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = args.GetIsolate();
	MQDocument doc = static_cast<MQDocument>(args.Data().As<External>()->Value());
	Local<Object> _obj = args[0].As<Object>();
	MQObject obj = static_cast<MQObject>(_obj->GetInternalField(0).As<External>()->Value());

	int index = doc->AddObject(obj);
	args.GetReturnValue().Set(index);
}

static void DeleteObject(const FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = args.GetIsolate();
	MQDocument doc = static_cast<MQDocument>(args.Data().As<External>()->Value());
	Local<Object> _obj = args[0].As<Object>();
	MQObject obj = static_cast<MQObject>(_obj->GetInternalField(0).As<External>()->Value());

	int idx = doc->GetObjectIndex(obj);
	if (idx >= 0) {
		doc->DeleteObject(idx);
	}
	args.GetReturnValue().SetUndefined();
}

static Handle<Array> MQObjectsAsArray(Isolate* isolate, MQDocument doc) {
	Handle<Array> array = Array::New(isolate, doc->GetObjectCount());
	for (int i = 0; i < doc->GetObjectCount(); i++) {
		if (doc->GetObject(i) != nullptr) {
			array->Set(i, MQObjectWrap(isolate, doc->GetObject(i)));
		}
	}
	array->Set(UTF8("append"), Function::New(isolate, AddObject, External::New(isolate, doc)));
	array->Set(UTF8("remove"), Function::New(isolate, DeleteObject, External::New(isolate, doc)));
	return array;
}

static void GetObjects(const Local<Name> property, const PropertyCallbackInfo<Value>& info) {
	Isolate* isolate = info.GetIsolate();
	MQDocument doc = GetInternal<MQDocument>(info);

	info.GetReturnValue().Set(MQObjectsAsArray(isolate, doc));
}


//---------------------------------------------------------------------------------------------------------------------
// MQMaterial
//---------------------------------------------------------------------------------------------------------------------
static void GetMaterialId(const Local<String> property, const PropertyCallbackInfo<Value>& info) {
	Isolate* isolate = info.GetIsolate();
	MQMaterial obj = GetInternal<MQMaterial>(info);

	info.GetReturnValue().Set(obj->GetUniqueID());
}

static void GetMaterialName(const Local<String> property, const PropertyCallbackInfo<Value>& info) {
	Isolate* isolate = info.GetIsolate();
	MQMaterial obj = GetInternal<MQMaterial>(info);

	info.GetReturnValue().Set(UTF8(sjisToUtf8(obj->GetName()).c_str()));
}

static void SetMaterialName(const Local<String> property, Local<Value> value, const PropertyCallbackInfo<void>& info) {
	Isolate* isolate = info.GetIsolate();
	MQMaterial obj = GetInternal<MQMaterial>(info);

	obj->SetName(utf8ToSjis(*String::Utf8Value(value)).c_str());
}

static void GetMaterialColor(const Local<String> property, const PropertyCallbackInfo<Value>& info) {
	Isolate* isolate = info.GetIsolate();
	MQMaterial m = GetInternal<MQMaterial>(info);

	MQColor color = m->GetColor();
	float alpha = m->GetAlpha();

	auto c = Object::New(isolate);
	c->Set(UTF8("r"), Number::New(isolate, color.r));
	c->Set(UTF8("g"), Number::New(isolate, color.g));
	c->Set(UTF8("b"), Number::New(isolate, color.b));
	c->Set(UTF8("a"), Number::New(isolate, alpha));

	info.GetReturnValue().Set(c);
}

static void SetMaterialColor(const Local<String> property, Local<Value> value, const PropertyCallbackInfo<void>& info) {
	Isolate* isolate = info.GetIsolate();
	MQMaterial m = GetInternal<MQMaterial>(info);

	Local<Object> c = value.As<Object>();

	MQColor color;
	color.r = (float)c->Get(UTF8("r")).As<Number>()->Value();
	color.g = (float)c->Get(UTF8("g")).As<Number>()->Value();
	color.b = (float)c->Get(UTF8("b")).As<Number>()->Value();
	m->SetColor(color);
	if (c->Get(UTF8("a"))->IsNumber()) {
		m->SetAlpha((float)c->Get(UTF8("a")).As<Number>()->Value());
	}
}

static Handle<Object> MQMaterialWrap(Isolate* isolate, MQMaterial m, int index) {
	auto objt = ObjectTemplate::New(isolate);
	objt->SetInternalFieldCount(1);
	objt->SetAccessor(UTF8("id"), GetMaterialId);
	objt->SetAccessor(UTF8("name"), GetMaterialName, SetMaterialName);
	objt->SetAccessor(UTF8("color"), GetMaterialColor, SetMaterialColor);

	Local<Object> obj = objt->NewInstance();
	obj->SetInternalField(0, External::New(isolate, m));
	obj->Set(UTF8("index"), Integer::New(isolate, index));
	return obj;
}

static void MQMaterialConstructor(const FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = args.GetIsolate();
	Local<Object> self = args.This();
	if (args.kArgsLength >= 1 && args[0]->IsExternal()) {
		self->SetInternalField(0, args[0]);
	} else {
		MQMaterial obj = MQ_CreateMaterial();
		self->SetInternalField(0, External::New(isolate, obj));
		if (args.kArgsLength >= 1 && args[0]->IsString()) {
			obj->SetName(utf8ToSjis(*String::Utf8Value(args[0])).c_str());
		} else {
			MQDocument doc = static_cast<MQDocument>(args.Data().As<External>()->Value());
			char name[256];
			doc->GetUnusedMaterialName(name, sizeof(name));
			obj->SetName(name);
		}
	}

	MQMaterial obj = static_cast<MQMaterial>(self->GetInternalField(0).As<External>()->Value());

	new Holder<MQMaterial>(isolate, self, obj); // delete when GC.

	args.GetReturnValue().Set(self);
}

static void AddMaterial(const FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = args.GetIsolate();
	MQDocument doc = static_cast<MQDocument>(args.Data().As<External>()->Value());
	Local<Object> _obj = args[0].As<Object>();
	MQMaterial obj = static_cast<MQMaterial>(_obj->GetInternalField(0).As<External>()->Value());

	int index = doc->AddMaterial(obj);
	args.GetReturnValue().Set(index);
}

static void DeleteMaterial(const FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = args.GetIsolate();
	MQDocument doc = static_cast<MQDocument>(args.Data().As<External>()->Value());
	Local<Object> _obj = args[0].As<Object>();
	MQObject obj = static_cast<MQObject>(_obj->GetInternalField(0).As<External>()->Value());

	doc->DeleteMaterial(_obj->Get(UTF8("index"))->Int32Value());
	args.GetReturnValue().SetUndefined();
}

static void GetMaterials(const Local<Name> property, const PropertyCallbackInfo<Value>& info) {
	Isolate* isolate = info.GetIsolate();
	MQDocument doc = GetInternal<MQDocument>(info);

	Handle<Array> array = Array::New(isolate, doc->GetMaterialCount());
	for (int i = 0; i < doc->GetMaterialCount(); i++) {
		if (doc->GetMaterial(i) != nullptr) {
			array->Set(i, MQMaterialWrap(isolate, doc->GetMaterial(i), i));
		}
	}
	array->Set(UTF8("append"), Function::New(isolate, AddMaterial, External::New(isolate, doc)));
	array->Set(UTF8("remove"), Function::New(isolate, DeleteMaterial, External::New(isolate, doc)));
	info.GetReturnValue().Set(array);
}

//---------------------------------------------------------------------------------------------------------------------
// MQScene
//---------------------------------------------------------------------------------------------------------------------
static void GetCameraPosition(const Local<String> property, const PropertyCallbackInfo<Value>& info) {
	Isolate* isolate = info.GetIsolate();
	MQScene s = GetInternal<MQScene>(info);

	MQPoint p = s->GetCameraPosition();
	auto obj = Object::New(isolate);
	obj->Set(UTF8("x"), Number::New(isolate, p.x));
	obj->Set(UTF8("y"), Number::New(isolate, p.y));
	obj->Set(UTF8("z"), Number::New(isolate, p.z));

	info.GetReturnValue().Set(obj);
}

static void SetCameraPosition(const Local<String> property, Local<Value> value, const PropertyCallbackInfo<void>& info) {
	Isolate* isolate = info.GetIsolate();
	MQScene s = GetInternal<MQScene>(info);

	MQPoint p;
	p.x = (float)value.As<Object>()->Get(UTF8("x")).As<Number>()->NumberValue();
	p.y = (float)value.As<Object>()->Get(UTF8("y")).As<Number>()->NumberValue();
	p.z = (float)value.As<Object>()->Get(UTF8("z")).As<Number>()->NumberValue();
	s->SetCameraPosition(p);
}

static void GetCameraLookAt(const Local<String> property, const PropertyCallbackInfo<Value>& info) {
	Isolate* isolate = info.GetIsolate();
	MQScene s = GetInternal<MQScene>(info);

	MQPoint p = s->GetLookAtPosition();
	auto obj = Object::New(isolate);
	obj->Set(UTF8("x"), Number::New(isolate, p.x));
	obj->Set(UTF8("y"), Number::New(isolate, p.y));
	obj->Set(UTF8("z"), Number::New(isolate, p.z));

	info.GetReturnValue().Set(obj);
}

static void SetCameraLookAt(const Local<String> property, Local<Value> value, const PropertyCallbackInfo<void>& info) {
	Isolate* isolate = info.GetIsolate();
	MQScene s = GetInternal<MQScene>(info);

	MQPoint p;
	p.x = (float)value.As<Object>()->Get(UTF8("x")).As<Number>()->NumberValue();
	p.y = (float)value.As<Object>()->Get(UTF8("y")).As<Number>()->NumberValue();
	p.z = (float)value.As<Object>()->Get(UTF8("z")).As<Number>()->NumberValue();
	s->SetLookAtPosition(p);
}

static void GetCameraAngle(const Local<String> property, const PropertyCallbackInfo<Value>& info) {
	Isolate* isolate = info.GetIsolate();
	MQScene s = GetInternal<MQScene>(info);

	MQAngle p = s->GetCameraAngle();
	auto obj = Object::New(isolate);
	obj->Set(UTF8("bank"), Number::New(isolate, p.bank));
	obj->Set(UTF8("head"), Number::New(isolate, p.head));
	obj->Set(UTF8("pitch"), Number::New(isolate, p.pitch));

	info.GetReturnValue().Set(obj);
}

static void SetCameraAngle(const Local<String> property, Local<Value> value, const PropertyCallbackInfo<void>& info) {
	Isolate* isolate = info.GetIsolate();
	MQScene s = GetInternal<MQScene>(info);

	MQAngle p;
	p.bank = (float)value.As<Object>()->Get(UTF8("bank")).As<Number>()->NumberValue();
	p.head = (float)value.As<Object>()->Get(UTF8("head")).As<Number>()->NumberValue();
	p.pitch = (float)value.As<Object>()->Get(UTF8("pitch")).As<Number>()->NumberValue();
	s->SetCameraAngle(p);
}

static void GetScene(const Local<Name> property, const PropertyCallbackInfo<Value>& info) {
	Isolate* isolate = info.GetIsolate();
	MQDocument doc = GetInternal<MQDocument>(info);

	MQScene scene = doc->GetScene(0);

	auto objt = ObjectTemplate::New(isolate);
	objt->SetInternalFieldCount(1);
	objt->SetAccessor(UTF8("cameraPosition"), GetCameraPosition, SetCameraPosition);
	objt->SetAccessor(UTF8("cameraAngle"), GetCameraAngle, SetCameraAngle);
	objt->SetAccessor(UTF8("cameraLookAt"), GetCameraLookAt, SetCameraLookAt);

	Local<Object> s = objt->NewInstance();
	s->SetInternalField(0, External::New(isolate, scene));

	info.GetReturnValue().Set(s);
}

//---------------------------------------------------------------------------------------------------------------------
// MQDocument
//---------------------------------------------------------------------------------------------------------------------

static void CompactDocument(const FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = args.GetIsolate();
	MQDocument doc = GetInternal<MQDocument>(args);

	doc->Compact();
	args.GetReturnValue().SetUndefined();
}

static void ClearSelect(const FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = args.GetIsolate();
	MQDocument doc = GetInternal<MQDocument>(args);

	doc->ClearSelect(MQDOC_CLEARSELECT_ALL);
	args.GetReturnValue().SetUndefined();
}

static void GetSelectetVertex(const FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = args.GetIsolate();
	MQDocument doc = GetInternal<MQDocument>(args);

	Handle<Array> array = Array::New(isolate);
	int n = 0;
	for (int i = 0; i < doc->GetObjectCount(); i++) {
		MQObject obj = doc->GetObject(i);
		if (obj != nullptr) {
			int vs = obj->GetVertexCount();
			for (int j = 0; j < vs; j++) {
				if (doc->IsSelectVertex(i, j)) {
					Handle<Array> selected = Array::New(isolate, 2);
					selected->Set(0, Integer::New(isolate, i));
					selected->Set(1, Integer::New(isolate, j));
					array->Set(n, selected);
					n++;
				}
			}
		}
	}
	args.GetReturnValue().Set(array);
}

static Local<Object> NewDocumentObject(Isolate* isolate, MQDocument doc) {
	auto obj = ObjectTemplate::New(isolate);
	obj->SetInternalFieldCount(1);
	obj->Set(UTF8("compact"), FunctionTemplate::New(isolate, CompactDocument));
	obj->Set(UTF8("clearSelect"), FunctionTemplate::New(isolate, ClearSelect));
	obj->Set(UTF8("getSelectedVertexes"), FunctionTemplate::New(isolate, GetSelectetVertex));
	obj->SetLazyDataProperty(UTF8("objects"), GetObjects);
	obj->SetLazyDataProperty(UTF8("materials"), GetMaterials);
	obj->SetAccessor(UTF8("scene"), GetScene);

	Local<Object> lobj = obj->NewInstance();
	lobj->SetInternalField(0, External::New(isolate, doc));
	return lobj;
}

static void GetDocument(const Local<Name> property, const PropertyCallbackInfo<Value>& info) {
	MQDocument doc = static_cast<MQDocument>(info.Data().As<External>()->Value());
	info.GetReturnValue().Set(NewDocumentObject(info.GetIsolate(), doc));
}

void InstallMQDocument(Local<ObjectTemplate> global, Isolate* isolate, MQDocument doc) {
	auto objcectConstructor = FunctionTemplate::New(isolate, MQObjectConstructor, External::New(isolate, doc));
	objcectConstructor->SetClassName(UTF8("MQObject"));
	InitMQObjectTemplate(isolate, objcectConstructor->InstanceTemplate());

	auto materialConstructor = FunctionTemplate::New(isolate, MQMaterialConstructor, External::New(isolate, doc));
	materialConstructor->SetClassName(UTF8("MQMaterial"));
	// InitMQMaterialTemplate(isolate, materialConstructor->InstanceTemplate());
	auto objt = materialConstructor->InstanceTemplate();
	objt->SetInternalFieldCount(1);
	objt->SetAccessor(UTF8("id"), GetMaterialId);
	objt->SetAccessor(UTF8("name"), GetMaterialName, SetMaterialName);
	objt->SetAccessor(UTF8("color"), GetMaterialColor, SetMaterialColor);

	global->Set(UTF8("MQObject"), objcectConstructor, PropertyAttribute::ReadOnly);
	global->Set(UTF8("MQMaterial"), materialConstructor, PropertyAttribute::ReadOnly);
	global->SetLazyDataProperty(UTF8("document"), GetDocument, External::New(isolate, doc));
	//global->Set(UTF8("document"), NewDocumentObject(isolate, doc));
}