﻿//---------------------------------------------------------------------------------------------------------------------
//  JavaScript plugin.
//       http://www.binzume.net
//---------------------------------------------------------------------------------------------------------------------

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <vector>
#include <fstream>
#include <string>
#include <sstream>

#include "MQBasePlugin.h"
#include "MQ3DLib.h"
#include "JSMacroWindow.h"

#include "libplatform/libplatform.h"
#include "v8.h"
using namespace v8;

HINSTANCE hInstance;



//---------------------------------------------------------------------------------------------------------------------
//  DllMain
//---------------------------------------------------------------------------------------------------------------------
BOOL APIENTRY DllMain( HANDLE hModule,
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
	hInstance = (HINSTANCE)hModule;
	return TRUE;
}


class JSMacroPlugin : public MQStationPlugin
{
public:
	JSMacroPlugin();
	JSMacroWindow *window;
	v8::Platform* v8platform;

	virtual void GetPlugInID(DWORD *Product, DWORD *ID);
	virtual const char *GetPlugInName(void);
	virtual const char *EnumString(void);
	virtual BOOL Initialize();
	virtual void Exit();
	virtual BOOL Activate(MQDocument doc, BOOL flag);
	virtual BOOL IsActivated(MQDocument doc);
	virtual void OnDraw(MQDocument doc, MQScene scene, int width, int height);
	virtual void OnUpdateObjectList(MQDocument doc);
	void AddMessage(const char *message) {
		if (window) window->AddMessage(message);
	}
	void ExecScript(MQDocument doc, const std::string &jsfile);
protected:
	virtual bool ExecuteCallback(MQDocument doc, void *option);
};


void log(const std::string s ...)
{
	JSMacroPlugin *plugin = static_cast<JSMacroPlugin*>(GetPluginClass());
	plugin->AddMessage(s.c_str());
	// std::ofstream fout("C:/tmp/jsmacro.log", std::ios::app);
	// fout << s << std::endl;
	// fout.close();
}


//---------------------------------------------------------------------------------------------------------------------
//  js stdout (debug)
//---------------------------------------------------------------------------------------------------------------------


static void WriteFile(const v8::FunctionCallbackInfo<v8::Value>& args) {
	std::ofstream &f = *(std::ofstream*)args.Data().As<External>()->Value();
	std::stringstream ss;
	bool first = true;
	for (int i = 0; i < args.Length(); i++) {
		v8::HandleScope handle_scope(args.GetIsolate());
		if (first) {
			first = false;
		}
		else {
			ss << " ";
		}
		v8::String::Utf8Value str(args[i]);
		ss << (*str ? *str : "[NOT_STRING]");
	}
	f << ss.str() << std::endl;
	log(ss.str().c_str());
	f.flush();
}

static Local<ObjectTemplate> NewProcessObject(Isolate* isolate) {
	auto obj = ObjectTemplate::New(isolate);


	Handle<External> selfRef = External::New(isolate, (void*)(new std::ofstream("C:/tmp/console.log.txt", std::ios::app)));
	auto fileObj = ObjectTemplate::New(isolate);
	fileObj->Set(String::NewFromUtf8(isolate, "write", NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(isolate, WriteFile, selfRef));

	obj->Set(String::NewFromUtf8(isolate, "stdout", NewStringType::kNormal).ToLocalChecked(), fileObj);
	obj->Set(String::NewFromUtf8(isolate, "stderr", NewStringType::kNormal).ToLocalChecked(), fileObj);
	return obj;
}

Local<Object> NewDocumentObject(Isolate* isolate, MQDocument doc);

static Local<Context> CreateContext(Isolate *isolate, MQDocument doc) {
	v8::Local<v8::ObjectTemplate> global = v8::ObjectTemplate::New(isolate);

	global->Set(v8::String::NewFromUtf8(isolate, "process", v8::NewStringType::kNormal).ToLocalChecked(),
		NewProcessObject(isolate));

	Local<Context> context = Context::New(isolate, nullptr, global);
	Context::Scope context_scope(context);

	context->Global()->Set(
		v8::String::NewFromUtf8(isolate, "document", v8::NewStringType::kNormal)
		.ToLocalChecked(),
		NewDocumentObject(isolate, doc));


	return context;
}

//---------------------------------------------------------------------------------------------------------------------
//    プラグインのベースクラスを返す
//---------------------------------------------------------------------------------------------------------------------
MQBasePlugin *GetPluginClass()
{
	static JSMacroPlugin plugin;
	return &plugin;
}

//---------------------------------------------------------------------------------------------------------------------
//    コンストラクタ
//---------------------------------------------------------------------------------------------------------------------
JSMacroPlugin::JSMacroPlugin()
{

	V8::InitializeICUDefaultLocation("C:/tmp/test");
	// V8::InitializeExternalStartupData("C:/tmp/test");

	v8platform = platform::CreateDefaultPlatform();
	V8::InitializePlatform(v8platform);
	V8::Initialize();
}


//---------------------------------------------------------------------------------------------------------------------
//    プラグインIDを返す。
//---------------------------------------------------------------------------------------------------------------------
void JSMacroPlugin::GetPlugInID(DWORD *Product, DWORD *ID)
{
	*Product = 0x7a6e6962;
	*ID      = 0x7a6e6963;
}

//---------------------------------------------------------------------------------------------------------------------
//    プラグイン名を返す。
//---------------------------------------------------------------------------------------------------------------------
const char *JSMacroPlugin::GetPlugInName(void)
{
	// プラグイン名
	return "JavaScript macro runner";
}

//---------------------------------------------------------------------------------------------------------------------
//    ボタンに表示される文字列を返す。
//---------------------------------------------------------------------------------------------------------------------
const char *JSMacroPlugin::EnumString(void)
{
	return "JS Macro";
}

//---------------------------------------------------------------------------------------------------------------------
//    アプリケーションの初期化
//---------------------------------------------------------------------------------------------------------------------
BOOL JSMacroPlugin::Initialize()
{
	if (window == nullptr) {
		MQWindowBase mainwnd = MQWindow::GetMainWindow();
		window = new JSMacroWindow(mainwnd);
	}
	window->AddMessage("Initialized.");
	return TRUE;
}

//---------------------------------------------------------------------------------------------------------------------
//    アプリケーションの終了
//---------------------------------------------------------------------------------------------------------------------
void JSMacroPlugin::Exit()
{
	delete v8platform;
}

//---------------------------------------------------------------------------------------------------------------------
//    表示・非表示切り替え要求
//---------------------------------------------------------------------------------------------------------------------
BOOL JSMacroPlugin::Activate(MQDocument doc, BOOL flag)
{
	MQ_RefreshView(NULL);

	window->SetVisible(flag != 0);

	if (flag != 0) {
		ExecScript(doc, "C:/tmp/test.js");
	}

	MQ_RefreshView(nullptr);

	return flag;
}

//---------------------------------------------------------------------------------------------------------------------
// 表示・非表示を返す
//---------------------------------------------------------------------------------------------------------------------
BOOL JSMacroPlugin::IsActivated(MQDocument doc)
{
	return window->GetVisible();
}

//---------------------------------------------------------------------------------------------------------------------
// 描画
//---------------------------------------------------------------------------------------------------------------------
void JSMacroPlugin::OnDraw(MQDocument doc, MQScene scene, int width, int height)
{
}

void JSMacroPlugin::OnUpdateObjectList(MQDocument doc)
{
}

bool JSMacroPlugin::ExecuteCallback(MQDocument doc, void *option)
{
	return false;
}


void JSMacroPlugin::ExecScript(MQDocument doc, const std::string &fname) {

	Isolate::CreateParams create_params;
	create_params.array_buffer_allocator =
		v8::ArrayBuffer::Allocator::NewDefaultAllocator();
	Isolate* isolate = Isolate::New(create_params);
	{
		Isolate::Scope isolate_scope(isolate);

		// Create a stack-allocated handle scope.
		HandleScope handle_scope(isolate);

		// Create a new context.
		Local<Context> context = CreateContext(isolate, doc);

		// Enter the context for compiling and running the hello world script.
		Context::Scope context_scope(context);

		log("v8 core.js...");
		{
			TCHAR path[MAX_PATH];
			GetModuleFileName(hInstance, path, MAX_PATH);
			std::ifstream jsfile(std::string(path) + ".core.js");
			std::stringstream buffer;
			buffer << jsfile.rdbuf();
			Local<String> source =
				String::NewFromUtf8(isolate, buffer.str().c_str(),
					NewStringType::kNormal).ToLocalChecked();

			Local<Script> script = Script::Compile(context, source).ToLocalChecked();
			script->Run(context).ToLocalChecked();
		}

		// Create a string containing the JavaScript source code.
		std::ifstream jsfile(fname);
		std::stringstream buffer;
		buffer << jsfile.rdbuf();
		Local<String> source =
			String::NewFromUtf8(isolate, buffer.str().c_str(), NewStringType::kNormal).ToLocalChecked();

		// Compile the source code.
		Local<Script> script = Script::Compile(context, source).ToLocalChecked();

		log(std::string("Run: ") + fname);
		// Run the script to get the result.
		TryCatch trycatch;
		auto ret = script->Run(context);
		if (!ret.IsEmpty()) {
			Local<Value> result = ret.ToLocalChecked();
			String::Utf8Value utf8(result);
			log(*utf8);
			log("ok.");
		}
		else {
			log("ERROR:");
			Handle<Value> exception = trycatch.Exception();
			String::Utf8Value exception_str(exception);
			log(*exception_str);
		}
	}

	// Dispose the isolate and tear down V8.
	isolate->Dispose();
	//V8::Dispose();
	//V8::ShutdownPlatform();
	delete create_params.array_buffer_allocator;
}
