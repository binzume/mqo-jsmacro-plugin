

workspace "mqo-plugin"
	-- Premake output folder
	location(path.join("_build", _ACTION))

	platforms { "x64", "x86" } 
	configurations { "Debug", "Release" }

	defines {
  	  "JS_STRICT_NAN_BOXING", -- this option enables x64 build on Windows/MSVC
      "CONFIG_BIGNUM",
      "CONFIG_JSX",           -- native JSX support - enables JSX literals
    } 

	filter "platforms:x86"
		architecture "x86"
	filter "platforms:x64"
		architecture "x86_64"  

	-- Debug
	filter { "configurations:Debug" }
		defines { "DEBUG" }
		symbols "On"
		optimize "Off"

	-- Release
	filter { "configurations:Release" }
		defines { "NDEBUG" }
		optimize "Speed"
		inlining "Auto"
		flags { staticruntime "On" }

	filter { "action:vs*" }
		buildoptions { "/std:c++latest" }
		systemversion "latest"
		defines { "_CRT_SECURE_NO_DEPRECATE", "_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING", "_WINDOWS" }
		characterset "MBCS"

	filter { }
		targetdir "%{cfg.location}/%{cfg.architecture}/%{cfg.buildcfg}/"

-----------------------------------------------------------------------------------------------------------------------

project "jsmacro-plugin"
	language "C++"
	kind "SharedLib"
	links { "mqsdk", "quickjs" }
	includedirs { "mqsdk", "quickjspp" }
	files {
		"src/*.cpp",
		"src/*.h",
	}

project "mqsdk"
	language "C++"
	kind "StaticLib"
	includedirs { "mqsdk" }
	files {
		"mqsdk/*.cpp",
		"mqsdk/*.h",
	}

project "quickjs"
	language "C"
	kind "StaticLib"
	exceptionhandling "Off"
	rtti "Off"
	files {
		"quickjspp/cutils.h",
		"quickjspp/cutils.c",
		"quickjspp/libregexp.c",
		"quickjspp/libunicode.c",
		"quickjspp/quickjs.c",
		"quickjspp/quickjs-libc.c",
		"quickjspp/libbf.c",
		"quickjspp/libregexp.h",
		"quickjspp/libregexp-opcode.h",
		"quickjspp/libunicode.h",
		"quickjspp/libunicode-table.h",
		"quickjspp/list.h",
		"quickjspp/quickjs.h",
		"quickjspp/quickjs-atom.h",
		"quickjspp/quickjs-libc.h",
		"quickjspp/quickjs-opcode.h",
		"quickjspp/quickjs-jsx.h",
	}
