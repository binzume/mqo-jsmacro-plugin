#define WIN32_LEAN_AND_MEAN
#include <vector>
#include <array>
#include <sstream>
#include <codecvt>
#include <windows.h>

#include "Utils.h"
#include "qjsutils.h"

//---------------------------------------------------------------------------------------------------------------------
// Shell
//---------------------------------------------------------------------------------------------------------------------

static JSValue ExecSync(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {

  if (argc < 1) {
	return JS_EXCEPTION;
  }
  ValueHolder cmd(ctx, argv[0]);
  if (!cmd.IsString()) {
	JS_ThrowTypeError(ctx, "invalid path");
	return JS_EXCEPTION;
  }

	SECURITY_ATTRIBUTES	sa = { 0 };
	HANDLE read, write;
	sa.nLength = sizeof(sa);
	sa.bInheritHandle = TRUE;

	if (!CreatePipe(&read, &write, &sa, 0)) {
	  JS_ThrowInternalError(ctx, "CreatePipe error");
		return JS_EXCEPTION;
	}

	STARTUPINFO si = { 0 };
	PROCESS_INFORMATION pi = { 0 };
	si.cb = sizeof(STARTUPINFO);
	si.dwFlags = STARTF_USESTDHANDLES;
	si.hStdOutput = write;
	si.hStdError = write;
	if (!CreateProcess(NULL, const_cast<LPSTR>(cmd.to<std::string>().c_str()), NULL, NULL, TRUE, DETACHED_PROCESS, NULL, NULL, &si, &pi)) {
	  JS_ThrowInternalError(ctx, "CreateProcess error");
	  return JS_EXCEPTION;
	}

	if (WaitForSingleObject(pi.hProcess, INFINITE) != WAIT_OBJECT_0) {
	  JS_ThrowInternalError(ctx, "WaitForSingleObject error");
	  return JS_EXCEPTION;
	}

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	CloseHandle(write);

	std::string result;
	std::array<unsigned char, 1024> buf = { 0 };
	DWORD rlen = 0;
	while (ReadFile(read, buf.data(), (DWORD)buf.size(), &rlen, NULL)) {
		std::copy(buf.begin(), buf.begin() + rlen, std::back_inserter(result));
	}

	CloseHandle(read);

	return to_jsvalue(ctx, result);
}


static JSValue Exec(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    if (argc < 1) {
    	return JS_EXCEPTION;
    }
    ValueHolder cmd(ctx, argv[0]);
	if (!cmd.IsString()) {
	  JS_ThrowTypeError(ctx, "invalid path");
	  return JS_EXCEPTION;
	}

	SECURITY_ATTRIBUTES	sa = { 0 };
	HANDLE read, write;
	sa.nLength = sizeof(sa);
	sa.bInheritHandle = TRUE;

	if (!CreatePipe(&read, &write, &sa, 0)) {
	  JS_ThrowInternalError(ctx, "CreatePipe error");
	  return JS_EXCEPTION;
	}

	STARTUPINFO si = { 0 };
	PROCESS_INFORMATION pi = { 0 };
	si.cb = sizeof(STARTUPINFO);
	si.hStdOutput = write;
	if (!CreateProcess(NULL, const_cast<LPSTR>(cmd.to<std::string>().c_str()), NULL, NULL, TRUE, DETACHED_PROCESS, NULL, NULL, &si, &pi)) {
	  JS_ThrowInternalError(ctx, "CreateProcess error");
		return JS_EXCEPTION;
	}
	// TODO
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	CloseHandle(write);
	return to_jsvalue(ctx, (int)pi.dwProcessId);
}

const JSCFunctionListEntry process_funcs[] = {
	function_entry("execSync", 2, ExecSync),
	function_entry("exec", 2, Exec),
};

static int ProcessModuleInit(JSContext* ctx, JSModuleDef* m)
{
  return JS_SetModuleExportList(ctx, m, process_funcs, COUNTOF(process_funcs));
}

JSModuleDef* InitChildProcessModule(JSContext* ctx) {
  JSModuleDef* m;
  m = JS_NewCModule(ctx, "child_process", ProcessModuleInit);
  if (!m) {
	return NULL;
  }
  JS_AddModuleExportList(ctx, m, process_funcs, COUNTOF(process_funcs));
  return m;
}
