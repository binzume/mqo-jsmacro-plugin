
#define WIN32_LEAN_AND_MEAN
#include <vector>
#include <sstream>
#include <codecvt>
#include "libplatform/libplatform.h"
#include "v8.h"


//---------------------------------------------------------------------------------------------------------------------
// Utils
//---------------------------------------------------------------------------------------------------------------------
#define UTF8(s) v8::String::NewFromUtf8(isolate, s, v8::NewStringType::kNormal).ToLocalChecked()
void debug_log(const std::string s, int tag = 1);


//---------------------------------------------------------------------------------------------------------------------
// File System
//---------------------------------------------------------------------------------------------------------------------
using namespace v8;

static void ReadFileSync(const FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = args.GetIsolate();

	if (!args.Holder()->Get(UTF8("path"))->IsString()) {
		isolate->ThrowException(Exception::TypeError(UTF8("invalid path")));
		return;
	}
	String::Utf8Value path(args.Holder()->Get(UTF8("path")).As<String>());
	std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;

	std::ifstream jsfile(converter.from_bytes(*path));
	std::stringstream buffer;
	buffer << jsfile.rdbuf();
	jsfile.close();
	if (jsfile.fail()) {
		isolate->ThrowException(Exception::Error(UTF8("read failed.")));
		return;
	}
	args.GetReturnValue().Set(UTF8(buffer.str().c_str()));
}

static void WriteFileSync(const FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = args.GetIsolate();

	if (!args[0]->IsString()) {
		isolate->ThrowException(Exception::TypeError(UTF8("invalid data")));
		return;
	}

	if (!args.Holder()->Get(UTF8("path"))->IsString()) {
		isolate->ThrowException(Exception::TypeError(UTF8("invalid path")));
		return;
	}
	String::Utf8Value path(args.Holder()->Get(UTF8("path")).As<String>());
	String::Utf8Value data(args[0].As<String>());

	int mode = std::ofstream::binary;
	if (args[1].As<Object>()->Get(UTF8("flag"))->Equals(UTF8("a"))) {
		mode |= std::ofstream::app;
	}

	std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
	std::ofstream jsfile(converter.from_bytes(*path), mode);
	jsfile << *data;
	jsfile.close();
	if (jsfile.fail()) {
		isolate->ThrowException(Exception::Error(UTF8("write failed.")));
		return;
	}
	args.GetReturnValue().Set(data.length());
}

Local<ObjectTemplate> NewFile(Isolate* isolate, const std::string &path, bool writable) {
	auto file = ObjectTemplate::New(isolate);
	file->Set(UTF8("path"), UTF8(path.c_str()), PropertyAttribute::ReadOnly);
	file->Set(UTF8("read"), FunctionTemplate::New(isolate, ReadFileSync));
	if (writable) {
		file->Set(UTF8("write"), FunctionTemplate::New(isolate, WriteFileSync));
	}
	return file;
}

static void ReadFile(const FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = args.GetIsolate();

	if (!args[0]->IsString()) {
		isolate->ThrowException(Exception::TypeError(UTF8("invalid path")));
		return;
	}
	String::Utf8Value path(args[0].As<String>());
	std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;

	std::ifstream jsfile(converter.from_bytes(*path));
	std::stringstream buffer;
	buffer << jsfile.rdbuf();
	jsfile.close();
	if (jsfile.fail()) {
		isolate->ThrowException(Exception::Error(UTF8("read failed.")));
		return;
	}
	args.GetReturnValue().Set(UTF8(buffer.str().c_str()));
}

static void WriteFile(const FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = args.GetIsolate();

	if (!args[0]->IsString()) {
		isolate->ThrowException(Exception::TypeError(UTF8("invalid path")));
		return;
	}
	if (!args[1]->IsString()) {
		isolate->ThrowException(Exception::TypeError(UTF8("invalid data")));
		return;
	}
	String::Utf8Value path(args[0].As<String>());
	String::Utf8Value data(args[1].As<String>());

	int mode = std::ofstream::binary;
	if (args[2].As<Object>()->Get(UTF8("flag"))->Equals(UTF8("a"))) {
		mode |= std::ofstream::app;
	}

	std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
	std::ofstream jsfile(converter.from_bytes(*path), mode);
	jsfile << *data;
	jsfile.close();
	if (jsfile.fail()) {
		isolate->ThrowException(Exception::Error(UTF8("write failed.")));
		return;
	}
	args.GetReturnValue().Set(data.length());
}


static void OpenFile(const FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = args.GetIsolate();

	if (!args[0]->IsString()) {
		isolate->ThrowException(Exception::TypeError(UTF8("invalid path")));
		return;
	}
	String::Utf8Value path(args[0].As<String>());

	bool write = false;
	if (args[2].As<Object>()->Get(UTF8("flag"))->Equals(UTF8("w"))) {
		write = true;
	}

	args.GetReturnValue().Set(NewFile(isolate, *path, write)->NewInstance());
}

Local<ObjectTemplate> FileSystemTemplate(Isolate* isolate) {
	auto fs = ObjectTemplate::New(isolate);
	fs->Set(UTF8("open"), FunctionTemplate::New(isolate, OpenFile));
	fs->Set(UTF8("readFile"), FunctionTemplate::New(isolate, ReadFile));
	fs->Set(UTF8("writeFile"), FunctionTemplate::New(isolate, WriteFile));
	return fs;
}
