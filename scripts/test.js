// @ts-check
"use strict";
// test

let asserts = 0, tests = 0, success = 0;
function assert(v, msg) {
	if (v !== true) throw "FAILED: " + msg;
	asserts ++;
}
function assertEquals(expect, v, msg) {
	assert (expect === v, "" + v +" != " + expect + "(expect) msg:" +  msg);
}
function assertNotNull(expect, v, msg) {
	assert (expect != null, "" + v +" is not null msg:" +  msg);
}
function assertThrows(t, b, msg) {
	let ok = false;
	try { b(); } catch (e) { ok = (e instanceof t); }
	assert(ok, t.name + " not throws. "+ msg);
}
function test(name, b) {
	tests++;
	asserts = 0;
	console.log(name);
	try { b() } catch (e) { console.log(" " + asserts + " " + e); return; }
	console.log(" ok. " + asserts + " assert.");
	success ++;
}

test("Core", () => {
	assertEquals("object", typeof mqdocument);
	assertEquals("object", typeof process);
	assertEquals("string", typeof process.version);
	assertEquals("function", typeof MQMaterial);
	assertEquals("function", typeof MQMaterial);
	assertEquals("function", typeof MQMatrix); // .core.js
	console.log(" Version: " + process.version);
});

test("MQDocument", () => {
	mqdocument.compact();
	mqdocument.clearSelect();
	assertNotNull(mqdocument.objects);
	assertNotNull(mqdocument.materials);
	assertNotNull(mqdocument.scene);
	assertNotNull(mqdocument.scene);

	{
		// triangulate
		assertEquals(0, mqdocument.triangulate([]).length);
		assertEquals(3, mqdocument.triangulate([{x:0,y:0,z:0}, {x:0,y:1,z:0}, {x:0,y:0,z:1}]).length);
		assertEquals(6, mqdocument.triangulate([{x:0,y:0,z:0}, {x:0,y:1,z:0}, {x:0,y:0,z:1}, {x:1,y:1,z:1}]).length);
		assertEquals(0, mqdocument.triangulate([{x:0,y:0,z:0}]).length);
	}
	{
		// select
		let obj = new MQObject("test");
		let idx = mqdocument.objects.append(obj);
		obj.verts.append(0,0,0);
		obj.verts.append(0,1,0);
		obj.verts.append(0,0,1);
		obj.faces.append([0,1,2], 0);

		assertEquals(false, mqdocument.isVertexSelected(idx, 0));
		mqdocument.setVertexSelected(idx, 0, true);
		assertEquals(true, mqdocument.isVertexSelected(idx, 0));
		assertEquals(false, mqdocument.isFaceSelected(idx, 0));
		mqdocument.setFaceSelected(idx, 0, true);
		assertEquals(true, mqdocument.isFaceSelected(idx, 0));

		mqdocument.objects.remove(obj);
	}
});

test("MQObject", () => {
	{
		let obj = new MQObject("test");
		assertEquals(0, obj.id, "id is not set.");
		assertEquals("test", obj.name);
		assertEquals(true, obj.visible);
		assertEquals(false, obj.selected);
		assertEquals(-1, obj.index);
		assertEquals(0, obj.type);
		obj.selected = true;
		assertEquals(true, obj.selected);
		obj.visible = false;
		assertEquals(false, obj.visible);
		mqdocument.objects.remove(obj);
		let idx = mqdocument.objects.append(obj);
		assert(obj.id != 0, "id is set.");
		assertEquals(idx, obj.index);
		assertEquals(idx, mqdocument.objects.append(obj), "appendx2 (?)");
		mqdocument.objects.remove(obj);
		assertEquals(0, obj.id, "id is not set.");
		assertEquals(undefined, mqdocument.objects[idx], "remove");
	}
	{
		let idx = mqdocument.objects.append( new MQObject("test") );
		assertEquals("test", mqdocument.objects[idx].name, "append");

		mqdocument.objects.remove(idx);
		assertEquals(undefined, mqdocument.objects[idx], "remove");
	}
	{
		// Error
		// @ts-ignore
		assertThrows(TypeError, ()=>{ mqdocument.objects.remove(new MQMaterial()) });
		// @ts-ignore
		assertThrows(TypeError, ()=>{ mqdocument.objects.remove({}) });
		// @ts-ignore
		assertThrows(TypeError, ()=>{ mqdocument.objects.remove("hoge") });
		// @ts-ignore
		assertThrows(TypeError, ()=>{ mqdocument.objects.append("hoge") });
		// @ts-ignore
		assertThrows(TypeError, ()=>{ mqdocument.objects.append({}) });
		// @ts-ignore
		assertThrows(TypeError, ()=>{ mqdocument.objects.append(123) });
	}
});

test("MQObject verts/faces", () => {
		let obj = new MQObject("test");
		assertEquals(0, obj.verts.length, "empty");
		assertEquals(0, obj.faces.length, "empty");
		obj.verts.append(123,0,0);
		obj.verts.append({x: 456, y: 1, z: 2});
		// @ts-ignore
		obj.verts.append({x: "789", y: 2, z: 1});
		assertEquals(3, obj.verts.length);
		assertEquals(123.0, obj.verts[0].x);
		assertEquals(456.0, obj.verts[1].x);
		// assertEquals(0, obj.verts[2].x, "0 if not a number");
		assertEquals("number", typeof obj.verts[0].id);

		obj.faces.append([0,1,2], 0);
		obj.faces.append([0,2,1], 0);
		assertEquals(2, obj.faces.length);
		assertEquals(1, obj.faces[0].points[1]);
		assertEquals(0, obj.faces[0].material);
		assertEquals(2, obj.faces[1].points[1]);
		assertEquals(0, obj.faces[0].index);
		assertEquals(3, obj.faces[0].uv.length);
		assertEquals("number", typeof obj.faces[0].uv[0].u);
		assertEquals("number", typeof obj.faces[0].uv[0].v);
		assertEquals(true, obj.faces[1].visible);
		obj.faces[1].visible = false;
		assertEquals(false, obj.faces[1].visible);
		assertEquals("number", typeof obj.faces[0].id);
		obj.faces[0] = {points: [0,2,1], material: 0};
		assertEquals(2, obj.faces[0].points[1]);

		delete obj.verts[0];
		assertEquals(0, obj.verts[2].refs);
		assertEquals(undefined, obj.faces[0]);
});

test("MQMaterial", () => {
	{
		let obj = new MQMaterial("test");
		assertEquals(0, obj.id, "id is not set.");
		assertEquals("test", obj.name);
		assertEquals(false, obj.selected);
		assertEquals(-1, obj.index);
		assertEquals("number", typeof obj.color.r);
		assertEquals("number", typeof obj.color.g);
		assertEquals("number", typeof obj.color.b);
		assertEquals("number", typeof obj.color.a);
		obj.color = {r: 0.5, g: 0.1, b: 0.2, a: 0.25};
		assertEquals(0.5, obj.color.r);
		obj.color = {r: 1, g: 1, b: 1};
		assertEquals(1.0, obj.color.r);
		assertEquals(0.25, obj.color.a);

		mqdocument.materials.remove(obj);
		let idx = mqdocument.materials.append(obj);
		assert(obj.id != 0, "id is set.");
		assertEquals(idx, obj.index);
		mqdocument.materials.remove(obj);
		assertEquals(0, obj.id, "id is not set.");
		assertEquals(undefined, mqdocument.materials[idx], "removed");
	}
	{
		let idx = mqdocument.materials.append( new MQMaterial("test") );
		assertEquals("test", mqdocument.materials[idx].name, "append");

		mqdocument.materials.remove(idx);
		assertEquals(undefined, mqdocument.materials[idx], "remove");
	}
	{
		// Error
		// @ts-ignore
		assertThrows(TypeError, ()=>{ mqdocument.materials.remove(new MQObject())});
		// @ts-ignore
		assertThrows(TypeError, ()=>{ mqdocument.materials.remove({})});
		// @ts-ignore
		assertThrows(TypeError, ()=>{ mqdocument.materials.remove("hoge") });
		// @ts-ignore
		assertThrows(TypeError, ()=>{ mqdocument.materials.append(new MQObject())});
		// @ts-ignore
		assertThrows(TypeError, ()=>{ mqdocument.materials.append({}) });
		// @ts-ignore
		assertThrows(TypeError, ()=>{ mqdocument.materials.append("hoge") });
		// @ts-ignore
		assertThrows(TypeError, ()=>{ mqdocument.materials.append(123) });
	}
});

test("MQScene", () => {
	assertEquals("number", typeof mqdocument.scene.cameraPosition.x);
	assertEquals("number", typeof mqdocument.scene.cameraPosition.y);
	assertEquals("number", typeof mqdocument.scene.cameraPosition.z);
	assertNotNull(mqdocument.scene.cameraLookAt.x);
	assertNotNull(mqdocument.scene.cameraLookAt.y);
	assertNotNull(mqdocument.scene.cameraLookAt.z);
	assertNotNull(mqdocument.scene.cameraAngle.head);
	assertNotNull(mqdocument.scene.rotationCenter.x);
	assertNotNull(mqdocument.scene.zoom);
	assertNotNull(mqdocument.scene.fov);
});

console.log("ok. "+ success + "/" + tests + " tests.");
if (success != tests) {
	alert("failed:"+(tests-success));
}
