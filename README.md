
# JavaScriptでメタセコイアのマクロを書くプラグイン

JavaScriptでメタセコイアのマクロを書けるようにするプラグインです．

## ダウンロード＆インストール

GitHub の `Releases` ページからダウンロードできます．
[`JSMacro.zip`](https://github.com/binzume/mqo-jsmacro-plugin/releases/latest) を展開し，
メタセコイアの `Plugins/Station` ディレクトリに `JSMacro.dll` と `JSMacro.dll.core.js` を配置して下さい．

現在ダウンロードできるDLLは**64bit Windows版**のみです．(32ビット版は余裕があったら用意するかもしれませんが，必要な場合はソースからビルドして下さい...)

ソースからのプラグインのビルド方法 [doc/BUILD.md](doc/BUILD.md) を参照してください．

## 利用方法

「パネル」→「JavaScript」でプラグインのウインドウが出ます．jsファイルを指定して実行できます．

![ss](doc/jsmacro.png)

- `...` ファイル選択ダイアログを開く
- `Run` 実行
- `Preset` スクリプトを8個まで登録できます．プラグインのサブコマンドとしてショートカットキーの割当が可能です．
- `Settings` エディタやログファイルの出力先を指定できます．

エディタとしての機能は持ってないので，お使いのテキストエディタで書いたjsファイルを指定して実行して下さい．
ファイル名の入力欄に `js:`から始まる文字列を入れて実行すると入力内容が実行されます `js:console.log("hello")` ．
最後に実行したスクリプトの変数などにアクセスできます(デバッグ用)．

## API

- メタセコイアのプラグイン向けのAPIを JavaScript から扱いやすいようにラップしてあります．
- 利用可能なモジュールやメソッドの一覧や型定義は [mq_plugin.d.ts](scripts/mq_plugin.d.ts) を参照してください．
- 個々の関数の動作は [メタセコイアSDK](https://www.metaseq.net/jp/download/sdk/) のドキュメントを参照してください．

### MQDocument

グローバルに`mqdocument`として宣言されています．(互換性のため `document` でも参照できますが `mqdocument` の方を使用してください)

- mqdocument.objects.length オブジェクト数(ReadOnly)
- mqdocument.objects[index] MQObjectを取得
- mqdocument.objects.append(obj) オブジェクトを追加
- mqdocument.objects.remove(obj) オブジェクトを削除
- mqdocument.materials.length マテリアル数(ReadOnly)
- mqdocument.materials[index] MQMaterialを取得
- mqdocument.materials.append(mat) マテリアルを追加
- mqdocument.materials.remove(mat) マテリアルを削除
- mqdocument.scene シーンを取得
- mqdocument.compact()
- mqdocument.clearSelect()
- mqdocument.isVertexSelected(objIndex, vertIndex)
- mqdocument.setVertexSelected(objIndex, vertIndex, bool)
- mqdocument.isFaceSelected(objIndex, faceIndex)
- mqdocument.setFaceSelected(objIndex, faceIndex, bool)
- mqdocument.currentObjectIndex

objects や materials 等はArrayLikeなオブジェクトを返しますが，remove/append以外のメソッドではドキュメントの内容を変更できません．
配列はundefinedの要素が存在する場合(削除操作の後など)があります．連続した配列にしたい場合は `compact()` メソッドを呼んでください．

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
- object.applyTransform(matrix_or_fun) オブジェクトの全頂点座標を変換します

新しくオブジェクトを生成する場合は`new MQObject()` で作成し， `mqdocument.objects.append(obj)` でドキュメントに追加して下さい．
コンストラクタの引数を省略した場合は，自動的に衝突しない名前が設定されます．

例：

```js
let square = new MQObject("square1");
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


新しくマテリアルを生成する場合は`new MQMaterial()` で作成し， materials.append() 関数でドキュメントに追加して下さい．
コンストラクタの引数を省略した場合は，自動的に衝突しない名前が設定されます．

例：

```js
var red = new MQMaterial("red1");
red.color = {r:1.0, g:0.0, b:0.0};
var redIndex = mqdocument.materials.append(red);
```

#### MQMaterial.color

内容は `{r: red, g: green, b: blue, a: alpha}`.

アルファの値も一緒に扱います．設定する場合はアルファは省略可能です．

### MQScene

カメラ情報のみアクセスできます．

- mqdocument.scene.cameraPosition → `{x: X, y: Y, z: Z}`
- mqdocument.scene.cameraLookAt → `{x: X, y: Y, z: Z}`
- mqdocument.scene.cameraAngle → `{bank: B, head: H, pitch: Z}`
- mqdocument.scene.rotationCenter
- mqdocument.scene.zoom
- mqdocument.scene.fov


### 組み込みモジュール

プラグイン組み込みのモジュールがいくつかあります．他にも `scripts/modules` 以下にも再利用できそうなものを置いてあります．

- geom: Vertex3,Matrix4,Quaternion,Plane 等の頻繁に使うクラス．
- mqwidget: 各種ダイアログやウインドウの生成
- child_process: プロセス起動
- fs: ファイルアクセス
- bsptree: シンプルなBinary Space Partition Treeの実装．C++で書かれているのでjsで処理するよりは高速

### 組み込み関数

- console.log("message") メッセージをログに出力
- alert()/prompt()/confirm() ダイアログ表示
- setInterval(), setTimeout() タイマー
- module.include(scriptPath) 別スクリプトの読み込み＆実行(仮実装)
- module.require(scriptPath) CommonJS形式のモジュール読み込み(仮実装)

カメラを回す例： (別のスクリプトを実行するまで停止しません)

```js
setInterval(() => {
	var originalLookAt = mqdocument.scene.cameraLookAt;
	mqdocument.scene.cameraPosition = MQMatrix.rotateMatrix(0,1,0, 1).transformV(mqdocument.scene.cameraPosition);
	mqdocument.scene.cameraLookAt = originalLookAt;
}, 10);
```

# License

MIT License

また，以下のライブラリに依存しています．

- https://github.com/c-smile/quickjspp
- https://bellard.org/quickjs/
