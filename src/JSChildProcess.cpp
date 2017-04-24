#define WIN32_LEAN_AND_MEAN
#include <vector>
#include <array>
#include <sstream>
#include <codecvt>
#include "libplatform/libplatform.h"
#include "v8.h"
#include "Utils.h"
#include <windows.h>


//---------------------------------------------------------------------------------------------------------------------
// Shell
//---------------------------------------------------------------------------------------------------------------------

using namespace v8;

static void ExecSync(const FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = args.GetIsolate();

	if (!args[0]->IsString()) {
		isolate->ThrowException(Exception::TypeError(UTF8("invalid path")));
		return;
	}
	String::Utf8Value cmd(args[0].As<String>());

	SECURITY_ATTRIBUTES	sa = { 0 };
	HANDLE read, write;
	sa.nLength = sizeof(sa);
	sa.bInheritHandle = TRUE;

	if (!CreatePipe(&read, &write, &sa, 0)) {
		isolate->ThrowException(Exception::Error(UTF8("CreatePipe error")));
		return;
	}

	STARTUPINFO si = { 0 };
	PROCESS_INFORMATION pi = { 0 };
	si.cb = sizeof(STARTUPINFO);
	si.dwFlags = STARTF_USESTDHANDLES;
	si.hStdOutput = write;
	si.hStdError = write;
	if (!CreateProcess(NULL, *cmd, NULL, NULL, TRUE, DETACHED_PROCESS, NULL, NULL, &si, &pi)) {
		isolate->ThrowException(Exception::Error(UTF8("CreateProcess error")));
		return;
	}

	if (WaitForSingleObject(pi.hProcess, INFINITE) != WAIT_OBJECT_0) {
		isolate->ThrowException(Exception::Error(UTF8("WaitForSingleObject error")));
		return;
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

	args.GetReturnValue().Set(UTF8(result.c_str()));
}


static void Exec(const FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = args.GetIsolate();

	if (!args[0]->IsString()) {
		isolate->ThrowException(Exception::TypeError(UTF8("invalid path")));
		return;
	}
	String::Utf8Value cmd(args[0].As<String>());

	SECURITY_ATTRIBUTES	sa = { 0 };
	HANDLE read, write;
	sa.nLength = sizeof(sa);
	sa.bInheritHandle = TRUE;

	if (!CreatePipe(&read, &write, &sa, 0)) {
		isolate->ThrowException(Exception::Error(UTF8("CreatePipe error")));
		return;
	}

	STARTUPINFO si = { 0 };
	PROCESS_INFORMATION pi = { 0 };
	si.cb = sizeof(STARTUPINFO);
	si.hStdOutput = write;
	if (!CreateProcess(NULL, *cmd, NULL, NULL, TRUE, DETACHED_PROCESS, NULL, NULL, &si, &pi)) {
		isolate->ThrowException(Exception::Error(UTF8("CreateProcess error")));
		return;
	}

	// TODO
	args.GetReturnValue().Set((int)pi.dwProcessId);
}

Local<ObjectTemplate> ChildProcessTemplate(Isolate* isolate) {
	auto t = ObjectTemplate::New(isolate);
	t->Set(UTF8("execSync"), FunctionTemplate::New(isolate, ExecSync));
	t->Set(UTF8("exec"), FunctionTemplate::New(isolate, Exec));
	return t;
}
