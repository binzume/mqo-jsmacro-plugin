
# JavaScriptでメタセコイアのマクロを書くプラグイン

JavaScriptでメタセコイアのマクロを書けるようにするプラグインです．

とりあえず動きますが，まだ開発中なので仕様は予告なしに変わる予定です．

**特に理由が無い限り標準のPythonスクリプトを使うのを推奨**


## インストール

[`JSMacro.zip`](https://github.com/binzume/mqo-jsmacro-plugin/raw/master/bin/JSMacro.zip) を展開し，
`Plugins/Station` ディレクトリに `JSMacro.dll` と `JSMacro.dll.core.js` を配置して下さい．

DLLは64bit版です．(32ビット版は用意する予定はないですが，必要な場合はソースからビルドして下さい...)

## 利用方法

「パネル」→「JS Macro」でウインドウが出ます．jsファイルを指定して実行できます．

## API

メタセコイアのプラグインから呼べる機能をJavaScriptから扱いやすいようにラップしてあります．
個々の関数の動作はメタセコイアSDKのドキュメントを参照してください．

- document.objects.length (ReadOnly)
- document.objects[index].id
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
- document.objects[index].faces.append([1,2,3,4], mat_index)
- document.objects[index].transform(matrix_or_fun)
- document.objects.append(obj)
- document.objects.remove(obj)
- document.materials.length (ReadOnly)
- document.materials[index].id
- document.materials[index].name
- document.materials[index].color
- document.objects.append(mat)
- document.compact()
- console.log("message")


#### 注意点

- なるべく通常のArrayと同じように扱えるようにしていますが，push()/pop()などは動作しません．
- 配列はundefinedの要素が存在する場合(削除操作の後など)があります．連続した配列にしたい場合は `compact()` メソッドを読んでください．

### MQObject

新しくオブジェクトを生成する場合は`new MQObject()` で作成し， append関数でドキュメントに追加して下さい．
コンストラクタの引数を省略した場合は，自動的に衝突しない名前が設定されます．

例：

```js
var square = new MQObject("square1");
square.verts.append(0, 0, 0);
square.verts.append(1, 0, 0);
square.verts.append(1, 1, 0);
square.verts.append(0, 1, 0);
square.faces.append([0, 1, 2, 3]);
document.objects.append(square);
```

#### MQObject.verts

- `verts[index] は [x,y,z]` 形式の値を返します
- `verts[0] = [x,y,z]` は動作しますが `verts[0][0] = x` は変更が反映されません
- 頂点の削除は `delete verts[index];`

### MQMaterial

新しくマテリアルを生成する場合は`new MQMaterial()` で作成し， append関数でドキュメントに追加して下さい．
コンストラクタの引数を省略した場合は，自動的に衝突しない名前が設定されます．

例：

```js
var red = new MQMaterial("red1");
red.color = {r:1.0, g:0.0, b:0.0};
var redIndex = document.materials.append(red);
```

#### MQMaterial.color

内容は `{r: red, g: green, b: blue, a: alpha}`.

アルファの値も一緒に帰ります．設定する場合はアルファの値は省略可能です．

### MQMatrix

C++用のSDKに含まれるMQMatrixとは別物です． core.jsに実装されていいます．

`MQObject.transform()`に渡すことでオブジェクトの全頂点を簡単に変換できます．

例：

```js
document.objects[0].transform(MQMatrix.rotateMatrix(1,0,0, 15));
```

## TODO

- UIまともに． スクリプトとショートカットキーなどの登録できるようにする
- スクリプトからアクセス出来る属性を増やす
- バックグラウンド処理

