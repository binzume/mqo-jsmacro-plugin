
#define WIN32_LEAN_AND_MEAN
#include <codecvt>
#include <fstream>
#include <sstream>
#include <vector>

#include "Utils.h"
#include "qjsutils.h"

//---------------------------------------------------------------------------------------------------------------------
// File System
//---------------------------------------------------------------------------------------------------------------------

class JSFile {
  std::string path;  // utf8 string
  bool writable;

 public:
  JSFile(const std::string& _path, bool _writable)
      : path(_path), writable(_writable) {}

  static JSClassID class_id;
  static const JSCFunctionListEntry proto_funcs[];

  std::string GetPath() { return path; }
  bool IsWritable() { return writable; }

  JSValue ReadFileSync(JSContext* ctx) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;

    std::ifstream jsfile(converter.from_bytes(path));
    std::stringstream buffer;
    buffer << jsfile.rdbuf();
    jsfile.close();
    if (jsfile.fail()) {
      JS_ThrowInternalError(ctx, "read failed.");
      return JS_EXCEPTION;
    }
    return to_jsvalue(ctx, buffer.str());
  }

  JSValue WriteFileSync(JSContext* ctx, std::string data, JSValue flags) {
    ValueHolder f(ctx, flags, true);

    int mode = std::ofstream::binary;
    if (f["mode"].To<std::string>() == "a") {
      mode |= std::ofstream::app;
    }

    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
    std::ofstream jsfile(converter.from_bytes(path), mode);
    jsfile << data;
    jsfile.close();
    if (jsfile.fail()) {
      JS_ThrowInternalError(ctx, "write failed.");
      return JS_EXCEPTION;
    }

    return to_jsvalue(ctx, data.length());
  }
};

JSClassID JSFile::class_id;

const JSCFunctionListEntry JSFile::proto_funcs[] = {
    function_entry_getset<&GetPath>("path"),
    function_entry_getset<&IsWritable>("writable"),
    function_entry<&ReadFileSync>("read"),
    function_entry<&WriteFileSync>("write"),
};

JSValue NewFile(JSContext* ctx, const std::string& path, bool writable) {
  JSValue obj = JS_NewObjectClass(ctx, JSFile::class_id);
  if (JS_IsException(obj)) return obj;
  JS_SetOpaque(obj, new JSFile(path, writable));
  return obj;
}

static JSValue ReadFile(JSContext* ctx, JSValueConst this_val, int argc,
                        JSValueConst* argv) {
  if (argc < 1) {
    return JS_EXCEPTION;
  }
  ValueHolder path(ctx, argv[0]);
  if (!path.IsString()) {
    JS_ThrowTypeError(ctx, "invalid path");
    return JS_EXCEPTION;
  }

  std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;

  std::ifstream jsfile(converter.from_bytes(path.To<std::string>()));
  std::stringstream buffer;
  buffer << jsfile.rdbuf();
  jsfile.close();
  if (jsfile.fail()) {
    JS_ThrowInternalError(ctx, "read failed.");
    return JS_EXCEPTION;
  }
  return to_jsvalue(ctx, buffer.str());
}

static JSValue WriteFile(JSContext* ctx, JSValueConst this_val, int argc,
                         JSValueConst* argv) {
  if (argc < 2) {
    return JS_EXCEPTION;
  }
  ValueHolder path(ctx, argv[0]);
  ValueHolder data(ctx, argv[1]);
  if (!path.IsString()) {
    JS_ThrowTypeError(ctx, "invalid path");
    return JS_EXCEPTION;
  }

  int mode = std::ofstream::binary;
  if (argc > 2 && JS_IsObject(argv[2])) {
    ValueHolder options(ctx, argv[2]);
    if (options["flag"].To<std::string>() == "a") {
      mode |= std::ofstream::app;
    }
  }

  std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
  std::ofstream jsfile(converter.from_bytes(path.To<std::string>()), mode);
  jsfile << data.To<std::string>();
  jsfile.close();
  if (jsfile.fail()) {
    JS_ThrowInternalError(ctx, "write failed.");
    return JS_EXCEPTION;
  }
  return to_jsvalue(ctx, data.Length());
}

static JSValue OpenFile(JSContext* ctx, JSValueConst this_val, int argc,
                        JSValueConst* argv) {
  if (argc < 1) {
    return JS_EXCEPTION;
  }
  ValueHolder path(ctx, argv[0]);
  ValueHolder write(ctx, argv[1]);
  if (!path.IsString()) {
    JS_ThrowTypeError(ctx, "invalid path");
    return JS_EXCEPTION;
  }

  return NewFile(ctx, path.To<std::string>(), write.To<bool>());
}

const JSCFunctionListEntry fs_funcs[] = {
    function_entry("open", 2, OpenFile),
    function_entry("readFile", 2, ReadFile),
    function_entry("writeFile", 2, WriteFile),
};

static int FsModuleInit(JSContext* ctx, JSModuleDef* m) {
  NewClassProto<JSFile>(ctx, "File");
  return JS_SetModuleExportList(ctx, m, fs_funcs, COUNTOF(fs_funcs));
}

JSModuleDef* InitFsModule(JSContext* ctx) {
  JSModuleDef* m;
  m = JS_NewCModule(ctx, "fs", FsModuleInit);
  if (!m) {
    return NULL;
  }
  JS_AddModuleExportList(ctx, m, fs_funcs, COUNTOF(fs_funcs));
  return m;
}
