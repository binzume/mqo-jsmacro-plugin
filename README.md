
# JavaScriptでメタセコイアのマクロを書くプラグイン

JavaScriptでメタセコイアのマクロを書けるようにするプラグインです．


## ダウンロード＆インストール

GitHub の `Releases` ページからダウンロードできます．
[`JSMacro.zip`](https://github.com/binzume/mqo-jsmacro-plugin/releases/latest) を展開し，
メタセコイアの `Plugins/Station` ディレクトリに `JSMacro.dll` と `JSMacro.dll.core.js` を配置して下さい．

現在ダウンロードできるDLLは**64bit版**のみです．(32ビット版は余裕があったら用意するかもしれませんが，必要な場合はソースからビルドして下さい...)

ソースからのプラグインのビルド方法 [doc/BUILD.md](doc/BUILD.md) を参照してください．

## 利用方法

「パネル」→「JavaScript」でウインドウが出ます．jsファイルを指定して実行できます．

![ss](doc/jsmacro.png)

- `...` ファイル選択ダイアログを開く
- `Run` 実行
- `Preset` スクリプトを8個まで登録できます．プラグインのサブコマンドとしてショートカットキーの割当が可能です．
- `Settings` エディタやログファイルの出力先を指定できます．

エディタとしての機能は持ってないので，お使いのテキストエディタで書いたjsファイルを指定して実行して下さい．
ファイル名の入力欄に `js:`から始まる文字列を入れて実行すると入力内容が実行されます `js:console.log("hello")` ．
最後に実行したスクリプトの変数などにアクセスできます(デバッグ用)．

## API

メタセコイアのプラグインから呼べる機能を JavaScript から扱いやすいようにラップしてあります．
個々の関数の動作は [メタセコイアSDK](https://www.metaseq.net/jp/download/sdk/) のドキュメントを参照してください．

#### 注意点

- なるべく通常のArrayと同じように扱えるようにしていますが，push()/pop()などは動作しません．
- 配列はundefinedの要素が存在する場合(削除操作の後など)があります．連続した配列にしたい場合は `compact()` メソッドを呼んでください．

### MQDocument

グローバルに`document`として宣言されています．

- document.objects.length オブジェクト数(ReadOnly)
- document.objects[index] MQObjectを取得
- document.objects.append(obj) オブジェクトを追加
- document.objects.remove(obj) オブジェクトを削除
- document.materials.length マテリアル数(ReadOnly)
- document.materials[index] MQMaterialを取得
- document.materials.append(mat) マテリアルを追加
- document.materials.remove(mat) マテリアルを削除
- document.scene シーンを取得
- document.compact()
- document.clearSelect()
- document.isVertexSelected(objIndex, vertIndex)
- document.setVertexSelected(objIndex, vertIndex, bool)
- document.isFaceSelected(objIndex, faceIndex)
- document.setFaceSelected(objIndex, faceIndex, bool)
- document.currentObjectIndex

### MQObject

- object.id ドキュメント内でユニークなID
- object.index ドキュメント内でのインデックス
- object.name オブジェクト名
- object.clone() 複製したオブジェクトを返します(trueを渡すとドキュメントに登録された状態で複製されます)
- object.compact() 使われていない要素を切り詰めます
- object.merge(srcObj) srcObjをobjectにマージします
- object.freeze(flag) 曲面・鏡面をフリーズします(flag省略時は全て)
- object.clear()
- object.optimizeVertex()
- object.verts.length (ReadOnly)
- object.verts[index]
- object.verts[index].id (ReadOnly)
- object.verts[index].refs (ReadOnly)
- object.verts.append(x,y,z) or append({x:X, y:Y, z:Z})
- object.faces.length (ReadOnly)
- object.faces[index]
- object.faces[index].invert()
- object.faces[index].points
- object.faces[index].uv
- object.faces[index].material
- object.faces.append([1,2,3,4], mat_index)
- object.selected
- object.visible
- object.locked
- object.type
- object.transform(matrix_or_fun) オブジェクトの全頂点座標を変換します

新しくオブジェクトを生成する場合は`new MQObject()` で作成し， `document.objects.append(obj)` でドキュメントに追加して下さい．
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

- `verts[index]` は `{x:0,y:1,z:2}` 形式の値のコピーを返します
- `verts[0] = {x:0,y:1,z:2}` は動作しますが `verts[0].x = 0` は変更が反映されません(faceやcolorなども同様)
- 頂点の削除は `delete verts[index];`

### MQMaterial

- material.id ドキュメント内でユニークなID
- material.index ドキュメント内でのインデックス
- material.name マテリアル名
- material.color 色
- material.ambientColor
- material.emissionColor
- material.specularColor
- material.power
- material.ambient
- material.emission
- material.specular
- material.reflection
- material.refraction
- material.doubleSided
- material.selected


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

アルファの値も一緒に扱います．設定する場合はアルファは省略可能です．

### MQScene

カメラ情報のみアクセスできます．

- document.scene.cameraPosition → `{x: X, y: Y, z: Z}`
- document.scene.cameraLookAt → `{x: X, y: Y, z: Z}`
- document.scene.cameraAngle → `{bank: B, head: H, pitch: Z}`
- document.scene.rotationCenter
- document.scene.zoom
- document.scene.fov

### MQMatrix

C++用のSDKに含まれるMQMatrixとは別物です． `.core.js` に実装されています．

`MQObject.transform()`に渡すことでオブジェクトの全頂点を簡単に変換できます．

例：

```js
document.objects[0].transform(MQMatrix.rotateMatrix(1,0,0, 15));
```

### その他

- console.log("message") メッセージをログに出力
- alert/prompt/confirm ダイアログ表示
- setInterval(), setTimeout() タイマー
- module.include(scriptPath) 別スクリプトの読み込み＆実行(仮実装)
- module.require(scriptPath) CommonJS形式のモジュール読み込み(仮実装)

カメラを回す例： (別のスクリプトを実行するまで停止しません)

```js
setInterval(() => {
	var originalLookAt = document.scene.cameraLookAt;
	document.scene.cameraPosition = MQMatrix.rotateMatrix(0,1,0, 1).transformV(document.scene.cameraPosition);
	document.scene.cameraLookAt = originalLookAt;
}, 10);
```


組み込みのモジュールがいくつかあります

- geom Vertex/Matrix(MQMatrixの実体)
- dialog 各種ダイアログ表示
- fs ファイルアクセス(実装中)
- child_process プロセス起動(実装中)

## TODO

- スクリプトからアクセス出来る属性を増やす
- メニュー操作自動化
- ファイルアクセス

# License

MIT License

また以下のライブラリに依存しています．

- https://github.com/c-smile/quickjspp
- https://bellard.org/quickjs/
