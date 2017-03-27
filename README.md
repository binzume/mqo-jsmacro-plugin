
# JavaScriptでメタセコイアのマクロを書くプラグイン

JavaScriptでメタセコイアのマクロを書けるようにするプラグインです．

**作りかけなのでまだ使い物にならないです．**

## インストール

`Plugins/Station` ディレクトリに `JSMacro.dll` と `JSMacro.dll.core.js` を置いて下さい．

## API

暫定仕様です．ここの関数の動作はメタセコイアSDKのドキュメントを参照してください．

- document.objects.length (ReadOnly)
- document.objects[index].name
- document.objects[index].clone()
- document.objects[index].compact()
- document.objects[index].merge(srcObj)
- document.objects[index].freeze(flag)
- document.objects[index].verts.length (ReadOnly)
- document.objects[index].verts[index]
- document.objects[index].verts.append(x,y,z)
- document.objects[index].faces.length (ReadOnly)
- document.objects[index].faces[index]
- document.objects[index].faces[index].invert()
- document.objects[index].faces[index].points
- document.objects[index].faces[index].material
- document.objects[index].faces.append([1,2,3,4])
- document.materials[]
- document.addObject(obj)
- document.createObject()
- console.log("message")


- `verts[index] は [x,y,z]` 形式の値を返します
- `verts[0] = [x,y,z]` は動作しますが `verts[0][0] = x` は変更が反映されません
- 頂点の削除は `delete verts[index];`

## TODO

- 面とマテリアルの編集
- C:/tmp/test.js とかを読みに行くの直す

