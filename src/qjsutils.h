#pragma once

#include <string>
#include <tuple>

#include "quickjs.h"

template <typename T>
static inline T convert_jsvalue(JSContext* ctx, JSValue v);
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

class ValueHolder {
  JSContext* ctx;
  JSValue value;

 public:
  // TODO: Dup
  ValueHolder& operator=(const ValueHolder&) = delete;

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
  template <typename TN>
  void Delete(TN name) {
    JSAtom prop = to_atom(name);
    JS_DeleteProperty(ctx, value, prop, JS_PROP_THROW);
    JS_FreeAtom(ctx, prop);
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

template <auto method>
JSValue method_wrapper(JSContext* ctx, JSValueConst this_val, int argc,
                       JSValueConst* argv) {
  return invoke_method(method, ctx, this_val, argc, argv);
}

template <auto method>
JSValue method_wrapper_bind(JSContext* ctx, JSValueConst this_val, int argc,
                            JSValueConst* argv, int magic, JSValue* func_data) {
  return invoke_method(method, ctx, func_data[0], argc, argv);
}

template <auto method>
JSValue method_wrapper_setter(JSContext* ctx, JSValueConst this_val,
                              JSValueConst arg) {
  return invoke_method(method, ctx, this_val, 1, &arg);
}

template <auto method>
JSValue method_wrapper_getter(JSContext* ctx, JSValueConst this_val) {
  return invoke_method(method, ctx, this_val, 0, nullptr);
}

template <typename R, typename T, typename... Args>
inline auto arg_count(R (T::*)(JSContext*, JSValueConst, int, JSValueConst*)) {
  return std::make_index_sequence<0>();
}
template <typename R, typename T, typename... Args>
inline auto arg_count(R (T::*)(JSContext*, Args...)) {
  return std::make_index_sequence<sizeof...(Args)>();
}
template <typename R, typename T, typename... Args>
inline auto arg_count(R (T::*)(Args...)) {
  return std::make_index_sequence<sizeof...(Args)>();
}

template <typename T, typename R, typename... Args>
inline JSValue invoke_method(R (T::*method)(Args...), JSContext* ctx,
                             JSValueConst this_val, int argc,
                             JSValueConst* argv) {
  T* p = (T*)JS_GetOpaque2(ctx, this_val, T::class_id);
  if (!p) {
    return JS_EXCEPTION;
  }
  const auto arg_seq = arg_count(method);
  if constexpr (std::is_same_v<R, void>) {
    invoke_method_impl(arg_seq, p, method, ctx, this_val, argc, argv);
    return JS_UNDEFINED;
  } else {
    return to_jsvalue(
        ctx, invoke_method_impl(arg_seq, p, method, ctx, this_val, argc, argv));
  }
}

template <typename T, typename R, std::size_t... N>
inline R invoke_method_impl(std::index_sequence<N...>, T* p,
                            R (T::*method)(JSContext*, JSValueConst, int,
                                           JSValueConst*),
                            JSContext* ctx, JSValueConst this_val, int argc,
                            JSValueConst* argv) {
  return std::invoke(method, p, ctx, this_val, argc, argv);
}
template <typename T, typename R, typename... Args, std::size_t... N>
inline R invoke_method_impl(std::index_sequence<N...>, T* p,
                            R (T::*method)(JSContext*, Args...), JSContext* ctx,
                            JSValueConst this_val, int argc,
                            JSValueConst* argv) {
  return std::invoke(method, p, ctx, convert_jsvalue<Args>(ctx, argv[N])...);
}
template <typename T, typename R, typename... Args, std::size_t... N>
inline R invoke_method_impl(std::index_sequence<N...>, T* p,
                            R (T::*method)(Args...), JSContext* ctx,
                            JSValueConst this_val, int argc,
                            JSValueConst* argv) {
  return std::invoke(method, p, convert_jsvalue<Args>(ctx, argv[N])...);
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
constexpr JSCFunctionListEntry function_entry(const char* name) {
  return function_entry(name, (uint8_t)arg_count(method).size(),
                        method_wrapper<method>);
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

template <auto method>
static int array_accessor(JSContext* ctx, JSPropertyDescriptor* desc,
                          JSValueConst obj, JSAtom prop) {
  typedef decltype(get_class(method)) T;
  JSValue propv = JS_AtomToValue(ctx, prop);
  // JS_AtomToValue and JS_ToIndex are too expensive...
  if ((prop & (1U << 31)) == 0) {
    return 0;
  }
  uint32_t index = (prop & ~(1U << 31));
  desc->flags = 0;
  desc->value =
      (((T*)JS_GetOpaque2(ctx, obj, T::class_id))->*method)(ctx, index);
  return 1;
}

template <auto getter, auto setter>
static int array_accessor2(JSContext* ctx, JSPropertyDescriptor* desc,
                           JSValueConst obj, JSAtom prop) {
  typedef decltype(get_class(getter)) T;
  JSValue propv = JS_AtomToValue(ctx, prop);
  // JS_AtomToValue and JS_ToIndex are too expensive...
  if ((prop & (1U << 31)) == 0) {
    return 0;
  }
  uint32_t index = (prop & ~(1U << 31));
  ((T*)JS_GetOpaque2(ctx, obj, T::class_id))->set_array_index(index);  // FIXME;
  desc->flags = JS_PROP_GETSET;
  desc->getter = JS_NewCFunction(ctx, method_wrapper<getter>, "getter", 0);
  desc->setter = JS_NewCFunction(ctx, method_wrapper<setter>, "setter", 1);
  return 1;
}

template <typename T>
static constexpr bool hasInit(...) {
  return false;
}
template <typename T>
static constexpr bool hasInit(
    int, decltype((std::declval<T>().Init(nullptr, JSValueConst(), int(),
                                          nullptr)))* = 0) {
  return true;
}

template <typename T>
static JSValue simple_constructor(JSContext* ctx, JSValueConst new_target,
                                  int argc, JSValueConst* argv) {
  JSRuntime* rt = JS_GetRuntime(ctx);
  JSValue proto = JS_GetClassProto(ctx, T::class_id);

  JSValue obj = JS_NewObjectProtoClass(ctx, proto, T::class_id);
  JS_FreeValue(ctx, proto);

  T* p = new T(ctx);
  JS_SetOpaque(obj, p);

  if constexpr (hasInit<T>(0)) {
    p->Init(ctx, obj, argc, argv);
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
