//---------------------------------------------------------------------------------------------------------------------
//  JavaScript plugin.
//       http://www.binzume.net
//---------------------------------------------------------------------------------------------------------------------

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <vector>
#include <fstream>
#include <string>
#include <sstream>
#include <codecvt>

#include "MQBasePlugin.h"
#include "MQ3DLib.h"
#include "JSMacroWindow.h"

#include "libplatform/libplatform.h"
#include "v8.h"

#define UTF8(s) v8::String::NewFromUtf8(isolate, s, v8::NewStringType::kNormal).ToLocalChecked()

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

static Local<Context> CreateContext(Isolate *isolate, MQDocument doc);
class JsContext {
	Isolate *isolate;
	Isolate::Scope isolate_scope;
	HandleScope handle_scope;
	Local<Context> context;
	Context::Scope context_scope;
public:
	v8::UniquePersistent<Function> nextTickFun;
	UINT_PTR tickTimerId;
	JsContext(Isolate *_isolate, MQDocument doc) : isolate(_isolate),  isolate_scope(_isolate), handle_scope(_isolate),
		context(CreateContext(_isolate, doc)), context_scope(context) {}
	v8::MaybeLocal<Value> ExecScript(const std::string &code, const std::string &maybepath = "");
	~JsContext(){
		if (tickTimerId) {
			KillTimer(NULL, tickTimerId);
		}
	}
};

const char* SUB_COMMAND[PRESET_SCRIPT_COUNT+1] = {"EXEC", "EXEC_P0", "EXEC_P1", "EXEC_P2", "EXEC_P3", "EXEC_P4"};
const wchar_t* SUB_COMMAND_STR[PRESET_SCRIPT_COUNT + 1] = {
	L"Run", L"Run 1", L"Run 2", L"Run 3", L"Run 4", L"Run 5"
};

class JSMacroPlugin : public MQStationPlugin
{
public:
	std::string currentScriptPath;
	JSMacroWindow *window;
	JSMacroPlugin();

	virtual void GetPlugInID(DWORD *Product, DWORD *ID);
	virtual const char *GetPlugInName(void);
	virtual const char *EnumString(void);
	virtual BOOL Initialize();
	virtual void Exit();
	virtual BOOL Activate(MQDocument doc, BOOL flag);
	virtual BOOL IsActivated(MQDocument doc);
	virtual void OnEndDocument(MQDocument doc) {
		if (jsContext) {
			delete jsContext;
			jsContext = nullptr;
		}
	};
	virtual void OnDraw(MQDocument doc, MQScene scene, int width, int height);
	virtual void OnUpdateObjectList(MQDocument doc);
	virtual const char *EnumSubCommand(int index) {
		if (index <= PRESET_SCRIPT_COUNT) {
			return SUB_COMMAND[index];
		}
		return nullptr;
	};
	virtual const wchar_t *GetSubCommandString(int index) { return SUB_COMMAND_STR[index]; };
	virtual BOOL OnSubCommand(MQDocument doc, int index) { window->Execute(doc, index - 1); return TRUE; };
	void AddMessage(const std::string &message, int tag = 0) {
		if (window) window->AddMessage(message, tag);
	}
	void ExecScript(MQDocument doc, const std::string &jsfile);
	void ExecScriptString(MQDocument doc, const std::string &code);
	v8::MaybeLocal<Value> ExecScriptCurrentContext(const std::string &code, const std::string &maybepath = "");
	static void SetNextTick(const FunctionCallbackInfo<Value>& args);
	static VOID CALLBACK TickTimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
	JsContext* GetJsContext(MQDocument doc);
	std::string GetScriptDir() const;
protected:
	v8::Platform* platform;
	v8::Isolate* isolate;
	v8::Isolate::CreateParams create_params;
	JsContext *jsContext;

	virtual bool ExecuteCallback(MQDocument doc, void *option);
};

class Callbacks : public WindowCallback {
	JSMacroPlugin *plugin;
public:
	Callbacks(JSMacroPlugin *p) : plugin(p) {}
	void ExecuteFile(const std::string &path, MQDocument doc) {
		plugin->ExecScript(doc, path);
		MQ_RefreshView(nullptr);
	}
	void ExecuteString(const std::string &code, MQDocument doc) {
		plugin->ExecScriptString(doc, code);
		MQ_RefreshView(nullptr);
	}
	void OnCloseWindow(MQDocument doc) {
		plugin->WindowClose();
	}
};

void debug_log(const std::string s, int tag = 1) {
	JSMacroPlugin *plugin = static_cast<JSMacroPlugin*>(GetPluginClass());
	plugin->AddMessage(s, tag);
	// std::ofstream fout("C:/tmp/jsmacro.log", std::ios::app);
	// fout << s << std::endl;
	// fout.close();
}

//---------------------------------------------------------------------------------------------------------------------
//  js stdout (debug)
//---------------------------------------------------------------------------------------------------------------------

static void WriteFile(const v8::FunctionCallbackInfo<v8::Value>& args) {
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

	JSMacroPlugin *plugin = static_cast<JSMacroPlugin*>(GetPluginClass());
	plugin->AddMessage(ss.str(), 0);
}

static void LoadScript(const FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = args.GetIsolate();
	if (args.kArgsLength >= 1) {
		JSMacroPlugin *plugin = static_cast<JSMacroPlugin*>(GetPluginClass());
		String::Utf8Value path(args[0].As<String>());
		debug_log("load " + plugin->GetScriptDir() + *path);
		std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
		std::ifstream jsfile(converter.from_bytes(plugin->GetScriptDir() + *path));
		std::stringstream buffer;
		buffer << jsfile.rdbuf();
		jsfile.close();
		if (jsfile.fail()) {
			debug_log(std::string("Read error: ") + *path, 2);
		}
		auto ret = plugin->ExecScriptCurrentContext(buffer.str(), *path);
		if (!ret.IsEmpty()) {
			args.GetReturnValue().Set(ret.ToLocalChecked());
			return;
		}
	}
	args.GetReturnValue().SetUndefined();
}

static void ExecScriptString(const FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = args.GetIsolate();
	if (args.kArgsLength >= 1) {
		JSMacroPlugin *plugin = static_cast<JSMacroPlugin*>(GetPluginClass());
		String::Utf8Value code(args[0].As<String>());
		String::Utf8Value path(args[1].As<String>());

		auto ret = plugin->ExecScriptCurrentContext(*code, *path);
		if (!ret.IsEmpty()) {
			args.GetReturnValue().Set(ret.ToLocalChecked());
			return;
		}
	}
	args.GetReturnValue().SetUndefined();
}

static void ShowWindow(const FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = args.GetIsolate();
	JSMacroPlugin *plugin = static_cast<JSMacroPlugin*>(GetPluginClass());
	if (args.kArgsLength >= 1) {
		plugin->window->SetVisible(args[0].As<Boolean>()->BooleanValue());
	} else {
		plugin->window->SetVisible(true);
	}
	args.GetReturnValue().SetUndefined();
}

static void ScriptDir(const FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = args.GetIsolate();
	JSMacroPlugin *plugin = static_cast<JSMacroPlugin*>(GetPluginClass());
	args.GetReturnValue().Set(UTF8(plugin->GetScriptDir().c_str()));
}

VOID CALLBACK JSMacroPlugin::TickTimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime) {
	JSMacroPlugin *plugin = static_cast<JSMacroPlugin*>(GetPluginClass());
	KillTimer(hwnd, idEvent);
	if (plugin->jsContext && plugin->jsContext->tickTimerId == idEvent) {
		plugin->jsContext->tickTimerId = 0;
		plugin->BeginCallback(nullptr);
	}
}

void JSMacroPlugin::SetNextTick(const FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = args.GetIsolate();
	JSMacroPlugin *plugin = static_cast<JSMacroPlugin*>(GetPluginClass());
	plugin->window->SetVisible(args[0].As<Boolean>()->BooleanValue());
	plugin->jsContext->nextTickFun.Reset(isolate, args[0].As<Function>());
	uint32_t timerMs = 10;
	if (args.kArgsLength > 1) {
		timerMs = args[1].As<Integer>()->Uint32Value();
	}
	if (plugin->jsContext->tickTimerId) {
		KillTimer(NULL, plugin->jsContext->tickTimerId);
	}
	plugin->jsContext->tickTimerId = SetTimer(NULL, 1234, timerMs, JSMacroPlugin::TickTimerProc);
	args.GetReturnValue().SetUndefined();
}

void InstallMQDocument(Local<ObjectTemplate> global, Isolate* isolate, MQDocument doc);
Local<ObjectTemplate> DialogTemplate(Isolate* isolate);


static Local<ObjectTemplate> NewProcessObject(Isolate* isolate) {
	auto obj = ObjectTemplate::New(isolate);

	auto fileObj = ObjectTemplate::New(isolate);
	fileObj->Set(String::NewFromUtf8(isolate, "write", NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(isolate, WriteFile));

	obj->Set(UTF8("version"), UTF8("v0.1.0"), PropertyAttribute::ReadOnly);
	obj->Set(UTF8("stdout"), fileObj);
	obj->Set(UTF8("stderr"), fileObj);
	obj->Set(UTF8("nextTick"), FunctionTemplate::New(isolate, JSMacroPlugin::SetNextTick));
	obj->Set(UTF8("showWindow"), FunctionTemplate::New(isolate, ShowWindow));
	obj->Set(UTF8("load"), FunctionTemplate::New(isolate, LoadScript));
	obj->Set(UTF8("execScript"), FunctionTemplate::New(isolate, ExecScriptString));
	obj->Set(UTF8("scriptDir"), FunctionTemplate::New(isolate, ScriptDir));
	obj->Set(UTF8("_dialog"), DialogTemplate(isolate));
	return obj;
}

static Local<Context> CreateContext(Isolate *isolate, MQDocument doc) {

	v8::Local<v8::ObjectTemplate> global = v8::ObjectTemplate::New(isolate);

	global->Set(UTF8("process"), NewProcessObject(isolate));
	InstallMQDocument(global, isolate, doc);

	Local<Context> context = Context::New(isolate, nullptr, global);
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
JSMacroPlugin::JSMacroPlugin() : jsContext(nullptr)
{
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
const char *JSMacroPlugin::GetPlugInName(void) {
	return "JavaScript macro runner";
}

//---------------------------------------------------------------------------------------------------------------------
//    ボタンに表示される文字列を返す。
//---------------------------------------------------------------------------------------------------------------------
const char *JSMacroPlugin::EnumString(void) {
	return "JS Macro";
}


//---------------------------------------------------------------------------------------------------------------------
//    アプリケーションの初期化
//---------------------------------------------------------------------------------------------------------------------
BOOL JSMacroPlugin::Initialize()
{
	V8::InitializeICUDefaultLocation("C:/tmp/test");
	// V8::InitializeExternalStartupData("C:/tmp/test");

	platform = platform::CreateDefaultPlatform();
	V8::InitializePlatform(platform);
	V8::Initialize();

	create_params.array_buffer_allocator = v8::ArrayBuffer::Allocator::NewDefaultAllocator();
	isolate = Isolate::New(create_params);

	static Callbacks callback(this);
	if (window == nullptr) {
		MQWindowBase mainwnd = MQWindow::GetMainWindow();
		window = new JSMacroWindow(mainwnd, callback);
	}
	debug_log("Initialized.");
	return TRUE;
}

//---------------------------------------------------------------------------------------------------------------------
//    アプリケーションの終了
//---------------------------------------------------------------------------------------------------------------------
void JSMacroPlugin::Exit()
{
	delete platform;

	// Dispose the isolate and tear down V8.
	isolate->Dispose();
	V8::Dispose();
	V8::ShutdownPlatform();
	delete create_params.array_buffer_allocator;
	if (jsContext) {
		delete jsContext;
		jsContext = nullptr;
	}
}

//---------------------------------------------------------------------------------------------------------------------
//    表示・非表示切り替え要求
//---------------------------------------------------------------------------------------------------------------------
BOOL JSMacroPlugin::Activate(MQDocument doc, BOOL flag)
{
	window->SetVisible(flag != 0);
	return flag;
}

//---------------------------------------------------------------------------------------------------------------------
// 表示・非表示を返す
//---------------------------------------------------------------------------------------------------------------------
BOOL JSMacroPlugin::IsActivated(MQDocument doc)
{
	debug_log(window->GetVisible()?"visible":"hidden");
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
	if (jsContext != nullptr && !jsContext->nextTickFun.IsEmpty()) {
		EscapableHandleScope handleScope(isolate);
		TryCatch trycatch;

		auto fun = jsContext->nextTickFun.Get(isolate);
		jsContext->nextTickFun.Reset();
		auto result = fun->Call(Null(isolate), 0, nullptr);
		if (result.IsEmpty()) {
			Handle<Value> exception = trycatch.Exception();
			String::Utf8Value exception_str(exception);
			Handle<Message> message = Exception::CreateMessage(isolate, exception);
			std::stringstream ss;
			ss << *exception_str << " Line:" << message->GetLineNumber() << " (" << currentScriptPath << ")";
			debug_log(ss.str(), 2);
		}
		return true;
	}
	return false;
}

std::string JSMacroPlugin::GetScriptDir() const {
	size_t p = currentScriptPath.find_last_of("/\\");
	if (p != std::string::npos) {
		return currentScriptPath.substr(0, p + 1);
	}
	return "";
}
MaybeLocal<Value> JSMacroPlugin::ExecScriptCurrentContext(const std::string &source, const std::string &maybepath) {
	return jsContext->ExecScript(source, maybepath);
}

MaybeLocal<Value> JsContext::ExecScript(const std::string &source, const std::string &maybepath) {
	Local<Context> context = isolate->GetCurrentContext();
	EscapableHandleScope handleScope(isolate);
	TryCatch trycatch;

	// Compile the source code.
	MaybeLocal<Script> script = Script::Compile(context, UTF8(source.c_str()), &ScriptOrigin(UTF8(maybepath.c_str())));
	if (script.IsEmpty()) {
		debug_log("COMPILE ERROR:", 2);
		Handle<Value> exception = trycatch.Exception();
		String::Utf8Value exception_str(exception);
		Handle<Message> message = Exception::CreateMessage(isolate, exception);
		std::stringstream ss;
		String::Utf8Value scriptName(message->GetScriptOrigin().ResourceName());
		ss << *exception_str << " Line:" << message->GetLineNumber() << " (" << *scriptName << ")";
		//ss << *exception_str << " Line:" << message->GetLineNumber() << " (" << maybepath << ")";
		debug_log(ss.str(), 2);
	} else {

		auto ret = script.ToLocalChecked()->Run(context);
		if (ret.IsEmpty()) {
			Handle<Value> exception = trycatch.Exception();
			String::Utf8Value exception_str(exception);
			Handle<Message> message = Exception::CreateMessage(isolate, exception);
			std::stringstream ss;

			String::Utf8Value scriptName(message->GetScriptOrigin().ResourceName());
			ss << *exception_str <<  " Line:" << message->GetLineNumber() << " (" << *scriptName << ")";
			debug_log(ss.str(), 2);
			return MaybeLocal<Value>();
		}
		return handleScope.Escape(ret.ToLocalChecked());
	}
	return MaybeLocal<Value>();
}

Local<ObjectTemplate> FileSystemTemplate(Isolate* isolate);

JsContext* JSMacroPlugin::GetJsContext(MQDocument doc) {
	if (!jsContext) {
		jsContext = new JsContext(isolate, doc);
		TCHAR path[MAX_PATH];
		GetModuleFileName(hInstance, path, MAX_PATH);
		std::string coreJsName = std::string(path) + ".core.js";
		currentScriptPath = coreJsName;
		std::ifstream jsfile(coreJsName);
		std::stringstream buffer;
		buffer << jsfile.rdbuf();
		jsfile.close();
		if (jsfile.fail()) {
			debug_log("Read error: " + coreJsName, 2);
		} else {
			auto process = isolate->GetCurrentContext()->Global()->Get(UTF8("process")).As<Object>();
			process->Set(UTF8("unsafe_fs"), FileSystemTemplate(isolate)->NewInstance());
			jsContext->ExecScript(buffer.str(), "core.js");
			process->Delete(UTF8("unsafe_fs")); // core.js only
		}
	}
	return jsContext;
}

void JSMacroPlugin::ExecScript(MQDocument doc, const std::string &fname) {
	if (jsContext) {
		delete jsContext;
		jsContext = nullptr;
	}

	debug_log(std::string("Run: ") + fname);
	std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
	std::ifstream jsfile(converter.from_bytes(fname));
	std::stringstream buffer;
	buffer << jsfile.rdbuf();
	jsfile.close();
	if (jsfile.fail()) {
		debug_log("Read error: " + fname, 2);
	} else {
		GetJsContext(doc);
		currentScriptPath = fname;
		if (!jsContext->ExecScript(buffer.str(), currentScriptPath).IsEmpty()) {
			debug_log("ok.");
		}
	}
	isolate->IdleNotification(100);
}

void JSMacroPlugin::ExecScriptString(MQDocument doc, const std::string &code) {
	debug_log("> "+ code);
	auto ret = GetJsContext(doc)->ExecScript(code);
	if (!ret.IsEmpty()) {
		v8::String::Utf8Value resultStr(ret.ToLocalChecked());
		debug_log(*resultStr);
	}
	isolate->IdleNotification(100);
};
