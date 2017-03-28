

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <codecvt>
#include "MQBasePlugin.h"
#include "MQ3DLib.h"

#include "libplatform/libplatform.h"
#include "v8.h"

#define UTF8(s) v8::String::NewFromUtf8(isolate, s, v8::NewStringType::kNormal).ToLocalChecked()
void debug_log(const std::string s);

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

static Handle<Object> MQObjectWrap(Isolate* isolate, MQObject o) {
	Local<Value> args[] = { External::New(isolate, o) };
	return isolate->GetCurrentContext()->Global()
		->Get(UTF8("MQObject")).As<Function>()->CallAsConstructor(1, args).As<Object>();
}

static void CloneObject(const FunctionCallbackInfo<Value>& info)
{
	Isolate* isolate = info.GetIsolate();
	Local<Object> self = info.Holder();
	Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
	MQObject obj = static_cast<MQObject>(wrap->Value());

	MQObject newObject = obj->Clone();

	info.GetReturnValue().Set(MQObjectWrap(isolate, newObject));
}

static void InitMQObjectTemplate(Isolate* isolate, Local<ObjectTemplate> objt) {
	objt->SetInternalFieldCount(1);
	objt->SetAccessor(UTF8("name"), GetObjectName, SetObjectName);
	objt->Set(UTF8("clone"), FunctionTemplate::New(isolate, CloneObject));
	objt->Set(UTF8("freeze"), FunctionTemplate::New(isolate, FreezeObject));
	objt->Set(UTF8("compact"), FunctionTemplate::New(isolate, CompactObject));
	objt->Set(UTF8("merge"), FunctionTemplate::New(isolate, MergeObject));
	objt->SetLazyDataProperty(UTF8("verts"), GetVerts);
	objt->SetLazyDataProperty(UTF8("faces"), GetFaces);
}

//#include <sstream>
void MQObjectDispose(const WeakCallbackInfo<MQCObject>& data) {
	//std::stringstream ss;
	//ss << (size_t)data.GetParameter() << " " << data.GetParameter()->GetUniqueID();
	//debug_log(ss.str());
	if (data.GetParameter()->GetUniqueID() == 0) {
		data.GetParameter()->DeleteThis();
	}
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
			std::locale sjis(".932", std::locale::ctype);
			typedef std::codecvt<wchar_t, char, std::mbstate_t> mbCvt;
			const mbCvt& cvt = std::use_facet<mbCvt>(sjis);
			std::wstring_convert<mbCvt, wchar_t> sjisConverter(&cvt);
			std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> utf8Converter;

			String::Utf8Value str(args[0]);
			std::string name = sjisConverter.to_bytes(utf8Converter.from_bytes(*str));
			obj->SetName(name.c_str());
		} else {
			MQDocument doc = static_cast<MQDocument>(args.Data().As<External>()->Value());
			char name[256];
			doc->GetUnusedObjectName(name, sizeof(name));
			obj->SetName(name);
		}
	}

	Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
	MQObject obj = static_cast<MQObject>(wrap->Value());
	Persistent<Object> *holder = new Persistent<Object>(isolate, self); // FIXME
	holder->SetWeak<MQCObject>(obj, MQObjectDispose, WeakCallbackType::kParameter);
	holder->MarkIndependent();
	// isolate->LowMemoryNotification();

	args.GetReturnValue().Set(self);
}

static void AddObject(const FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = args.GetIsolate();
	MQDocument doc = static_cast<MQDocument>(args.Data().As<External>()->Value());
	Local<Object> _obj = args[0].As<Object>();
	Local<External> wrap = Local<External>::Cast(_obj->GetInternalField(0));
	MQObject obj = static_cast<MQObject>(wrap->Value());

	doc->AddObject(obj);
	args.GetReturnValue().SetUndefined();
}

static void CompactDocument(const FunctionCallbackInfo<Value>& args) {
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
	array->Set(UTF8("append"), Function::New(isolate, AddObject, External::New(isolate, doc)));
	return array;
}

static void GetObjects(const Local<Name> property, const PropertyCallbackInfo<Value>& info) {
	Isolate* isolate = info.GetIsolate();
	Local<Object> self = info.Holder();
	MQDocument doc = static_cast<MQDocument>(self->GetInternalField(0).As<External>()->Value());
	info.GetReturnValue().Set(MQObjectsAsArray(isolate, doc));
}

static void GetMaterialColor(const Local<String> property, const PropertyCallbackInfo<Value>& info)
{
	Isolate* isolate = info.GetIsolate();
	Local<Object> self = info.Holder();
	MQMaterial m = static_cast<MQMaterial>(self->GetInternalField(0).As<External>()->Value());
	MQColor color = m->GetColor();
	float alpha = m->GetAlpha();

	auto c = Object::New(isolate);
	c->Set(UTF8("r"), Number::New(isolate, color.r));
	c->Set(UTF8("g"), Number::New(isolate, color.g));
	c->Set(UTF8("b"), Number::New(isolate, color.b));
	c->Set(UTF8("a"), Number::New(isolate, alpha));

	info.GetReturnValue().Set(c);
}

static void SetMaterialColor(const Local<String> property, Local<Value> value, const PropertyCallbackInfo<void>& info)
{
	Isolate* isolate = info.GetIsolate();
	Local<Object> self = info.Holder();
	MQMaterial m = static_cast<MQMaterial>(self->GetInternalField(0).As<External>()->Value());
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

static Handle<Object> MQMaterialWrap(Isolate* isolate, MQMaterial m) {
	auto objt = ObjectTemplate::New(isolate);
	objt->SetInternalFieldCount(1);
	objt->SetAccessor(UTF8("name"), GetMaterialName, SetMaterialName);
	objt->SetAccessor(UTF8("color"), GetMaterialColor, SetMaterialColor);

	Local<Object> obj = objt->NewInstance();
	obj->SetInternalField(0, External::New(isolate, m));
	return obj;
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

static Local<Object> NewDocumentObject(Isolate* isolate, MQDocument doc) {
	auto obj = ObjectTemplate::New(isolate);
	obj->SetInternalFieldCount(1);
	obj->Set(UTF8("compact"), FunctionTemplate::New(isolate, CompactDocument, External::New(isolate, doc)));
	obj->SetLazyDataProperty(UTF8("objects"), GetObjects);
	obj->SetLazyDataProperty(UTF8("materials"), GetMaterials);

	Local<Object> lobj = obj->NewInstance();
	lobj->SetInternalField(0, External::New(isolate, doc));
	return lobj;
}

void InstallMQDocument(Local<Object> global, Isolate* isolate, MQDocument doc) {
	auto objcectConstructor = FunctionTemplate::New(isolate, MQObjectConstructor, External::New(isolate, doc));
	objcectConstructor->SetClassName(UTF8("MQObject"));
	InitMQObjectTemplate(isolate, objcectConstructor->InstanceTemplate());

	global->Set(UTF8("MQObject"), objcectConstructor->GetFunction());
	global->Set(UTF8("document"), NewDocumentObject(isolate, doc));
}
