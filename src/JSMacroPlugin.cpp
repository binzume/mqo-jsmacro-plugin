//---------------------------------------------------------------------------------------------------------------------
//  JavaScript plugin.
//       https://github.com/binzume/mqo-jsmacro-plugin
//---------------------------------------------------------------------------------------------------------------------

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <vector>
#include <fstream>
#include <string>
#include <sstream>
#include <codecvt>
#include <functional>

#include "MQBasePlugin.h"
#include "JSMacroWindow.h"
#include "Utils.h"
#include "qjsutils.h"

#define PLUGIN_VERSION "v0.2.0"
#define PRODUCT_ID 0x7a6e6962
#define PLUGIN_ID 0x7a6e6963

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

static JSContext* CreateContext(JSRuntime* runtime, MQDocument doc, const std::vector<std::string> &argv = std::vector<std::string>());
class JsContext {
public:
  JSContext* ctx;
  UINT_PTR tickTimerId;
	JsContext(JSRuntime* runtime, MQDocument doc, const std::vector<std::string> &argv = std::vector<std::string>())
		: ctx(CreateContext(runtime, doc, argv)) {}
	ValueHolder ExecScript(const std::string &code, const std::string &maybepath = "");
	ValueHolder GetGlobal() {
	  return ValueHolder(ctx, JS_GetGlobalObject(ctx));
	}
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
	ValueHolder ExecScriptCurrentContext(const std::string &code, const std::string &maybepath = "");
	// static JSValue SetNextTick(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv);
	static VOID CALLBACK TickTimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
	JsContext* GetJsContext(MQDocument doc, const std::vector<std::string> &argv = std::vector<std::string>());
	std::string GetScriptDir() const;
protected:
	JSRuntime *runtime;
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

void debug_log(const std::string s, int tag) {
	std::ofstream fout("C:/logs/jsmacro.log", std::ios::app);
	fout << s << std::endl;
	fout.close();
	JSMacroPlugin *plugin = static_cast<JSMacroPlugin*>(GetPluginClass());
	plugin->AddMessage(s, tag);
}

void dump_exception(JSContext *ctx, JSValue val = JS_UNDEFINED) {
  if (!JS_IsUndefined(val)) {
	const char* str = JS_ToCString(ctx, val);
	if (str) {
	  debug_log(str, 2);
	  JS_FreeCString(ctx, str);
	}
  }
  JSValue e = JS_GetException(ctx);
  const char* str = JS_ToCString(ctx, e);
  if (str) {
	debug_log(str, 2);
	JS_FreeCString(ctx, str);
  }
  if (JS_IsError(ctx, e)) {
	JSValue s = JS_GetPropertyStr(ctx, e, "stack");
	if (!JS_IsUndefined(s)) {
	  const char* str = JS_ToCString(ctx, s);
	  if (str) {
		debug_log(str, 2);
		JS_FreeCString(ctx, str);
	  }
	}
	JS_FreeValue(ctx, s);
  }
  JS_FreeValue(ctx, e);
}


//---------------------------------------------------------------------------------------------------------------------
//  js stdout (debug)
//---------------------------------------------------------------------------------------------------------------------


static JSValue WriteFile(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
	std::stringstream ss;
	bool first = true;
	for (int i = 0; i < argc; i++) {
		if (first) {
			first = false;
		}
		else {
			ss << " ";
		}
		ss << convert_jsvalue<std::string>(ctx, argv[i]);
	}
	JSMacroPlugin *plugin = static_cast<JSMacroPlugin*>(GetPluginClass());
	plugin->AddMessage(ss.str(), 0);
	return JS_UNDEFINED;
}

static JSValue LoadScript(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
	if (argc >= 1) {
		JSMacroPlugin *plugin = static_cast<JSMacroPlugin*>(GetPluginClass());
		auto path = convert_jsvalue<std::string>(ctx, argv[0]);
		debug_log("load " + plugin->GetScriptDir() + path);
		std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
		std::ifstream jsfile(converter.from_bytes(plugin->GetScriptDir() + path));
		std::stringstream buffer;
		buffer << jsfile.rdbuf();
		jsfile.close();
		if (jsfile.fail()) {
			debug_log(std::string("Read error: ") + path, 2);
		}
		return plugin->ExecScriptCurrentContext(buffer.str(), path).GetValue();
	}
	return JS_UNDEFINED;
}

static JSValue ExecScriptString(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  if (argc >= 1) {
    	auto code = convert_jsvalue<std::string>(ctx, argv[0]);
	    auto path = (argc >= 2) ? convert_jsvalue<std::string>(ctx, argv[1]) : "";

		JSValue r = JS_Eval(ctx, code.c_str(), code.size(), path.c_str() , JS_EVAL_FLAG_STRICT);
		if (JS_IsException(r)) {
		  dump_exception(ctx, r);
		}
		return r;
	}
  return JS_UNDEFINED;
}

static JSValue ShowWindow(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
	JSMacroPlugin *plugin = static_cast<JSMacroPlugin*>(GetPluginClass());
	if (argc >= 1) {
		plugin->window->SetVisible(convert_jsvalue<bool>(ctx, argv[0]));
	} else {
		plugin->window->SetVisible(true);
	}
	return JS_UNDEFINED;
}

JSValue ScriptDir(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
	JSMacroPlugin *plugin = static_cast<JSMacroPlugin*>(GetPluginClass());
	return JS_NewString(ctx, plugin->GetScriptDir().c_str());
}

VOID CALLBACK JSMacroPlugin::TickTimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime) {
	JSMacroPlugin *plugin = static_cast<JSMacroPlugin*>(GetPluginClass());
	KillTimer(hwnd, idEvent);
	if (plugin->jsContext && plugin->jsContext->tickTimerId == idEvent) {
		plugin->jsContext->tickTimerId = 0;
		plugin->BeginCallback(nullptr);
	}
}

/*
static JSValue JSMacroPlugin::SetNextTick(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
	JSMacroPlugin *plugin = static_cast<JSMacroPlugin*>(GetPluginClass());
	plugin->window->SetVisible(ValueHolder(ctx, argv[0], true).to<bool>());
	plugin->jsContext->nextTickFun.Reset(ctx, args[0].As<Function>());
	uint32_t timerMs = 10;
	if (argc > 1) {
		timerMs = ValueHolder(ctx, argv[1], true).to<uint32_t>();
	}
	if (plugin->jsContext->tickTimerId) {
		KillTimer(NULL, plugin->jsContext->tickTimerId);
	}
	plugin->jsContext->tickTimerId = SetTimer(NULL, 1234, timerMs, JSMacroPlugin::TickTimerProc);
	return JS_UNDEFINED;
}
*/

JSModuleDef* InitDialogModule(JSContext *ctx);
JSModuleDef* InitFsModule(JSContext* ctx);
JSModuleDef* InitChildProcessModule(JSContext* ctx);
void InstallMQDocument(JSContext* ctx, MQDocument doc);

static ValueHolder NewProcessObject(JSContext *ctx, const std::vector<std::string> &args) {
	auto obj = ValueHolder(ctx);

	auto fileObj = ValueHolder(ctx);
	fileObj.Set("write", JS_NewCFunction(ctx, WriteFile, "write", 1));

	obj.Set("version", PLUGIN_VERSION);
	obj.Set(("stdout"), fileObj);
	obj.Set(("stderr"), fileObj);
	// obj.Set(("nextTick"), JS_NewCFunction(ctx, JSMacroPlugin::SetNextTick));
	obj.Set(("showWindow"), JS_NewCFunction(ctx, ShowWindow, "showWindow", 2));
	obj.Set(("load"), JS_NewCFunction(ctx, LoadScript, "load", 2));
	obj.Set(("execScript"), JS_NewCFunction(ctx, ExecScriptString, "execScript", 2));
	obj.Set(("scriptDir"), JS_NewCFunction(ctx, ScriptDir, "scriptDir", 0));
	return obj;
}

static JSContext* CreateContext(JSRuntime *runtime, MQDocument doc, const std::vector<std::string> &args) {

	JSContext* ctx = JS_NewContext(runtime);

	ValueHolder global(ctx, JS_GetGlobalObject(ctx));
  
	global.Set("process", NewProcessObject(ctx, args));

	// TODO: permission settings.
	InitChildProcessModule(ctx);
	InitFsModule(ctx);

	InitDialogModule(ctx);
	InstallMQDocument(ctx, doc);

	return ctx;
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
	*Product = PRODUCT_ID;
	*ID      = PLUGIN_ID;
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
	return "JavaScript macro";
}

//---------------------------------------------------------------------------------------------------------------------
//    アプリケーションの初期化
//---------------------------------------------------------------------------------------------------------------------
BOOL JSMacroPlugin::Initialize()
{
	char cwd[1024];
	GetModuleFileName(hInstance, cwd, sizeof cwd);
	debug_log(cwd);

	// Initialize JS runtime.
	runtime = JS_NewRuntime();

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
	if (jsContext) {
		delete jsContext;
		jsContext = nullptr;
	}
	JS_FreeRuntime(runtime);
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
    JSContext* ctx;
    int ret = JS_ExecutePendingJob(runtime, &ctx);
	if (ret < 0) {
	  dump_exception(ctx);
	}
	return ret > 0;
}

std::string JSMacroPlugin::GetScriptDir() const {
	size_t p = currentScriptPath.find_last_of("/\\");
	if (p != std::string::npos) {
		return currentScriptPath.substr(0, p + 1);
	}
	return "";
}
ValueHolder JSMacroPlugin::ExecScriptCurrentContext(const std::string &source, const std::string &maybepath) {
	return jsContext->ExecScript(source, maybepath);
}

ValueHolder JsContext::ExecScript(const std::string &source, const std::string &maybepath) {
  auto val = JS_Eval(ctx, source.c_str(), source.size(), maybepath.c_str(), JS_EVAL_TYPE_MODULE);
  if (JS_IsException(val)) {
	dump_exception(ctx, val);
  }
  return ValueHolder(ctx, val);
}

JsContext* JSMacroPlugin::GetJsContext(MQDocument doc, const std::vector<std::string> &args) {
	if (!jsContext) {
		jsContext = new JsContext(runtime, doc, args);
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
		  ValueHolder argv(jsContext->ctx, JS_NewArray(jsContext->ctx));

			for (int i = 0; i < args.size() - 1; i++) {
				argv.Set(i, args[i+1]);
			}
			auto global = jsContext->GetGlobal();
			auto unsafe = JS_NewObject(jsContext->ctx);
			// unsafe->Set(UTF8("fs"), FileSystemTemplate(isolate));
			// unsafe->Set(UTF8("child_process"), ChildProcessTemplate(isolate));
			global.Set("unsafe", unsafe);
			global["process"].Set("argv", (argv));
			jsContext->ExecScript(buffer.str(), "core.js");
			global.Delete("unsafe"); // core.js only
		}
	}
	return jsContext;
}

std::vector<std::string> SplitString(const std::string& s, char delim) {
	std::vector<std::string> results;
	size_t offset = 0;
	while (true) {
		size_t next = s.find_first_of(delim, offset);
		results.push_back(s.substr(offset, next - offset));
		if (next == std::string::npos) {
			break;
		}
		offset = next + 1;
	}
	return results;
}


void JSMacroPlugin::ExecScript(MQDocument doc, const std::string &fname) {
	if (jsContext) {
		delete jsContext;
		jsContext = nullptr;
	}

	debug_log(std::string("Run: ") + fname);
	auto argv = SplitString(fname, ';');
	std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
	std::ifstream jsfile(converter.from_bytes(argv[0]));
	std::stringstream buffer;
	buffer << jsfile.rdbuf();
	jsfile.close();
	if (jsfile.fail()) {
		debug_log("Read error: " + fname, 2);
	} else {
		GetJsContext(doc, argv);
		currentScriptPath = argv[0];
		if (!jsContext->ExecScript(buffer.str(), currentScriptPath).IsException()) {
			debug_log("ok.");
		}
	}
	// TODO:GC
}

void JSMacroPlugin::ExecScriptString(MQDocument doc, const std::string &code) {
	debug_log("> "+ code);
	auto ret = GetJsContext(doc)->ExecScript(code);
	if (!ret.IsUndefined()) {
		debug_log(ret.to<std::string>());
	}
	// TODO:GC
}
