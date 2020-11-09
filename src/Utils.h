#pragma once

#include <string>
#include <codecvt>

void debug_log(const std::string s, int tag = 1);
#define COUNTOF(array) (sizeof(array) / sizeof(array[0]))

// #define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
static std::string utf8ToSjis(const std::string& str) {
  std::locale sjis(".932", std::locale::ctype);
  typedef std::codecvt<wchar_t, char, std::mbstate_t> mbCvt;
  const mbCvt& cvt = std::use_facet<mbCvt>(sjis);
  std::wstring_convert<mbCvt, wchar_t> sjisConverter(&cvt);
  std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> utf8Converter;
  return sjisConverter.to_bytes(utf8Converter.from_bytes(str));
}

static std::string sjisToUtf8(const std::string& str) {
  std::locale sjis(".932", std::locale::ctype);
  typedef std::codecvt<wchar_t, char, std::mbstate_t> mbCvt;
  const mbCvt& cvt = std::use_facet<mbCvt>(sjis);
  std::wstring_convert<mbCvt, wchar_t> sjisConverter(&cvt);
  std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> utf8Converter;
  return utf8Converter.to_bytes(sjisConverter.from_bytes(str));
}
