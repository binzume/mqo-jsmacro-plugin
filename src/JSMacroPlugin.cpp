//---------------------------------------------------------------------------------------------------------------------
//  JavaScript plugin.
//       https://github.com/binzume/mqo-jsmacro-plugin
//---------------------------------------------------------------------------------------------------------------------

#include <windows.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <functional>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "MQBasePlugin.h"
#include "MQSetting.h"
#include "JSContext.h"
#include "JSMacroWindow.h"
#include "Utils.h"
#include "preference.h"

#define PLUGIN_VERSION "v0.2.0"
#define PRODUCT_ID 0x7a6e6962
#define PLUGIN_ID 0x7a6e6963

HINSTANCE hInstance;

//---------------------------------------------------------------------------------------------------------------------
//  DllMain
//---------------------------------------------------------------------------------------------------------------------
BOOL APIENTRY DllMain(HANDLE hModule, DWORD ul_reason_for_call,
                      LPVOID lpReserved) {
  hInstance = (HINSTANCE)hModule;
  return TRUE;
}

class JSMacroPlugin : public MQStationPlugin {
 public:
  MQDocument currentDocument = nullptr;

  JSMacroPlugin();

  void GetPlugInID(DWORD *Product, DWORD *ID) override;
  const char *GetPlugInName(void) override;
  const wchar_t *EnumString(void) override;
  BOOL Initialize() override;
  void Exit() override;
  BOOL Activate(MQDocument doc, BOOL flag) override;
  BOOL IsActivated(MQDocument doc) override;
  void OnNewDocument(MQDocument doc, const char *filename,
                     NEW_DOCUMENT_PARAM &param) override {
    currentDocument = doc;
    ParseElements(param.elem);
  }
  void OnSaveDocument(MQDocument doc, const char *filename,
                      SAVE_DOCUMENT_PARAM &param) override {
    currentDocument = doc;
    WriteElements(param.elem);
  }
  void OnSavePastDocument(MQDocument doc, const char *filename,
                          SAVE_DOCUMENT_PARAM &param) override {
    currentDocument = doc;
    WriteElements(param.elem);
  }
  void OnEndDocument(MQDocument doc) override {
    if (jsContext) {
      delete jsContext;
      jsContext = nullptr;
    }
    currentDocument = nullptr;
  };
  void OnDraw(MQDocument doc, MQScene scene, int width, int height) override;
  void OnUpdateObjectList(MQDocument doc) override;
  const char *EnumSubCommand(int index) override {
    return index < subCommand.size() ? subCommand[index].c_str() : nullptr;
  };
  const wchar_t *GetSubCommandString(int index) override {
    return index < subCommandStr.size() ? subCommandStr[index].c_str()
                                        : nullptr;
  };
  BOOL OnSubCommand(MQDocument doc, int index) override {
    InitWindow();
    window->Execute(doc, index - 1);
    return TRUE;
  };

  void AddMessage(const std::string &message, int tag = 0) {
    if (logFilePath.length() > 0) {
      std::ofstream fout(logFilePath, std::ios::app);
      fout << message << std::endl;
      fout.close();
    }
    if (tag == 2) {
      ShowWindow(true);
    }
    if (window) window->AddMessage(message, tag);
  }
  void ExecScript(MQDocument doc, const std::string &jsfile);
  ValueHolder ExecScriptCurrentContext(const std::string &code,
                                       const std::string &maybepath = "",
                                       bool asmodule = true);
  static VOID CALLBACK TickTimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent,
                                     DWORD dwTime);
  JsContext *GetJsContext(MQDocument doc, const std::vector<std::string> &argv =
                                              std::vector<std::string>());
  std::string GetScriptDir() const;

  void ShowWindow(int visible) {
    if (visible && window == nullptr) {
      InitWindow();
    }
    if (window) window->SetVisible(visible);
  }

  void LoadSettings() {
    MQSetting *setting = OpenSetting();
    setting->Load(PREF_LOG_FILE_PATH, logFilePath);
    CloseSetting(setting);
  }

 protected:
  JSRuntime *runtime;
  JsContext *jsContext;
  JSMacroWindow *window = nullptr;
  std::string currentScriptPath;
  std::wstring logFilePath;
  std::map<std::string, std::string> pluginKeyValue;
  std::vector<std::string> subCommand;
  std::vector<std::wstring> subCommandStr;

  void InitWindow();
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
  void ExecuteScript(const std::string &path, MQDocument doc) override {
    plugin->ExecScript(doc, path);
    MQ_RefreshView(nullptr);
  }
  void OnCloseWindow(MQDocument doc) override { plugin->WindowClose(); }
  void OnSettingUpdated() override { plugin->LoadSettings(); }
};

std::vector<std::string> SplitString(const std::string &s, char delim);

void debug_log(const std::string s, int tag) {
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
//    プラグインのベースクラスを返す
//---------------------------------------------------------------------------------------------------------------------
MQBasePlugin *GetPluginClass() {
  static JSMacroPlugin plugin;
  return &plugin;
}

//---------------------------------------------------------------------------------------------------------------------
//    コンストラクタ
//---------------------------------------------------------------------------------------------------------------------
JSMacroPlugin::JSMacroPlugin() : runtime(JS_NewRuntime()), jsContext(nullptr) {}

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
  LoadSettings();

  subCommand.push_back("EXEC");
  subCommandStr.push_back(L"Run");
  for (int i = 0; i < PRESET_SCRIPT_COUNT; i++) {
    subCommand.push_back("EXEC_P" + std::to_string(i));
    subCommandStr.push_back(L"Preset " + std::to_wstring(i));
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
  ShowWindow(flag != 0);
  return flag;
}

//---------------------------------------------------------------------------------------------------------------------
// 表示・非表示を返す
//---------------------------------------------------------------------------------------------------------------------
BOOL JSMacroPlugin::IsActivated(MQDocument doc) {
  return window && window->GetVisible();
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
    const std::string &source, const std::string &maybepath, bool asmodule) {
  return jsContext->ExecScript(source, maybepath, asmodule);
}

void JSMacroPlugin::InitWindow() {
  if (window == nullptr) {
    static Callbacks callback(this);
    MQWindowBase mainwnd = MQWindow::GetMainWindow();
    window = new JSMacroWindow(mainwnd, callback);
  }
}

VOID CALLBACK JSMacroPlugin::TickTimerProc(HWND hwnd, UINT uMsg,
                                           UINT_PTR idEvent, DWORD dwTime) {
  JSMacroPlugin *plugin = static_cast<JSMacroPlugin *>(GetPluginClass());
  KillTimer(hwnd, idEvent);
  if (plugin->jsContext && plugin->jsContext->tickTimerId == idEvent) {
    plugin->jsContext->tickTimerId = 0;
    plugin->BeginCallback(nullptr);
  }
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
      return JS_EXCEPTION;
    }
    return unwrap(plugin->ExecScriptCurrentContext(
        buffer.str(), path, convert_jsvalue<bool>(ctx, argv[1])));
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
  plugin->ShowWindow(argc < 1 || convert_jsvalue<bool>(ctx, argv[0]));
  return JS_UNDEFINED;
}

JSValue ScriptDir(JSContext *ctx) {
  JSMacroPlugin *plugin = static_cast<JSMacroPlugin *>(GetPluginClass());
  return JS_NewString(ctx, plugin->GetScriptDir().c_str());
}

std::u8string GetDocumentFileName() {
  JSMacroPlugin *plugin = static_cast<JSMacroPlugin *>(GetPluginClass());
  if (plugin->currentDocument == nullptr) {
    return u8"";
  }
  std::filesystem::path path(plugin->GetFilename(plugin->currentDocument));
  return path.u8string();
}

#if MQPLUGIN_VERSION >= 0x0471
bool InsertDocument(const std::string &filename, int mergeMode) {
  JSMacroPlugin *plugin = static_cast<JSMacroPlugin *>(GetPluginClass());
  if (plugin->currentDocument == nullptr) {
    return false;
  }
  MQBasePlugin::OPEN_DOCUMENT_OPTION option;
  option.MergeMode = (MQBasePlugin::OPEN_DOCUMENT_OPTION::MERGE_MODE)mergeMode;

  std::filesystem::path path(
      reinterpret_cast<const char8_t *>(filename.c_str()));

  return plugin->InsertDocument(plugin->currentDocument, path.c_str(), option);
}
#endif

bool SaveDocument(const std::string &filename, int productId, int pluginId,
                  int index) {
  JSMacroPlugin *plugin = static_cast<JSMacroPlugin *>(GetPluginClass());
  if (plugin->currentDocument == nullptr) {
    return false;
  }

  std::filesystem::path path(
      reinterpret_cast<const char8_t *>(filename.c_str()));
  auto fullpath = std::filesystem::weakly_canonical(path);

  MQBasePlugin::SAVE_DOCUMENT_OPTION option;
  option.ExportProductID = productId;
  option.ExportPluginID = pluginId;
  option.ExportIndex = index;

  return plugin->SaveDocument(plugin->currentDocument,
                              fullpath.wstring().c_str(), option);
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

  static const JSCFunctionListEntry funcs[] = {
    function_entry<JsContext::RegisterTimer>("registerTimer", 3),
    function_entry<JsContext::RemoveTimer>("removeTimer"),
    function_entry("showWindow", 2, ShowWindow),
    function_entry("load", 2, LoadScript),
    function_entry("execScript", 2, ExecScriptString),
    function_entry<&GetDocumentFileName>("getDocumentFileName"),
    function_entry<&SaveDocument>("saveDocument"),
    function_entry<&ScriptDir>("scriptDir"),
#if MQPLUGIN_VERSION >= 0x0471
    function_entry<&InsertDocument>("insertDocument"),
#endif
  };
  JS_SetPropertyFunctionList(ctx, obj.GetValueNoDup(), funcs,
                             (int)std::size(funcs));

  obj.Set("version", PLUGIN_VERSION);
  obj.Set("stdout", fileObj.GetValue());
  obj.Set("stderr", fileObj.GetValue());
  return obj;
}

JsContext *JSMacroPlugin::GetJsContext(MQDocument doc,
                                       const std::vector<std::string> &args) {
  if (!jsContext) {
    jsContext = new JsContext(runtime, args);
    auto ctx = jsContext->ctx;
    ValueHolder global(ctx, JS_GetGlobalObject(ctx));

    global.Set("process", NewProcessObject(ctx, args));

    // TODO: permission settings.
    InitChildProcessModule(ctx);
    InitFsModule(ctx);

    InitDialogModule(ctx);
    InstallMQDocument(ctx, doc, &pluginKeyValue);


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

      for (size_t i = 0; i < args.size() - 1; i++) {
        argv.Set(uint32_t(i), args[i + 1]);
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
  if (fname.starts_with("js:")) {
    std::string code = fname.substr(3);
    debug_log("> " + code);
    auto ret = GetJsContext(doc)->ExecScript(code);
    if (!ret.IsUndefined()) {
      debug_log(ret.To<std::string>());
    }
    JS_RunGC(runtime);
    return;
  }
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
    return;
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

  MQSetting *setting = OpenSetting();
  setting->Save(PREF_LAST_SCRIPT_PATH, fname);
  CloseSetting(setting);
}
