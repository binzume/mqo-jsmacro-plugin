#pragma once

#include "qjsutils.h"

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
  JsContext(JSRuntime *runtime,
            const std::vector<std::string> &argv = std::vector<std::string>()) {
    ctx = JS_NewContext(runtime);
    JS_SetContextOpaque(ctx, this);
  }
  ValueHolder ExecScript(const std::string &code,
                         const std::string &maybepath = "",
                         bool asmodule = true) {
    auto val = JS_Eval(ctx, code.c_str(), code.size(), maybepath.c_str(),
                       asmodule ? JS_EVAL_TYPE_MODULE : JS_EVAL_FLAG_STRICT);
    if (JS_IsException(val)) {
      dump_exception(ctx, val);
    }
    if (JS_PromiseState(ctx, val) == JS_PROMISE_FULFILLED) {
      auto promise = val;
      val = JS_PromiseResult(ctx, promise);
      JS_FreeValue(ctx, promise);
    }
    return ValueHolder(ctx, val);
  }

  ValueHolder GetGlobal() { return ValueHolder(ctx, JS_GetGlobalObject(ctx)); }
  void RegisterTimerImpl(JSValue f, int32_t time, uint32_t id = 0) {
    timers.push_back(TimerEntry{time, id, f});
  }
  void RemoveTimerImpl(uint32_t id) {
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

  static JSValue RegisterTimer(JSContext *ctx, JSValueConst this_val, int argc,
                               JSValueConst *argv) {
    JsContext *context = GetJsContext(ctx);
    if (context == nullptr) {
      return JS_EXCEPTION;
    }

    uint32_t timerMs = 10;
    if (argc > 1) {
      timerMs = convert_jsvalue<uint32_t>(ctx, argv[1]);
    }
    context->RegisterTimerImpl(JS_DupValue(ctx, argv[0]),
                               timerMs + GetTickCount(),
                               convert_jsvalue<uint32_t>(ctx, argv[2]));
    return JS_UNDEFINED;
  }

  static JSValue RemoveTimer(JSContext *ctx, int id) {
    JsContext *context = GetJsContext(ctx);
    if (context == nullptr) {
      return JS_EXCEPTION;
    }
    context->RemoveTimerImpl(id);
    return JS_UNDEFINED;
  }

 protected:
  static JsContext *GetJsContext(JSContext *ctx) {
    return (JsContext *)JS_GetContextOpaque(ctx);
  }
};
