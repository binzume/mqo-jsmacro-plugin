

workspace "mqo-plugin"
	-- Premake output folder
	location(path.join("_build", _ACTION))

	platforms { "x64", "x86" } 
	configurations { "Debug", "Release" }

	defines {
  	  "JS_STRICT_NAN_BOXING", -- this option enables x64 build on Windows/MSVC
      "CONFIG_BIGNUM",
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
	includedirs { "mqsdk", "quickjs-msvc" }
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
		"quickjs-msvc/cutils.h",
		"quickjs-msvc/cutils.c",
		"quickjs-msvc/libregexp.c",
		"quickjs-msvc/libunicode.c",
		"quickjs-msvc/libbf.c",
		"quickjs-msvc/libregexp.h",
		"quickjs-msvc/libregexp-opcode.h",
		"quickjs-msvc/libunicode.h",
		"quickjs-msvc/libunicode-table.h",
		"quickjs-msvc/list.h",
		"quickjs-msvc/quickjs.c",
		"quickjs-msvc/quickjs.h",
		"quickjs-msvc/quickjs-atom.h",
		"quickjs-msvc/quickjs-opcode.h",
		"quickjs-msvc/quickjs-libc.c",
		"quickjs-msvc/quickjs-libc.h",
	}

newaction {
	trigger     = "downloadmqsdk",
	description = "Download mqsdk",
	execute = function ()
		local sdkver = "mqsdk483b"
		local result_str, response_code = http.download("https://www.metaseq.net/metaseq/" .. sdkver .. ".zip", "mqsdk/" .. sdkver .. ".zip")
		if (response_code ~= 200) then
			error("Failed to download Metasequoia SDK")
		end
		zip.extract("mqsdk/" .. sdkver .. ".zip", "mqsdk")
		os.execute("{COPY} mqsdk/mqsdk483b/mqsdk/* mqsdk/")
	end
}
