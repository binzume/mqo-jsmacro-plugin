

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "MQBasePlugin.h"
#include "MQ3DLib.h"

#include "libplatform/libplatform.h"
#include "v8.h"

#define UTF8(s) v8::String::NewFromUtf8(isolate, s, v8::NewStringType::kNormal).ToLocalChecked()

using namespace v8;


static void GetVertCount(const Local<String> property, const PropertyCallbackInfo<Value>& info)
{
	Isolate* isolate = info.GetIsolate();
	Local<Object> self = info.Holder();
	Local<Object> _obj = Local<Object>::Cast(self->Get(UTF8("_obj")));
	MQObject obj = static_cast<MQObject>(_obj->GetInternalField(0).As<External>()->Value());

	info.GetReturnValue().Set(Integer::New(isolate, obj->GetVertexCount()));
}

static void GetVertex(uint32_t index, const PropertyCallbackInfo<Value>& info)
{
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

static void SetVertex(uint32_t index, Local<Value> value, const PropertyCallbackInfo<Value>& info)
{
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
		args.GetReturnValue().Set(Integer::New(isolate, index));
	}
}

static void DeleteVertex(uint32_t index, const PropertyCallbackInfo<Boolean>& info) {
	Isolate* isolate = info.GetIsolate();
	Local<Object> self = info.Holder();
	Local<Object> _obj = self->Get(UTF8("_obj")).As<Object>();
	MQObject obj = static_cast<MQObject>(_obj->GetInternalField(0).As<External>()->Value());

	obj->DeleteVertex(index);
}


static void GetFaceCount(const Local<String> property, const PropertyCallbackInfo<Value>& info)
{
	Isolate* isolate = info.GetIsolate();
	Local<Object> self = info.Holder();
	Local<Object> _obj = Local<Object>::Cast(self->Get(UTF8("_obj")));
	MQObject obj = static_cast<MQObject>(_obj->GetInternalField(0).As<External>()->Value());

	info.GetReturnValue().Set(Integer::New(isolate, obj->GetFaceCount()));
}

static void InvertFace(const FunctionCallbackInfo<Value>& args)
{
	Isolate* isolate = args.GetIsolate();
	Local<Object> self = args.Holder();
	Local<Object> _obj = self->Get(UTF8("_obj")).As<Object>();
	MQObject obj = static_cast<MQObject>(_obj->GetInternalField(0).As<External>()->Value());
	uint32_t index = self->Get(UTF8("index")).As<Integer>()->Uint32Value();
	obj->InvertFace(index);
}

static void GetFace(uint32_t index, const PropertyCallbackInfo<Value>& info)
{
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
		args.GetReturnValue().Set(Integer::New(isolate, index));
	}
}
static void DeleteFace(uint32_t index, const PropertyCallbackInfo<Boolean>& info) {
	Isolate* isolate = info.GetIsolate();
	Local<Object> self = info.Holder();
	Local<Object> _obj = self->Get(UTF8("_obj")).As<Object>();
	MQObject obj = static_cast<MQObject>(_obj->GetInternalField(0).As<External>()->Value());

	obj->DeleteFace(index);
}
#include <codecvt>


static void GetObjectName(const Local<String> property, const PropertyCallbackInfo<Value>& info)
{
	Isolate* isolate = info.GetIsolate();
	Local<Object> self = info.Holder();
	MQObject obj = static_cast<MQObject>(self->GetInternalField(0).As<External>()->Value());

	std::locale sjis(".932", std::locale::ctype);
	typedef std::codecvt<wchar_t, char, std::mbstate_t> mbCvt;
	const mbCvt& cvt = std::use_facet<mbCvt>(sjis);
	std::wstring_convert<mbCvt, wchar_t> sjisConverter(&cvt);
	std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> utf8Converter;

	std::string name = utf8Converter.to_bytes(sjisConverter.from_bytes(obj->GetName()));
	info.GetReturnValue().Set(UTF8(name.c_str()));
}

static void SetObjectName(const Local<String> property, Local<Value> value, const PropertyCallbackInfo<void>& info)
{
	Isolate* isolate = info.GetIsolate();
	Local<Object> self = info.Holder();
	MQObject obj = static_cast<MQObject>(self->GetInternalField(0).As<External>()->Value());
	String::Utf8Value str(value);

	std::locale sjis(".932", std::locale::ctype);
	typedef std::codecvt<wchar_t, char, std::mbstate_t> mbCvt;
	const mbCvt& cvt = std::use_facet<mbCvt>(sjis);
	std::wstring_convert<mbCvt, wchar_t> sjisConverter(&cvt);
	std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> utf8Converter;

	std::string name = sjisConverter.to_bytes(utf8Converter.from_bytes(*str));

	obj->SetName(name.c_str());
}


static void GetMaterialName(const Local<String> property, const PropertyCallbackInfo<Value>& info)
{
	Isolate* isolate = info.GetIsolate();
	Local<Object> self = info.Holder();
	MQMaterial obj = static_cast<MQMaterial>(self->GetInternalField(0).As<External>()->Value());

	info.GetReturnValue().Set(UTF8(obj->GetName().c_str()));
}

static void SetMaterialName(const Local<String> property, Local<Value> value, const PropertyCallbackInfo<void>& info)
{
	Isolate* isolate = info.GetIsolate();
	Local<Object> self = info.Holder();
	MQMaterial obj = static_cast<MQMaterial>(self->GetInternalField(0).As<External>()->Value());
	String::Utf8Value str(value);
	obj->SetName(*str);
}

static void GetVerts(const Local<Name> property, const PropertyCallbackInfo<Value>& info)
{
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

static void GetFaces(const Local<Name> property, const PropertyCallbackInfo<Value>& info)
{
	Isolate* isolate = info.GetIsolate();
	Local<Object> self = info.Holder();
	MQObject obj = static_cast<MQObject>(self->GetInternalField(0).As<External>()->Value());

	auto facest = ObjectTemplate::New(isolate);
	facest->SetIndexedPropertyHandler(GetFace, nullptr, nullptr, DeleteFace);
	facest->SetAccessor(UTF8("length"), GetFaceCount);
	facest->Set(UTF8("append"), FunctionTemplate::New(isolate, AddFace));
	Local<Object> faces = facest->NewInstance();
	faces->Set(UTF8("_obj"), self);
	faces->SetPrototype(Array::New(isolate));
	info.GetReturnValue().Set(faces);
}

static Handle<Object> MQMaterialWrap(Isolate* isolate, MQMaterial m) {
	auto objt = ObjectTemplate::New(isolate);
	objt->SetInternalFieldCount(1);
	objt->SetAccessor(UTF8("name"), GetMaterialName, SetMaterialName);

	Local<Object> obj = objt->NewInstance();
	obj->SetInternalField(0, External::New(isolate, m));
	return obj;
}


static void FreezeObject(const FunctionCallbackInfo<Value>& args)
{
	Isolate* isolate = args.GetIsolate();
	Local<Object> self = args.Holder();
	Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
	MQObject obj = static_cast<MQObject>(wrap->Value());
	int flag;
	if (args.kArgsLength > 0) {
		flag = args[0].As<Integer>()->Int32Value();
	} else {
		flag = MQOBJECT_FREEZE_ALL;
	}
	obj->Freeze(flag);
}

static void CompactObject(const FunctionCallbackInfo<Value>& args)
{
	Isolate* isolate = args.GetIsolate();
	Local<Object> self = args.Holder();
	Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
	MQObject obj = static_cast<MQObject>(wrap->Value());
	obj->Compact();
}

static void MergeObject(const FunctionCallbackInfo<Value>& args)
{
	Isolate* isolate = args.GetIsolate();
	Local<Object> self = args.Holder();
	Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
	MQObject obj = static_cast<MQObject>(wrap->Value());
	MQObject src = static_cast<MQObject>(args[0].As<Object>()->GetInternalField(0).As<External>()->Value());
	obj->Merge(src);
}

static Handle<Object> MQObjectWrap(Isolate* isolate, MQObject o);

static void CloneObject(const FunctionCallbackInfo<Value>& info)
{
	Isolate* isolate = info.GetIsolate();
	Local<Object> self = info.Holder();
	Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
	MQObject obj = static_cast<MQObject>(wrap->Value());

	MQObject newObject = obj->Clone();

	info.GetReturnValue().Set(MQObjectWrap(isolate, newObject));
}

static void CreateObject(const FunctionCallbackInfo<Value>& info)
{
	Isolate* isolate = info.GetIsolate();
	MQObject newObject = MQ_CreateObject();
	info.GetReturnValue().Set(MQObjectWrap(isolate, newObject));
}

static Handle<Object> MQObjectWrap(Isolate* isolate, MQObject o) {
	auto objt = ObjectTemplate::New(isolate);
	objt->SetInternalFieldCount(1);
	objt->SetAccessor(UTF8("name"), GetObjectName, SetObjectName);
	objt->Set(UTF8("clone"), FunctionTemplate::New(isolate, CloneObject));
	objt->Set(UTF8("freeze"), FunctionTemplate::New(isolate, FreezeObject));
	objt->Set(UTF8("compact"), FunctionTemplate::New(isolate, CompactObject));
	objt->Set(UTF8("merge"), FunctionTemplate::New(isolate, MergeObject));
	objt->SetLazyDataProperty(UTF8("verts"), GetVerts);
	objt->SetLazyDataProperty(UTF8("faces"), GetFaces);

	Local<Object> obj = objt->NewInstance();
	obj->SetInternalField(0, External::New(isolate, o));
	return obj;
}


static void AddObject(const FunctionCallbackInfo<Value>& args)
{
	Isolate* isolate = args.GetIsolate();
	MQDocument doc = static_cast<MQDocument>(args.Data().As<External>()->Value());
	Local<Object> _obj = args[0].As<Object>();
	Local<External> wrap = Local<External>::Cast(_obj->GetInternalField(0));
	MQObject obj = static_cast<MQObject>(wrap->Value());

	doc->AddObject(obj);
	args.GetReturnValue().SetUndefined();
}

static void CompactDocument(const FunctionCallbackInfo<Value>& args)
{
	Isolate* isolate = args.GetIsolate();
	MQDocument doc = static_cast<MQDocument>(args.Data().As<External>()->Value());

	doc->Compact();
	args.GetReturnValue().SetUndefined();
}

static Handle<Array> MQObjectsAsArray(Isolate* isolate, MQDocument doc) {
	Handle<Array> array = Array::New(isolate, doc->GetObjectCount());
	for (int i = 0; i < doc->GetObjectCount(); i++) {
		if (doc->GetObject(i) != nullptr) {
			array->Set(i, MQObjectWrap(isolate, doc->GetObject(i)));
		}
	}
	return array;
}

static void GetObjects(const Local<Name> property, const PropertyCallbackInfo<Value>& info) {
	Isolate* isolate = info.GetIsolate();
	Local<Object> self = info.Holder();
	MQDocument doc = static_cast<MQDocument>(self->GetInternalField(0).As<External>()->Value());
	info.GetReturnValue().Set(MQObjectsAsArray(isolate, doc));
}


static void GetMaterials(const Local<Name> property, const PropertyCallbackInfo<Value>& info) {
	Isolate* isolate = info.GetIsolate();
	Local<Object> self = info.Holder();
	MQDocument doc = static_cast<MQDocument>(self->GetInternalField(0).As<External>()->Value());
	Handle<Array> array = Array::New(isolate, doc->GetMaterialCount());
	for (int i = 0; i < doc->GetMaterialCount(); i++) {
		if (doc->GetMaterial(i) != nullptr) {
			array->Set(i, MQMaterialWrap(isolate, doc->GetMaterial(i)));
		}
	}
	info.GetReturnValue().Set(array);
}

Local<Object> NewDocumentObject(Isolate* isolate, MQDocument doc) {
	auto obj = ObjectTemplate::New(isolate);
	obj->SetInternalFieldCount(1);
	obj->Set(UTF8("compact"), FunctionTemplate::New(isolate, CompactDocument, External::New(isolate, doc)));
	obj->Set(UTF8("addObject"), FunctionTemplate::New(isolate, AddObject, External::New(isolate, doc)));
	obj->Set(UTF8("createObject"), FunctionTemplate::New(isolate, CreateObject));
	obj->SetLazyDataProperty(UTF8("objects"), GetObjects);
	obj->SetLazyDataProperty(UTF8("materials"), GetMaterials);

	Local<Object> lobj = obj->NewInstance();
	lobj->SetInternalField(0, External::New(isolate, doc));
	return lobj;
}
