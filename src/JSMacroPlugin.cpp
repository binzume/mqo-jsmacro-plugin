//---------------------------------------------------------------------------------------------------------------------
//  JavaScript plugin.
//       https://github.com/binzume/mqo-jsmacro-plugin
//---------------------------------------------------------------------------------------------------------------------

#include <windows.h>

#include <algorithm>
#include <fstream>
#include <functional>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "JSMacroWindow.h"
#include "MQBasePlugin.h"
#include "Utils.h"
#include "qjsutils.h"

#define PLUGIN_VERSION "v0.2.0"
#define PRODUCT_ID 0x7a6e6962
#define PLUGIN_ID 0x7a6e6964  // 0x7a6e6963

HINSTANCE hInstance;

//---------------------------------------------------------------------------------------------------------------------
//  DllMain
//---------------------------------------------------------------------------------------------------------------------
BOOL APIENTRY DllMain(HANDLE hModule, DWORD ul_reason_for_call,
                      LPVOID lpReserved) {
  hInstance = (HINSTANCE)hModule;
  return TRUE;
}

static JSContext *CreateContext(
    JSRuntime *runtime, MQDocument doc,
    const std::vector<std::string> &argv = std::vector<std::string>());
std::vector<std::string> SplitString(const std::string &s, char delim);
void dump_exception(JSContext *ctx, JSValue val = JS_UNDEFINED);
class JsContext {
 public:
  JSContext *ctx;
  UINT_PTR tickTimerId = 0;
  struct TimerEntry {
    int32_t timeout;
    uint32_t id;
    JSValue func;
  };
  std::vector<TimerEntry> timers;
  JsContext(JSRuntime *runtime, MQDocument doc,
            const std::vector<std::string> &argv = std::vector<std::string>())
      : ctx(CreateContext(runtime, doc, argv)) {}
  ValueHolder ExecScript(const std::string &code,
                         const std::string &maybepath = "");
  ValueHolder GetGlobal() { return ValueHolder(ctx, JS_GetGlobalObject(ctx)); }
  void RegisterTimer(JSValue f, int32_t time, uint32_t id = 0) {
    timers.push_back(TimerEntry{time, id, f});
  }
  void RemoveTimer(uint32_t id) {
    timers.erase(std::remove_if(timers.begin(), timers.end(),
                                [id](auto &t) { return t.id == id; }),
                 timers.end());
  }
  int32_t GetNextTimeout(int32_t now) {
    if (timers.empty()) {
      return -1;
    }
    std::sort(timers.begin(), timers.end(), [now](auto &a, auto &b) -> bool {
      return (a.timeout - now) > (b.timeout - now);  // 2^32 wraparound
    });
    int next = timers.back().timeout - now;
    return max(next, 0);
  }
  bool ConsumeTimer(int32_t now) {
    std::vector<TimerEntry> t;
    int32_t eps = 2;
    while (!timers.empty() && timers.back().timeout - now <= eps) {
      t.push_back(timers.back());
      timers.pop_back();
    }
    for (auto &ent : t) {
      JSValue r =
          JS_Call(ctx, ent.func, ent.func, 0, nullptr);  // may update timers.
      if (JS_IsException(r)) {
        dump_exception(ctx, r);
      }
      JS_FreeValue(ctx, r);
      JS_FreeValue(ctx, ent.func);
    }
    return !t.empty();
  }
  void SetNextTimer(UINT_PTR newTimer) {
    if (tickTimerId) {
      KillTimer(NULL, tickTimerId);
    }
    tickTimerId = newTimer;
  }
  ~JsContext() {
    SetNextTimer(0);
    for (auto &ent : timers) {
      JS_FreeValue(ctx, ent.func);
    }
    JS_FreeContext(ctx);
  }
};

const char *SUB_COMMAND[PRESET_SCRIPT_COUNT + 1] = {
    "EXEC", "EXEC_P0", "EXEC_P1", "EXEC_P2", "EXEC_P3", "EXEC_P4"};
const wchar_t *SUB_COMMAND_STR[PRESET_SCRIPT_COUNT + 1] = {
    L"Run", L"Run 1", L"Run 2", L"Run 3", L"Run 4", L"Run 5"};

class JSMacroPlugin : public MQStationPlugin {
 public:
  std::map<std::string, std::string> pluginKeyValue;
  std::string currentScriptPath;
  JSMacroWindow *window;
  MQDocument currentDocument = nullptr;
  JSMacroPlugin();

  void GetPlugInID(DWORD *Product, DWORD *ID);
  const char *GetPlugInName(void);
  const wchar_t *EnumString(void);
  BOOL Initialize();
  void Exit();
  BOOL Activate(MQDocument doc, BOOL flag);
  BOOL IsActivated(MQDocument doc);
  void OnNewDocument(MQDocument doc, const char *filename,
                     NEW_DOCUMENT_PARAM &param) {
    currentDocument = doc;
    ParseElements(param.elem);
  }
  void OnSaveDocument(MQDocument doc, const char *filename,
                      SAVE_DOCUMENT_PARAM &param) {
    currentDocument = doc;
    WriteElements(param.elem);
  }
  void OnSavePastDocument(MQDocument doc, const char *filename,
                          SAVE_DOCUMENT_PARAM &param) {
    currentDocument = doc;
    WriteElements(param.elem);
  }
  void OnEndDocument(MQDocument doc) {
    if (jsContext) {
      delete jsContext;
      jsContext = nullptr;
    }
    currentDocument = nullptr;
  };
  void OnDraw(MQDocument doc, MQScene scene, int width, int height);
  void OnUpdateObjectList(MQDocument doc);
  const char *EnumSubCommand(int index) {
    if (index <= PRESET_SCRIPT_COUNT) {
      return SUB_COMMAND[index];
    }
    return nullptr;
  };
  const wchar_t *GetSubCommandString(int index) {
    return SUB_COMMAND_STR[index];
  };
  BOOL OnSubCommand(MQDocument doc, int index) {
    window->Execute(doc, index - 1);
    return TRUE;
  };
  void AddMessage(const std::string &message, int tag = 0) {
    if (window) window->AddMessage(message, tag);
  }
  void ExecScript(MQDocument doc, const std::string &jsfile);
  void ExecScriptString(MQDocument doc, const std::string &code);
  ValueHolder ExecScriptCurrentContext(const std::string &code,
                                       const std::string &maybepath = "");
  static JSValue RegisterTimer(JSContext *ctx, JSValueConst this_val, int argc,
                               JSValueConst *argv);
  static void RemoveTimer(int id);
  static VOID CALLBACK TickTimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent,
                                     DWORD dwTime);
  JsContext *GetJsContext(MQDocument doc, const std::vector<std::string> &argv =
                                              std::vector<std::string>());
  std::string GetScriptDir() const;

 protected:
  JSRuntime *runtime;
  JsContext *jsContext;

  virtual bool ExecuteCallback(MQDocument doc, void *option);

  void ParseElements(MQXmlElement elem) {
    if (elem == nullptr) {
      return;
    }
    MQXmlElement child = elem->FirstChildElement();
    while (child != NULL) {
      if (child->GetName() == "KeyValue") {
        MQXmlElement item = child->FirstChildElement();
        while (item != NULL) {
          debug_log(item->GetAttribute("key"));
          pluginKeyValue[item->GetAttribute("key")] = item->GetText();
          item = item->NextChildElement(child);
        }
      }
      child = elem->NextChildElement(child);
    }
  }
  void WriteElements(MQXmlElement elem) {
    if (elem == nullptr || pluginKeyValue.empty()) {
      return;
    }
    MQXmlElement keyvalue = elem->AddChildElement("KeyValue");
    for (auto kv : pluginKeyValue) {
      MQXmlElement item = keyvalue->AddChildElement("Item");
      item->SetAttribute("key", kv.first.c_str());
      item->SetText(kv.second.c_str());
    }
  }
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
  void OnCloseWindow(MQDocument doc) { plugin->WindowClose(); }
};

void debug_log(const std::string s, int tag) {
  std::ofstream fout("C:/logs/jsmacro.log", std::ios::app);
  fout << s << std::endl;
  fout.close();
  JSMacroPlugin *plugin = static_cast<JSMacroPlugin *>(GetPluginClass());
  plugin->AddMessage(s, tag);
}

void print_memory_usage(JSRuntime *runtime) {
  JSMemoryUsage m;
  JS_ComputeMemoryUsage(runtime, &m);
  debug_log(std::string("malloc count:") + std::to_string(m.malloc_count));
  debug_log(std::string("memory_used_size:") +
            std::to_string(m.memory_used_size));
  debug_log(std::string("obj_count:") + std::to_string(m.obj_count));
}

void dump_exception(JSContext *ctx, JSValue val) {
  JSMacroPlugin *plugin = static_cast<JSMacroPlugin *>(GetPluginClass());
  plugin->window->SetVisible(true);

  if (!JS_IsUndefined(val)) {
    const char *str = JS_ToCString(ctx, val);
    if (str) {
      debug_log(str, 2);
      JS_FreeCString(ctx, str);
    } else {
      debug_log("[Exception]", 2);
    }
  }
  JSValue e = JS_GetException(ctx);
  const char *str = JS_ToCString(ctx, e);
  if (str) {
    debug_log(str, 2);
    JS_FreeCString(ctx, str);
  }
  if (JS_IsError(ctx, e)) {
    JSValue s = JS_GetPropertyStr(ctx, e, "stack");
    if (!JS_IsUndefined(s)) {
      const char *str = JS_ToCString(ctx, s);
      if (str) {
        for (auto line : SplitString(str, '\n')) {
          debug_log(line, 2);
        }
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

static JSValue WriteFile(JSContext *ctx, JSValueConst this_val, int argc,
                         JSValueConst *argv) {
  std::stringstream ss;
  bool first = true;
  for (int i = 0; i < argc; i++) {
    if (first) {
      first = false;
    } else {
      ss << " ";
    }
    ss << convert_jsvalue<std::string>(ctx, argv[i]);
  }
  JSMacroPlugin *plugin = static_cast<JSMacroPlugin *>(GetPluginClass());
  plugin->AddMessage(ss.str(), 0);
  return JS_UNDEFINED;
}

static JSValue LoadScript(JSContext *ctx, JSValueConst this_val, int argc,
                          JSValueConst *argv) {
  if (argc >= 1) {
    JSMacroPlugin *plugin = static_cast<JSMacroPlugin *>(GetPluginClass());
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

static JSValue ExecScriptString(JSContext *ctx, JSValueConst this_val, int argc,
                                JSValueConst *argv) {
  if (argc >= 1) {
    auto code = convert_jsvalue<std::string>(ctx, argv[0]);
    auto path = (argc >= 2) ? convert_jsvalue<std::string>(ctx, argv[1]) : "";

    JSValue r = JS_Eval(ctx, code.c_str(), code.size(), path.c_str(),
                        JS_EVAL_FLAG_STRICT);
    if (JS_IsException(r)) {
      dump_exception(ctx, r);
    }
    return r;
  }
  return JS_UNDEFINED;
}

static JSValue ShowWindow(JSContext *ctx, JSValueConst this_val, int argc,
                          JSValueConst *argv) {
  JSMacroPlugin *plugin = static_cast<JSMacroPlugin *>(GetPluginClass());
  if (argc >= 1) {
    plugin->window->SetVisible(convert_jsvalue<bool>(ctx, argv[0]));
  } else {
    plugin->window->SetVisible(true);
  }
  return JS_UNDEFINED;
}

JSValue ScriptDir(JSContext *ctx, JSValueConst this_val, int argc,
                  JSValueConst *argv) {
  JSMacroPlugin *plugin = static_cast<JSMacroPlugin *>(GetPluginClass());
  return JS_NewString(ctx, plugin->GetScriptDir().c_str());
}

std::string GetDocumentFileName() {
  JSMacroPlugin *plugin = static_cast<JSMacroPlugin *>(GetPluginClass());
  if (plugin->currentDocument == nullptr) {
    return "";
  }
  std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
  auto filename = plugin->GetFilename(plugin->currentDocument);
  return converter.to_bytes(filename);
}

#if MQPLUGIN_VERSION >= 0x0471
void InsertDocument(std::string filename, int mergeMode) {
  JSMacroPlugin *plugin = static_cast<JSMacroPlugin *>(GetPluginClass());
  if (plugin->currentDocument == nullptr) {
    return;
  }
  MQBasePlugin::OPEN_DOCUMENT_OPTION option;
  option.MergeMode = (MQBasePlugin::OPEN_DOCUMENT_OPTION::MERGE_MODE)mergeMode;

  std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
  auto filenameW = converter.from_bytes(filename);

  plugin->InsertDocument(plugin->currentDocument, filenameW.c_str(), option);
}
#endif

VOID CALLBACK JSMacroPlugin::TickTimerProc(HWND hwnd, UINT uMsg,
                                           UINT_PTR idEvent, DWORD dwTime) {
  JSMacroPlugin *plugin = static_cast<JSMacroPlugin *>(GetPluginClass());
  KillTimer(hwnd, idEvent);
  if (plugin->jsContext && plugin->jsContext->tickTimerId == idEvent) {
    plugin->jsContext->tickTimerId = 0;
    plugin->BeginCallback(nullptr);
  }
}

JSValue JSMacroPlugin::RegisterTimer(JSContext *ctx, JSValueConst this_val,
                                     int argc, JSValueConst *argv) {
  JSMacroPlugin *plugin = static_cast<JSMacroPlugin *>(GetPluginClass());

  uint32_t timerMs = 10;
  if (argc > 1) {
    timerMs = convert_jsvalue<uint32_t>(ctx, argv[1]);
  }
  plugin->jsContext->RegisterTimer(JS_DupValue(ctx, argv[0]),
                                   timerMs + GetTickCount(),
                                   convert_jsvalue<uint32_t>(ctx, argv[2]));
  return JS_UNDEFINED;
}

void JSMacroPlugin::RemoveTimer(int id) {
  JSMacroPlugin *plugin = static_cast<JSMacroPlugin *>(GetPluginClass());
  plugin->jsContext->RemoveTimer(id);
}

JSModuleDef *InitDialogModule(JSContext *ctx);
JSModuleDef *InitFsModule(JSContext *ctx);
JSModuleDef *InitChildProcessModule(JSContext *ctx);
void InstallMQDocument(JSContext *ctx, MQDocument doc,
                       std::map<std::string, std::string> *keyValue);

static ValueHolder NewProcessObject(JSContext *ctx,
                                    const std::vector<std::string> &args) {
  auto obj = ValueHolder(ctx);

  auto fileObj = ValueHolder(ctx);
  fileObj.Set("write", JS_NewCFunction(ctx, WriteFile, "write", 1));

  obj.Set("version", PLUGIN_VERSION);
  obj.Set(("stdout"), fileObj.GetValue());
  obj.Set(("stderr"), fileObj.GetValue());
  obj.Set(("registerTimer"), JS_NewCFunction(ctx, JSMacroPlugin::RegisterTimer,
                                             "registerTimer", 3));
  obj.Set(("removeTimer"),
          JS_NewCFunction(ctx, method_wrapper<JSMacroPlugin::RemoveTimer>,
                          "removeTimer", 1));
  obj.Set(("showWindow"), JS_NewCFunction(ctx, ShowWindow, "showWindow", 2));
  obj.Set(("load"), JS_NewCFunction(ctx, LoadScript, "load", 2));
  obj.Set(("execScript"),
          JS_NewCFunction(ctx, ExecScriptString, "execScript", 2));
  obj.Set(("scriptDir"), JS_NewCFunction(ctx, ScriptDir, "scriptDir", 0));
  obj.Set(("getDocumentFileName"),
          JS_NewCFunction(ctx, method_wrapper<GetDocumentFileName>,
                          "getDocumentFileName", 0));

#if MQPLUGIN_VERSION >= 0x0471
  obj.Set(("insertDocument"),
          JS_NewCFunction(ctx, method_wrapper<InsertDocument>, "insertDocument",
                          2));
#endif
  return obj;
}

static JSContext *CreateContext(JSRuntime *runtime, MQDocument doc,
                                const std::vector<std::string> &args) {
  JSContext *ctx = JS_NewContext(runtime);

  ValueHolder global(ctx, JS_GetGlobalObject(ctx));

  global.Set("process", NewProcessObject(ctx, args));

  // TODO: permission settings.
  InitChildProcessModule(ctx);
  InitFsModule(ctx);

  InitDialogModule(ctx);
  JSMacroPlugin *plugin = static_cast<JSMacroPlugin *>(GetPluginClass());
  InstallMQDocument(ctx, doc, &plugin->pluginKeyValue);

  return ctx;
}

//---------------------------------------------------------------------------------------------------------------------
//    プラグインのベースクラスを返す
//---------------------------------------------------------------------------------------------------------------------
MQBasePlugin *GetPluginClass() {
  static JSMacroPlugin plugin;
  return &plugin;
}

//---------------------------------------------------------------------------------------------------------------------
//    コンストラクタ
//---------------------------------------------------------------------------------------------------------------------
JSMacroPlugin::JSMacroPlugin() : jsContext(nullptr) {}

//---------------------------------------------------------------------------------------------------------------------
//    プラグインIDを返す。
//---------------------------------------------------------------------------------------------------------------------
void JSMacroPlugin::GetPlugInID(DWORD *Product, DWORD *ID) {
  *Product = PRODUCT_ID;
  *ID = PLUGIN_ID;
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
const wchar_t *JSMacroPlugin::EnumString(void) { return L"JavaScript macro"; }

//---------------------------------------------------------------------------------------------------------------------
//    アプリケーションの初期化
//---------------------------------------------------------------------------------------------------------------------
BOOL JSMacroPlugin::Initialize() {
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
void JSMacroPlugin::Exit() {
  if (jsContext) {
    delete jsContext;
    jsContext = nullptr;
  }
  JS_FreeRuntime(runtime);
}

//---------------------------------------------------------------------------------------------------------------------
//    表示・非表示切り替え要求
//---------------------------------------------------------------------------------------------------------------------
BOOL JSMacroPlugin::Activate(MQDocument doc, BOOL flag) {
  window->SetVisible(flag != 0);
  return flag;
}

//---------------------------------------------------------------------------------------------------------------------
// 表示・非表示を返す
//---------------------------------------------------------------------------------------------------------------------
BOOL JSMacroPlugin::IsActivated(MQDocument doc) {
  debug_log(window->GetVisible() ? "visible" : "hidden");
  return window->GetVisible();
}

//---------------------------------------------------------------------------------------------------------------------
// 描画
//---------------------------------------------------------------------------------------------------------------------
void JSMacroPlugin::OnDraw(MQDocument doc, MQScene scene, int width,
                           int height) {}

void JSMacroPlugin::OnUpdateObjectList(MQDocument doc) {}

bool JSMacroPlugin::ExecuteCallback(MQDocument doc, void *option) {
  JSContext *ctx;

  int32_t now = GetTickCount();
  bool update = false;
  int nextTick = -1;

  if (jsContext != nullptr) {
    update = jsContext->ConsumeTimer(now);
    nextTick = jsContext->GetNextTimeout(now);
  }

  int ret = JS_ExecutePendingJob(runtime, &ctx);
  if (ret < 0) {
    dump_exception(ctx);
    return false;
  } else if (ret > 0) {
    nextTick = 1;
    update = true;
  }
  if (jsContext != nullptr && nextTick >= 0) {
    jsContext->SetNextTimer(
        SetTimer(NULL, PLUGIN_ID, nextTick, JSMacroPlugin::TickTimerProc));
  }
  return update;
}

std::string JSMacroPlugin::GetScriptDir() const {
  size_t p = currentScriptPath.find_last_of("/\\");
  if (p != std::string::npos) {
    return currentScriptPath.substr(0, p + 1);
  }
  return "";
}
ValueHolder JSMacroPlugin::ExecScriptCurrentContext(
    const std::string &source, const std::string &maybepath) {
  return jsContext->ExecScript(source, maybepath);
}

ValueHolder JsContext::ExecScript(const std::string &source,
                                  const std::string &maybepath) {
  auto val = JS_Eval(ctx, source.c_str(), source.size(), maybepath.c_str(),
                     JS_EVAL_TYPE_MODULE);
  if (JS_IsException(val)) {
    dump_exception(ctx, val);
  }
  return ValueHolder(ctx, val);
}

JsContext *JSMacroPlugin::GetJsContext(MQDocument doc,
                                       const std::vector<std::string> &args) {
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
        argv.Set(i, args[i + 1]);
      }
      auto global = jsContext->GetGlobal();
      auto unsafe = JS_NewObject(jsContext->ctx);
      global.Set("unsafe", unsafe);
      global["process"].Set("argv", argv);

      jsContext->ExecScript(buffer.str(), "core.js");
      global.Delete("unsafe");  // core.js only
    }
  }
  return jsContext;
}

std::vector<std::string> SplitString(const std::string &s, char delim) {
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
  JS_RunGC(runtime);
  int32_t next = jsContext->GetNextTimeout(GetTickCount());
  if (JS_IsJobPending(runtime)) {
    next = 1;
  }
  if (next >= 0) {
    jsContext->SetNextTimer(
        SetTimer(NULL, PLUGIN_ID, next, JSMacroPlugin::TickTimerProc));
  }
}

void JSMacroPlugin::ExecScriptString(MQDocument doc, const std::string &code) {
  debug_log("> " + code);
  auto ret = GetJsContext(doc)->ExecScript(code);
  if (!ret.IsUndefined()) {
    debug_log(ret.To<std::string>());
  }
  JS_RunGC(runtime);
}
