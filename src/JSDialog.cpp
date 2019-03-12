
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

class Dialog : public MQDialog {
public:
	uint64_t button;
	std::vector<std::pair<Local<Object>, MQEdit*>> itemholders;
	std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
	Isolate* isolate;
	MQWindow mainWindow;

	Dialog(Isolate* i) : mainWindow(MQWindow::GetMainWindow()), MQDialog(mainWindow), button(-1), isolate(i) {
	}

	void AddItems(Local<Array> items, MQWidgetBase *parent) {
		for (uint32_t i = 0; i < items->Length(); i++) {
			Local<Value> item = items->Get(i);
			auto context = isolate->GetCurrentContext();
			if (item->IsString()) {
				std::wstring label = converter.from_bytes(*String::Utf8Value(isolate, item.As<String>()));
				auto *w = this->CreateLabel(this, label);
				parent->AddChild(w);
			} else if (item->IsObject()) {
				Local<Value> type = item.As<Object>()->Get(UTF8("type"));
				Local<Value> value = item.As<Object>()->Get(UTF8("value"));
				// Local<String> id = item.As<Object>()->Get(UTF8("id")).As<String>();
				if (type->Equals(context, UTF8("text")).ToChecked()) {
					std::wstring v = converter.from_bytes(*String::Utf8Value(isolate, value.As<String>()));
					auto *w = this->CreateEdit(this, v);
					parent->AddChild(w);
					itemholders.push_back(std::make_pair(item.As<Object>(), w));
				} else if (type->Equals(context, UTF8("number")).ToChecked()) {
					std::wstring v = converter.from_bytes(*String::Utf8Value(isolate, value.As<String>()));
					auto *w = this->CreateEdit(this, v);
					w->SetNumeric(MQEdit::NUMERIC_DOUBLE);
					parent->AddChild(w);
					itemholders.push_back(std::make_pair(item.As<Object>(), w));
				} else if (type->Equals(context, UTF8("button")).ToChecked()) {
					std::wstring v = converter.from_bytes(*String::Utf8Value(isolate, value.As<String>()));
					auto *w = this->CreateButton(this, v);
					parent->AddChild(w);
				} else if (type->Equals(context, UTF8("hframe")).ToChecked()) {
					auto *w = this->CreateHorizontalFrame(this);
					parent->AddChild(w);
					Local<Array> items = item.As<Object>()->Get(UTF8("items")).As<Array>();
					AddItems(items, w);
				} else if (type->Equals(context, UTF8("vframe")).ToChecked()) {
					auto *w = this->CreateVerticalFrame(this);
					parent->AddChild(w);
					Local<Array> items = item.As<Object>()->Get(UTF8("items")).As<Array>();
					AddItems(items, w);
				}
			}
		}
	}

	BOOL OnClick(MQWidgetBase *sender, MQDocument doc) {
		button = sender->GetTag();
		Close(MQDialog::DIALOG_OK);
		return TRUE;
	}
};

static void ModalDialog(const FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = args.GetIsolate();

	auto dialog = new Dialog(isolate);
	Local<Object> params = args[0].As<Object>();
	Local<Array> buttons = args[1].As<Array>();

	Local<String> title = params->Get(UTF8("title")).As<String>();
	Local<Array> items = params->Get(UTF8("items")).As<Array>();

	dialog->SetTitle(dialog->converter.from_bytes(*String::Utf8Value(isolate, title)));
	dialog->AddItems(items, dialog);


	int buttonCount = buttons->Length();
	if (buttonCount > 0) {
		auto *frame = dialog->CreateHorizontalFrame(dialog);
		dialog->AddChild(frame);
		for (int i = 0; i < buttonCount; i++) {
			Local<String> name = buttons->Get(i).As<String>();
			MQButton *b = dialog->CreateButton(frame, dialog->converter.from_bytes(*String::Utf8Value(isolate, name)));
			b->AddClickEvent(dialog, &Dialog::OnClick);
			b->SetTag(i);
			frame->AddChild(b);
		}
	}

	if (dialog->Execute()) {
		Local<Object> result = Object::New(isolate);
		Local<Object> values = Object::New(isolate);
		for (const auto &item : dialog->itemholders) {
			Local<Value> value = UTF8(dialog->converter.to_bytes(item.second->GetText()).c_str());
			item.first->Set(UTF8("value"), value);
			if (!item.first->Get(UTF8("id"))->IsNullOrUndefined()) {
				values->Set(item.first->Get(UTF8("id")), value);
			}
		}
		result->Set(UTF8("values"), values);
		result->Set(UTF8("button"), Number::New(isolate, dialog->button));
		args.GetReturnValue().Set(result);
	} else {
		args.GetReturnValue().SetNull();
	}
	delete dialog;
}


static void FileDialog(const FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = args.GetIsolate();

	if (!args[0]->IsString()) {
		isolate->ThrowException(Exception::TypeError(UTF8("invalid args")));
		return;
	}
	String::Utf8Value path(isolate, args[0].As<String>());
	bool save = args[1]->BooleanValue(isolate);

	std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
	std::wstring filter = converter.from_bytes(*path);

	if (save) {
		auto window = MQWindow::GetMainWindow();
		auto dialog = new MQSaveFileDialog(window);
		dialog->AddFilter(filter);
		if (dialog->Execute()) {
			std::wstring path = dialog->GetFileName();
			args.GetReturnValue().Set(NewFile(isolate, converter.to_bytes(path), save)->NewInstance());
		} else {
			args.GetReturnValue().SetNull();
		}
	} else {
		auto window = MQWindow::GetMainWindow();
		auto dialog = new MQOpenFileDialog(window);
		dialog->AddFilter(filter);
		if (dialog->Execute()) {
			std::wstring path = dialog->GetFileName();
			args.GetReturnValue().Set(NewFile(isolate, converter.to_bytes(path), save)->NewInstance());
		} else {
			args.GetReturnValue().SetNull();
		}
	}
}

static void FolderDialog(const FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = args.GetIsolate();

	if (!args[0]->IsString()) {
		isolate->ThrowException(Exception::TypeError(UTF8("invalid args")));
		return;
	}
	String::Utf8Value path(isolate, args[0].As<String>());

	std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
	std::wstring filter = converter.from_bytes(*path);

	auto window = MQWindow::GetMainWindow();
	auto dialog = new MQFolderDialog(window);
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
	String::Utf8Value message(isolate, args[0].As<String>());

	std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
	std::wstring wmessage = converter.from_bytes(*message);
	std::wstring wtitle = L"";
	MQWindow window = MQWindow::GetMainWindow();
	MQDialog::MessageInformationBox(window, wmessage, wtitle);
	args.GetReturnValue().SetNull();
}

Local<ObjectTemplate> DialogTemplate(Isolate* isolate) {
	auto fs = ObjectTemplate::New(isolate);
	fs->Set(UTF8("modalDialog"), FunctionTemplate::New(isolate, ModalDialog));
	fs->Set(UTF8("fileDialog"), FunctionTemplate::New(isolate, FileDialog));
	fs->Set(UTF8("folderDialog"), FunctionTemplate::New(isolate, FolderDialog));
	fs->Set(UTF8("alertDialog"), FunctionTemplate::New(isolate, AlertDialog));
	return fs;
}
