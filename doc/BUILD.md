
# Build

## requirements

- Visual Studio 2019
- premake5 https://premake.github.io/download.html
- mqsdk https://www.metaseq.net/jp/download/sdk/

## build

- git clone .../mqo-jsmacro-plugin
- cd mqo-jsmacro-plugin
- (copy mqsdk into mqsdk folder.)
- git submodule init
- premake5 vs2019

`_build` ディレクトリの下に Visual Studio のソリューションとプロジェクトが出力されます．

以下のようなディレクトリ構成になっていればOKです．

```
- mqo-jsmacro-plugin
  - src
     - JSMacroPlugin.cpp
     - JSDocmentWrapper.cpp
     ...
  - mqsdk
     - MQPlugin.h
     ...
  - quickjspp
     - quickjs.c
     - quickjs.h
     ...
  - samples
     ...
  - _build/vs2019/mqo-plugin.sln (generated solution file)
```
