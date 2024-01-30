#pragma once

#include <string>
#include <tuple>
#include <type_traits>

#include "quickjs.h"

template <typename R, typename T = void>
static inline R convert_jsvalue(JSContext* ctx, JSValue v);
template <>
static inline JSValue convert_jsvalue(JSContext* ctx, JSValue v) {
  return v;
}
template <>
static inline int32_t convert_jsvalue(JSContext* ctx, JSValue v) {
  int32_t ret = 0;
  JS_ToInt32(ctx, &ret, v);
  return ret;
}
template <>
static inline uint32_t convert_jsvalue(JSContext* ctx, JSValue v) {
  uint32_t ret = 0;
  JS_ToUint32(ctx, &ret, v);
  return ret;
}
template <>
static inline int64_t convert_jsvalue(JSContext* ctx, JSValue v) {
  int64_t ret = 0;
  JS_ToInt64(ctx, &ret, v);
  return ret;
}
template <>
static inline double convert_jsvalue(JSContext* ctx, JSValue v) {
  double ret = 0;
  JS_ToFloat64(ctx, &ret, v);
  return ret;
}
template <>
static inline float convert_jsvalue(JSContext* ctx, JSValue v) {
  double ret = 0;
  JS_ToFloat64(ctx, &ret, v);
  return (float)ret;
}
template <>
static inline bool convert_jsvalue(JSContext* ctx, JSValue v) {
  return JS_ToBool(ctx, v);
}
template <>
static inline std::string convert_jsvalue(JSContext* ctx, JSValue v) {
  const char* str = JS_ToCString(ctx, v);
  std::string s;
  if (str) {
    s = str;
    JS_FreeCString(ctx, str);
  }
  return s;
}

static inline JSValue to_jsvalue(JSContext* ctx, JSValue v) { return v; }
static inline JSValue to_jsvalue(JSContext* ctx, int32_t v) {
  return JS_NewInt32(ctx, v);
}
static inline JSValue to_jsvalue(JSContext* ctx, uint32_t v) {
  return JS_NewUint32(ctx, v);
}
static inline JSValue to_jsvalue(JSContext* ctx, int64_t v) {
  return JS_NewInt64(ctx, v);
}
static inline JSValue to_jsvalue(JSContext* ctx, double v) {
  return JS_NewFloat64(ctx, v);
}
static inline JSValue to_jsvalue(JSContext* ctx, bool v) {
  return JS_NewBool(ctx, v);
}
static inline JSValue to_jsvalue(JSContext* ctx, float v) {
  return JS_NewFloat64(ctx, v);
}
static inline JSValue to_jsvalue(JSContext* ctx, const char* v) {
  return JS_NewString(ctx, v);
}
static inline JSValue to_jsvalue(JSContext* ctx, const std::string& v) {
  return JS_NewString(ctx, v.c_str());
}
static inline JSValue to_jsvalue(JSContext* ctx, const std::u8string& v) {
  return JS_NewString(ctx, reinterpret_cast<const char*>(v.c_str()));
}

template <auto method>
JSValue method_wrapper(JSContext* ctx, JSValueConst this_val, int argc,
                       JSValueConst* argv) {
  return invoke_function(method, ctx, this_val, argc, argv);
}

template <auto method>
JSValue method_wrapper_bind(JSContext* ctx, JSValueConst this_val, int argc,
                            JSValueConst* argv, int magic, JSValue* func_data) {
  return invoke_function(method, ctx, func_data[0], argc, argv);
}

template <auto method>
JSValue method_wrapper_setter(JSContext* ctx, JSValueConst this_val,
                              JSValueConst arg) {
  return invoke_function(method, ctx, this_val, 1, &arg);
}

template <auto method>
JSValue method_wrapper_getter(JSContext* ctx, JSValueConst this_val) {
  return invoke_function(method, ctx, this_val, 0, nullptr);
}

template <auto method>
JSValue method_wrapper_append_data(JSContext* ctx, JSValueConst this_val,
                                   int argc, JSValueConst* argv, int magic,
                                   JSValue* func_data) {
  argv[argc] = *func_data;  // TODO: check length of function.
  return invoke_function(method, ctx, this_val, argc + 1, argv);
}

// function/method type.
template <typename... Types>
struct types {
  static constexpr int size() { return sizeof...(Types); }
};
template <typename T>
struct arg_info;
template <typename R, typename T, typename... Args>
struct arg_info<R (T::*)(Args...)> {
  using prefix = typename types<R, T*>;
  using extra = typename types<Args...>;
};
template <typename R, typename T, typename... Args>
struct arg_info<R (T::*)(JSContext*, Args...)> {
  using prefix = typename types<R, T*, JSContext*>;
  using extra = typename types<Args...>;
};
template <typename R, typename T, typename... Args>
struct arg_info<R (T::*)(JSContext*, JSValueConst, int, JSValueConst*,
                         Args...)> {
  using prefix =
      typename types<R, T*, JSContext*, JSValueConst, int, JSValueConst*>;
  using extra = typename types<Args...>;
};
template <typename R, typename... Args>
struct arg_info<R (*)(Args...)> {
  using prefix = typename types<R>;
  using extra = typename types<Args...>;
};
template <typename R, typename... Args>
struct arg_info<R (*)(JSContext*, Args...)> {
  using prefix = typename types<R, JSContext*>;
  using extra = typename types<Args...>;
};
template <typename R, typename... Args>
struct arg_info<R (*)(JSContext*, JSValueConst, int, JSValueConst*, Args...)> {
  using prefix =
      typename types<R, JSContext*, JSValueConst, int, JSValueConst*>;
  using extra = typename types<Args...>;
};

template <typename T, typename R, typename... Args>
inline JSValue invoke_function(R (T::*method)(Args...), JSContext* ctx,
                               JSValueConst this_val, int argc,
                               JSValueConst* argv) {
  T* p = (T*)JS_GetOpaque2(ctx, this_val, T::class_id);
  if (!p) {
    return JS_EXCEPTION;
  }
  using arg = arg_info<decltype(method)>;
  std::tuple prefix(ctx, this_val, argc, argv, p);
  return invoke_function_impl(arg::prefix(), arg::extra(),
                              std::make_index_sequence<arg::extra::size()>(),
                              prefix, method, ctx, argv);
}

template <typename R, typename... Args>
inline JSValue invoke_function(R (*method)(Args...), JSContext* ctx,
                               JSValueConst this_val, int argc,
                               JSValueConst* argv) {
  using arg = arg_info<decltype(method)>;
  std::tuple prefix(ctx, this_val, argc, argv);
  return invoke_function_impl(arg::prefix(), arg::extra(),
                              std::make_index_sequence<arg::extra::size()>(),
                              prefix, method, ctx, argv);
}

template <typename T, typename F, typename R, typename... Pre, std::size_t... N,
          typename... Args>
inline JSValue invoke_function_impl(types<R, Pre...>, types<Args...>,
                                    std::index_sequence<N...>, const T& p, F f,
                                    JSContext* ctx, JSValueConst* argv) {
  if constexpr (std::is_void<R>()) {
    std::invoke(
        f, std::get<Pre>(p)...,
        std::forward<std::remove_cvref_t<Args>>(
            convert_jsvalue<std::remove_cvref_t<Args>>(ctx, argv[N]))...);
    return JS_UNDEFINED;
  } else {
    return to_jsvalue(
        ctx, std::invoke(f, std::get<Pre>(p)...,
                         std::forward<std::remove_cvref_t<Args>>(
                             convert_jsvalue<std::remove_cvref_t<Args>>(
                                 ctx, argv[N]))...));
  }
}

constexpr JSCFunctionListEntry function_entry(const char* name, uint8_t length,
                                              JSCFunction func1) {
  return JSCFunctionListEntry{
      .name = name,
      .prop_flags = JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE,
      .def_type = JS_DEF_CFUNC,
      .magic = 0,
      .u = {.func = {length, JS_CFUNC_generic, {.generic = func1}}}};
}

template <auto method>
constexpr JSCFunctionListEntry function_entry(
    const char* name, uint8_t len = arg_info<decltype(method)>::extra::size()) {
  return function_entry(name, len, method_wrapper<method>);
}

constexpr JSCFunctionListEntry function_entry_getset(
    const char* name, JSValue (*getter)(JSContext* ctx, JSValueConst this_val),
    JSValue (*setter)(JSContext* ctx, JSValueConst this_val,
                      JSValueConst val) = nullptr) {
  return JSCFunctionListEntry{
      .name = name,
      .prop_flags = JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE,
      .def_type = JS_DEF_CGETSET,
      .magic = 0,
      .u = {.getset = {.get = {.getter = getter}, .set = {.setter = setter}}}};
}

template <auto getter, auto setter>
constexpr JSCFunctionListEntry function_entry_getset(const char* name) {
  return function_entry_getset(name, method_wrapper_getter<getter>,
                               method_wrapper_setter<setter>);
}

template <auto getter>
constexpr JSCFunctionListEntry function_entry_getset(const char* name) {
  return function_entry_getset(name, method_wrapper_getter<getter>);
}

template <typename T, typename... Args>
T get_class(JSValue (T::*method)(Args...)) {}

template <auto getter>
static int indexed_propery_getter(JSContext* ctx, JSPropertyDescriptor* desc,
                                  JSValueConst obj, JSAtom prop) {
  typedef decltype(get_class(getter)) T;
  JSValue propv = JS_AtomToValue(ctx, prop);
  // JS_AtomToValue and JS_ToIndex are too expensive...
  if ((prop & (1U << 31)) == 0) {
    return 0;
  }
  uint32_t index = (prop & ~(1U << 31));
  desc->flags = 0;
  desc->value =
      std::invoke(getter, (T*)JS_GetOpaque2(ctx, obj, T::class_id), ctx, index);
  return 1;
}

template <auto getter, auto setter>
static int indexed_propery_handler(JSContext* ctx, JSPropertyDescriptor* desc,
                                   JSValueConst obj, JSAtom prop) {
  typedef decltype(get_class(getter)) T;
  // JSValue propv = JS_AtomToValue(ctx, prop);
  // JS_AtomToValue and JS_ToIndex are too expensive...
  if ((prop & (1U << 31)) == 0) {
    return 0;
  }
  static JSValue indexValue = JS_UNDEFINED;  // TODO: multi-tread
  if (desc != nullptr) {
    JS_FreeValue(ctx, indexValue);
    indexValue = JS_NewInt32(ctx, (prop & ~(1U << 31)));
    desc->flags = JS_PROP_GETSET;
    desc->getter = JS_NewCFunctionData(ctx, method_wrapper_append_data<getter>,
                                       1, 0, 1, &indexValue);
    desc->setter = JS_NewCFunctionData(ctx, method_wrapper_append_data<setter>,
                                       2, 0, 1, &indexValue);
  }
  return 1;
}

template <typename, typename = std::void_t<>>
struct has_init : std::false_type {};
template <typename T>
struct has_init<T, std::void_t<decltype(&T::Init)>> : std::true_type {};

template <typename T>
inline auto instantiate(JSContext* ctx, JSValueConst new_target, int argc,
                        JSValueConst* argv) -> decltype(new T()) {
  return new T();
}
template <typename T>
inline auto instantiate(JSContext* ctx, JSValueConst new_target, int argc,
                        JSValueConst* argv) -> decltype(new T(ctx)) {
  return new T(ctx);
}
template <typename T>
inline auto instantiate(JSContext* ctx, JSValueConst new_target, int argc,
                        JSValueConst* argv)
    -> decltype(new T(ctx, new_target, argc, argv)) {
  return new T(ctx, new_target, argc, argv);
}

template <typename T>
static JSValue simple_constructor(JSContext* ctx, JSValueConst new_target,
                                  int argc, JSValueConst* argv) {
  JSRuntime* rt = JS_GetRuntime(ctx);
  JSValue proto = JS_GetClassProto(ctx, T::class_id);

  JSValue obj = JS_NewObjectProtoClass(ctx, proto, T::class_id);
  JS_FreeValue(ctx, proto);

  T* p = instantiate<T>(ctx, obj, argc, argv);
  JS_SetOpaque(obj, p);

  if constexpr (has_init<T>::value) {
    invoke_function(&T::Init, ctx, obj, argc, argv);
  }

  return obj;
}

template <typename T>
void simple_finalizer(JSRuntime* rt, JSValue val) {
  T* s = (T*)JS_GetOpaque(val, T::class_id);
  if (s) {
    delete s;
  }
}

class ValueHolder {
  JSValue value;
  friend JSValue unwrap(ValueHolder&& v);

 public:
  JSContext* ctx;
  // TODO: Dup
  ValueHolder& operator=(const ValueHolder&) = delete;
  ValueHolder& operator=(ValueHolder&& v) noexcept {
    JS_FreeValue(ctx, value);
    value = v.value;
    v.value = JS_UNDEFINED;
    return *this;
  }

  ValueHolder(JSContext* ctx, JSValue _value, bool dup = false)
      : value(_value), ctx(ctx) {
    if (dup) JS_DupValue(ctx, value);
  }
  ValueHolder(JSContext* ctx) : value(JS_NewObject(ctx)), ctx(ctx) {}
  ValueHolder(ValueHolder&& v) noexcept : value(v.value), ctx(v.ctx) {
    v.value = JS_UNDEFINED;
  }
  ValueHolder(const ValueHolder& v) : value(v.value), ctx(v.ctx) {
    JS_DupValue(ctx, value);
  }
  ~ValueHolder() { JS_FreeValue(ctx, value); }
  JSValue GetValue() {
    JS_DupValue(ctx, value);
    return value;
  }
  JSValue GetValueNoDup() { return value; }
  bool IsString() { return JS_IsString(value); }
  bool IsObject() { return JS_IsObject(value); }
  bool IsArray() { return JS_IsArray(ctx, value); }
  bool IsUndefined() { return JS_IsUndefined(value); }
  bool IsException() { return JS_IsException(value); }
  bool IsFunction() { return JS_IsFunction(ctx, value); }
  uint32_t Length() { return (*this)["length"].To<uint32_t>(); }
  template <typename T>
  T To() {
    return convert_jsvalue<T>(ctx, value);
  }
  ValueHolder operator[](uint32_t index) {
    return ValueHolder(ctx, JS_GetPropertyUint32(ctx, value, index));
  }
  ValueHolder operator[](const char* name) {
    return ValueHolder(ctx, JS_GetPropertyStr(ctx, value, name));
  }
  template <typename TN>
  void SetFree(TN name, JSValue v) {
    JSAtom prop = to_atom(name);
    JS_SetProperty(ctx, value, prop, v);
    JS_FreeAtom(ctx, prop);
  }
  template <typename TN, typename TV>
  void Set(TN name, TV v) {
    JSAtom prop = to_atom(name);
    JS_SetProperty(ctx, value, prop, to_value(v));
    JS_FreeAtom(ctx, prop);
  }
  template <typename TN, typename TV>
  void Define(TN name, TV v, int flags = JS_PROP_HAS_WRITABLE) {
    JSAtom prop = to_atom(name);
    JS_DefinePropertyValue(ctx, value, prop, to_value(v),
                           flags | JS_PROP_THROW);
    JS_FreeAtom(ctx, prop);
  }
  template <typename TN>
  void Delete(TN name) {
    JSAtom prop = to_atom(name);
    JS_DeleteProperty(ctx, value, prop, JS_PROP_THROW);
    JS_FreeAtom(ctx, prop);
  }
  template <typename TN>
  bool Has(TN name) {
    JSAtom prop = to_atom(name);
    bool ret = JS_HasProperty(ctx, value, prop) == 1;
    JS_FreeAtom(ctx, prop);
    return ret;
  }
  void swap(ValueHolder& v) {
    JSValue t = v.value;
    value = v.value;
    v.value = t;
  }
  void swap(ValueHolder&& v) {
    JSValue t = v.value;
    value = v.value;
    v.value = t;
  }

 private:
  template <typename T>
  JSValue to_value(T v) {
    return to_jsvalue(ctx, v);
  }
  JSValue to_value(ValueHolder& v) { return v.GetValue(); }
  JSAtom to_atom(const std::string& v) { return JS_NewAtom(ctx, v.c_str()); }
  JSAtom to_atom(const char* v) { return JS_NewAtom(ctx, v); }
  JSAtom to_atom(uint32_t v) { return JS_NewAtomUInt32(ctx, v); }
};

inline JSValue unwrap(ValueHolder&& v) {
  JSValue value = v.value;
  v.value = JS_UNDEFINED;
  return value;
}

template <typename T>
class JSClassBase {
 public:
  static JSClassID class_id;
  static T* Unwrap(JSValueConst v) { return (T*)JS_GetOpaque(v, class_id); }
  static T* Unwrap(JSContext* ctx, JSValueConst v) {
    return (T*)JS_GetOpaque2(ctx, v, class_id);
  }

  static JSValue NewClassProto(JSContext* ctx, const char* className,
                               JSClassExoticMethods* exotic = nullptr) {
    return ::NewClassProto<T>(ctx, className, exotic);
  }
};

template <typename T>
JSClassID JSClassBase<T>::class_id = 0;

template <typename R, typename T = std::remove_pointer<R>::type>
requires std::derived_from<T, JSClassBase<T>> static inline R convert_jsvalue(
    JSContext* ctx, JSValue v) {
  return T::Unwrap(ctx, v);
}

template <typename T>
inline JSValue NewClassProto(JSContext* ctx, const char* name,
                             JSClassExoticMethods* exotic = nullptr) {
  JSClassDef classdef = {
      .class_name = name,
      .finalizer = simple_finalizer<T>,
      .exotic = exotic,
  };
  JS_NewClassID(&T::class_id);
  JS_NewClass(JS_GetRuntime(ctx), T::class_id, &classdef);

  JSValue proto = JS_NewObject(ctx);
  JS_SetPropertyFunctionList(
      ctx, proto, T::proto_funcs,
      sizeof(T::proto_funcs) / sizeof(JSCFunctionListEntry));
  JS_SetClassProto(ctx, T::class_id, proto);
  return proto;
}

template <typename T>
inline JSValue newClassConstructor(JSContext* ctx, const char* name,
                                   JSClassExoticMethods* exotic = nullptr) {
  JSValue proto = NewClassProto<T>(ctx, name);
  JSValue constructor = JS_NewCFunction2(ctx, simple_constructor<T>, name, 1,
                                         JS_CFUNC_constructor, 0);
  JS_SetConstructor(ctx, constructor, proto);
  return constructor;
}
