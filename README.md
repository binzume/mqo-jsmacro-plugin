
# JavaScriptでメタセコイアのマクロを書くプラグイン

JavaScriptでメタセコイアのマクロを書けるようにするプラグインです．

**作りかけなのでまだ使い物にならないです．**

## インストール

`Plugins/Station` ディレクトリに `JSMacro.dll` と `JSMacro.dll.core.js` を置いて下さい．

DLLは64bit版です．(32ビット版が必要な場合はソースからビルドして下さい...)

## API

暫定仕様です．個々の関数の動作はメタセコイアSDKのドキュメントを参照してください．

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
- document.compact()
- console.log("message")

注意点

- `verts[index] は [x,y,z]` 形式の値を返します
- `verts[0] = [x,y,z]` は動作しますが `verts[0][0] = x` は変更が反映されません
- 頂点の削除は `delete verts[index];`
- 配列はundefinedの要素が存在する場合(削除操作の後など)があります．連続した配列にしたい場合は `compact()` メソッドを読んでください．

## TODO

- 面とマテリアルの編集
- UIまともに． スクリプトとショートカットキーなどの登録できるようにする

