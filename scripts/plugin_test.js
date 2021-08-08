// @ts-check
/// <reference path="mq_plugin.d.ts" />
import { assert, test } from "./modules/tests.js"

test("Core", (t) => {
	assert.equals("object", typeof mqdocument);
	assert.equals("object", typeof process);
	assert.equals("string", typeof process.version);
	assert.equals("function", typeof MQMaterial);
	assert.equals("function", typeof MQMaterial);
	assert.equals("function", typeof MQMatrix); // .core.js
	console.log(" Version: " + process.version);
});

test("MQDocument", (t) => {
	mqdocument.compact();
	mqdocument.clearSelect();
	assert.notNull(mqdocument.objects);
	assert.notNull(mqdocument.materials);
	assert.notNull(mqdocument.scene);
	assert.notNull(mqdocument.scene);

	{
		// triangulate
		assert.equals(0, mqdocument.triangulate([]).length);
		assert.equals(3, mqdocument.triangulate([{ x: 0, y: 0, z: 0 }, { x: 0, y: 1, z: 0 }, { x: 0, y: 0, z: 1 }]).length);
		assert.equals(6, mqdocument.triangulate([{ x: 0, y: 0, z: 0 }, { x: 0, y: 1, z: 0 }, { x: 0, y: 0, z: 1 }, { x: 1, y: 1, z: 1 }]).length);
		assert.equals(0, mqdocument.triangulate([{ x: 0, y: 0, z: 0 }]).length);
	}
	{
		// select
		let obj = new MQObject("test");
		let idx = mqdocument.objects.append(obj);
		obj.verts.append(0, 0, 0);
		obj.verts.append(0, 1, 0);
		obj.verts.append(0, 0, 1);
		obj.faces.append([0, 1, 2], 0);

		assert.equals(false, mqdocument.isVertexSelected(idx, 0));
		mqdocument.setVertexSelected(idx, 0, true);
		assert.equals(true, mqdocument.isVertexSelected(idx, 0));
		assert.equals(false, mqdocument.isFaceSelected(idx, 0));
		mqdocument.setFaceSelected(idx, 0, true);
		assert.equals(true, mqdocument.isFaceSelected(idx, 0));

		mqdocument.objects.remove(obj);
	}
});

test("MQObject", (t) => {
	{
		let obj = new MQObject("test");
		assert.equals(0, obj.id, "id is not set.");
		assert.equals("test", obj.name);
		assert.equals(true, obj.visible);
		assert.equals(false, obj.selected);
		assert.equals(-1, obj.index);
		assert.equals(0, obj.type);
		obj.selected = true;
		assert.equals(true, obj.selected);
		obj.visible = false;
		assert.equals(false, obj.visible);
		mqdocument.objects.remove(obj);
		let idx = mqdocument.objects.append(obj);
		assert.assert(obj.id != 0, "id is set.");
		assert.equals(idx, obj.index);
		assert.equals(obj, mqdocument.objects[idx], "get object");
		assert.equals(idx, mqdocument.objects.append(obj), "appendx2 (?)");
		mqdocument.objects.remove(obj);
		assert.equals(0, obj.id, "id is not set.");
		assert.equals(undefined, mqdocument.objects[idx], "remove");
	}
	{
		let idx = mqdocument.objects.append(new MQObject("test"));
		assert.equals("test", mqdocument.objects[idx].name, "append");

		mqdocument.objects.remove(idx);
		assert.equals(undefined, mqdocument.objects[idx], "remove");
	}
	{
		// Error
		// @ts-ignore
		assert.throws(TypeError, () => { mqdocument.objects.remove(new MQMaterial()) }, "not a MQObject");
		// @ts-ignore
		assert.throws(TypeError, () => { mqdocument.objects.remove({}) }, "not a MQObject 2");
		// @ts-ignore
		assert.throws(TypeError, () => { mqdocument.objects.remove("hoge") });
		// @ts-ignore
		assert.throws(TypeError, () => { mqdocument.objects.append("hoge") });
		// @ts-ignore
		assert.throws(TypeError, () => { mqdocument.objects.append({}) });
		// @ts-ignore
		assert.throws(TypeError, () => { mqdocument.objects.append(123) });
	}
});

test("MQObject verts/faces", (t) => {
	let obj = new MQObject("test");
	assert.equals(0, obj.verts.length, "empty");
	assert.equals(0, obj.faces.length, "empty");
	obj.verts.append(123, 0, 0);
	obj.verts.append({ x: 456, y: 1, z: 2 });
	// @ts-ignore
	obj.verts.append({ x: "789", y: 2, z: 1 });
	assert.equals(3, obj.verts.length);
	assert.equals(123.0, obj.verts[0].x);
	assert.equals(456.0, obj.verts[1].x);
	// assert.equals(0, obj.verts[2].x, "0 if not a number");
	assert.equals("number", typeof obj.verts[0].id);

	obj.faces.append([0, 1, 2], 0);
	obj.faces.append([0, 2, 1], 0);
	assert.equals(2, obj.faces.length);
	assert.equals(1, obj.faces[0].points[1]);
	assert.equals(0, obj.faces[0].material);
	assert.equals(2, obj.faces[1].points[1]);
	assert.equals(0, obj.faces[0].index);
	assert.equals(3, obj.faces[0].uv.length);
	assert.equals("number", typeof obj.faces[0].uv[0].u);
	assert.equals("number", typeof obj.faces[0].uv[0].v);
	assert.equals(true, obj.faces[1].visible);
	obj.faces[1].visible = false;
	assert.equals(false, obj.faces[1].visible);
	assert.equals("number", typeof obj.faces[0].id);
	obj.faces[0] = { points: [0, 2, 1], material: 0 };
	assert.equals(2, obj.faces[0].points[1]);

	delete obj.verts[0];
	assert.equals(0, obj.verts[2].refs);
	assert.equals(undefined, obj.faces[0]);
});

test("MQMaterial", (t) => {
	{
		let obj = new MQMaterial("test");
		assert.equals(0, obj.id, "id is not set.");
		assert.equals("test", obj.name);
		assert.equals(false, obj.selected);
		assert.equals(-1, obj.index);
		assert.equals("number", typeof obj.color.r);
		assert.equals("number", typeof obj.color.g);
		assert.equals("number", typeof obj.color.b);
		assert.equals("number", typeof obj.color.a);
		obj.color = { r: 0.5, g: 0.1, b: 0.2, a: 0.25 };
		assert.equals(0.5, obj.color.r);
		obj.color = { r: 1, g: 1, b: 1 };
		assert.equals(1.0, obj.color.r);
		assert.equals(0.25, obj.color.a);

		mqdocument.materials.remove(obj);
		let idx = mqdocument.materials.append(obj);
		assert.assert(obj.id != 0, "id is set.");
		assert.equals(idx, obj.index);
		mqdocument.materials.remove(obj);
		assert.equals(0, obj.id, "id is not set.");
		assert.equals(undefined, mqdocument.materials[idx], "removed");
	}
	{
		let idx = mqdocument.materials.append(new MQMaterial("test"));
		assert.equals("test", mqdocument.materials[idx].name, "append");

		mqdocument.materials.remove(idx);
		assert.equals(undefined, mqdocument.materials[idx], "remove");
	}
	{
		// Error
		// @ts-ignore
		assert.throws(TypeError, () => { mqdocument.materials.remove(new MQObject()) });
		// @ts-ignore
		assert.throws(TypeError, () => { mqdocument.materials.remove({}) });
		// @ts-ignore
		assert.throws(TypeError, () => { mqdocument.materials.remove("hoge") });
		// @ts-ignore
		assert.throws(TypeError, () => { mqdocument.materials.append(new MQObject()) });
		// @ts-ignore
		assert.throws(TypeError, () => { mqdocument.materials.append({}) });
		// @ts-ignore
		assert.throws(TypeError, () => { mqdocument.materials.append("hoge") });
		// @ts-ignore
		assert.throws(TypeError, () => { mqdocument.materials.append(123) });
	}
});

test("MQScene", (t) => {
	assert.equals("number", typeof mqdocument.scene.cameraPosition.x);
	assert.equals("number", typeof mqdocument.scene.cameraPosition.y);
	assert.equals("number", typeof mqdocument.scene.cameraPosition.z);
	assert.notNull(mqdocument.scene.cameraLookAt.x);
	assert.notNull(mqdocument.scene.cameraLookAt.y);
	assert.notNull(mqdocument.scene.cameraLookAt.z);
	assert.notNull(mqdocument.scene.cameraAngle.head);
	assert.notNull(mqdocument.scene.rotationCenter.x);
	assert.notNull(mqdocument.scene.zoom);
	assert.notNull(mqdocument.scene.fov);
});

test("Result", (t) => {
	if (t.success == t.count) {
		console.log(" ok. " + t.success + "/" + t.count + " tests passed.");
	} else {
		console.error(" " + t.success + "/" + t.count + " tests passed.");
		console.error(" failed:" + (t.count - t.success));
	}
});
