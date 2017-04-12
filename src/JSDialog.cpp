
#define WIN32_LEAN_AND_MEAN
#include <vector>
#include <sstream>
#include <codecvt>
#include "libplatform/libplatform.h"
#include "v8.h"
#include <windows.h>
#include "MQWidget.h"


//---------------------------------------------------------------------------------------------------------------------
// Utils
//---------------------------------------------------------------------------------------------------------------------
#define UTF8(s) v8::String::NewFromUtf8(isolate, s, v8::NewStringType::kNormal).ToLocalChecked()


//---------------------------------------------------------------------------------------------------------------------
// Dialog
//---------------------------------------------------------------------------------------------------------------------
using namespace v8;

Local<ObjectTemplate> NewFile(Isolate* isolate, const std::string &path, bool writable);


static void FileDialog(const FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = args.GetIsolate();

	if (!args[0]->IsString()) {
		isolate->ThrowException(Exception::TypeError(UTF8("invalid args")));
		return;
	}
	String::Utf8Value path(args[0].As<String>());
	bool save = args[1]->BooleanValue();

	std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
	std::wstring filter = converter.from_bytes(*path);

	auto dialog = new MQOpenFileDialog(MQWindow::GetMainWindow());
	dialog->SetFileMustExist(!save);
	dialog->AddFilter(filter);
	if (dialog->Execute()) {
		std::wstring path = dialog->GetFileName();
		args.GetReturnValue().Set(NewFile(isolate, converter.to_bytes(path), save)->NewInstance());
	} else {
		args.GetReturnValue().SetNull();
	}
}

static void FolderDialog(const FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = args.GetIsolate();

	if (!args[0]->IsString()) {
		isolate->ThrowException(Exception::TypeError(UTF8("invalid args")));
		return;
	}
	String::Utf8Value path(args[0].As<String>());

	std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
	std::wstring filter = converter.from_bytes(*path);

	auto dialog = new MQFolderDialog(MQWindow::GetMainWindow());
	// dialog->SetFileMustExist(true);
	if (dialog->Execute()) {
		std::wstring path = dialog->GetFolder();
		args.GetReturnValue().Set(UTF8(converter.to_bytes(path).c_str()));
	} else {
		args.GetReturnValue().SetNull();
	}
	delete dialog;
}

static void AlertDialog(const FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = args.GetIsolate();

	if (!args[0]->IsString()) {
		isolate->ThrowException(Exception::TypeError(UTF8("invalid args")));
		return;
	}
	String::Utf8Value message(args[0].As<String>());

	std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
	std::wstring wmessage = converter.from_bytes(*message);
	std::wstring wtitle = L"";
	MQDialog::MessageInformationBox(MQWindow::GetMainWindow(), wmessage, wtitle);
	args.GetReturnValue().SetNull();
}

Local<ObjectTemplate> DialogTemplate(Isolate* isolate) {
	auto fs = ObjectTemplate::New(isolate);
	fs->Set(UTF8("fileDialog"), FunctionTemplate::New(isolate, FileDialog));
	fs->Set(UTF8("folderDialog"), FunctionTemplate::New(isolate, FolderDialog));
	fs->Set(UTF8("alertDialog"), FunctionTemplate::New(isolate, AlertDialog));
	return fs;
}
