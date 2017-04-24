#pragma once

#include <string>

//---------------------------------------------------------------------------------------------------------------------
// Utils
//---------------------------------------------------------------------------------------------------------------------
#define UTF8(s) v8::String::NewFromUtf8(isolate, s, v8::NewStringType::kNormal).ToLocalChecked()
#define CHECK_TYPE_OR_RETURN(O, NAME) if (!(O)->IsObject() || !(O)->GetConstructorName()->Equals(UTF8(NAME))) {\
		isolate->ThrowException(Exception::TypeError(UTF8("not " NAME)));\
		return;\
	}

void debug_log(const std::string s, int tag = 1);
