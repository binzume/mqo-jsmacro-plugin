
# JavaScriptでメタセコイアのマクロを書くプラグイン

JavaScriptでメタセコイアのマクロを書けるようにするプラグインです．

**特に理由が無いなら標準のPythonスクリプトを使うのを推奨します．**


## インストール

[`JSMacro.zip`](https://github.com/binzume/mqo-jsmacro-plugin/raw/master/bin/JSMacro.zip) を展開し，
`Plugins/Station` ディレクトリに `JSMacro.dll` と `JSMacro.dll.core.js` を配置して下さい．

DLLは64bit版です．(32ビット版が必要な場合はソースからビルドして下さい...)

## 利用方法

「パネル」→「JS Macro」でウインドウが出ます．jsファイルを指定して実行できます．

## API

実装中なので暫定仕様です．個々の関数の動作はメタセコイアSDKのドキュメントを参照してください．

- document.objects.length (ReadOnly)
- document.objects[index].name
- document.objects[index].clone()
- document.objects[index].compact()
- document.objects[index].merge(srcObj)
- document.objects[index].freeze(flag)
- document.objects[index].verts.length (ReadOnly)
- document.objects[index].verts[index]
- document.objects[index].verts[index].refs (ReadOnly)
- document.objects[index].verts.append(x,y,z)
- document.objects[index].faces.length (ReadOnly)
- document.objects[index].faces[index]
- document.objects[index].faces[index].invert()
- document.objects[index].faces[index].points
- document.objects[index].faces[index].material
- document.objects[index].faces.append([1,2,3,4])
- document.objects[index].transform(matrix_or_fun)
- document.objects.append(obj)
- document.materials.length (ReadOnly)
- document.materials[index].name
- document.materials[index].color
- document.compact()
- console.log("message")


注意点

- `verts[index] は [x,y,z]` 形式の値を返します
- `verts[0] = [x,y,z]` は動作しますが `verts[0][0] = x` は変更が反映されません
- 頂点の削除は `delete verts[index];`
- なるべくArrayのように振る舞うようにしてますが，Arrayではないのでpush()/pop()などは出来ません．
- 配列はundefinedの要素が存在する場合(削除操作の後など)があります．連続した配列にしたい場合は `compact()` メソッドを読んでください．

### MQObject

新しくオブジェクトを作る例：

```js
var square = new MQObject("square1");
square.verts.append(0, 0, 0);
square.verts.append(1, 0, 0);
square.verts.append(1, 1, 0);
square.verts.append(0, 1, 0);
square.faces.append([0, 1, 2, 3]);
document.objects.append(square);
```

### MQMatrix

C++用のSDKに含まれるMQMatrixとは別物です． core.jsに実装されていいます．

`object.transform()`に渡すことでオブジェクトの全頂点を簡単に変換できます．

例：

```js
document.objects[0].transform(MQMatrix.rotateMatrix(1,0,0, 15));
```

## TODO

- 面とマテリアルの編集
- UIまともに． スクリプトとショートカットキーなどの登録できるようにする

