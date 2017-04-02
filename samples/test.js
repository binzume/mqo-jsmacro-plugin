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

assertEquals("object", typeof document);
assertEquals("object", typeof process);
assertEquals("function", typeof MQMaterial);
assertEquals("function", typeof MQMaterial);

assertEquals("function", typeof MQMatrix); // .core.js

test("MQDocument", () => {
	document.compact();
	document.clearSelect();
	assertNotNull(document.objects);
	assertNotNull(document.materials);
	assertNotNull(document.scene);
	assertNotNull(document.scene);

	{
		// triangulate
		assertEquals(0, document.triangulate([]).length);
		assertEquals(3, document.triangulate([{x:0,y:0,z:0}, {x:0,y:1,z:0}, {x:0,y:0,z:1}]).length);
		assertEquals(6, document.triangulate([{x:0,y:0,z:0}, {x:0,y:1,z:0}, {x:0,y:0,z:1}, {x:1,y:1,z:1}]).length);
		assertEquals(0, document.triangulate([{x:0,y:0,z:0}]).length);
	}
	{
		// select
		let obj = new MQObject("test");
		let idx = document.objects.append(obj);
		obj.verts.append(0,0,0);
		obj.verts.append(0,1,0);
		obj.verts.append(0,0,1);
		obj.faces.append([0,1,2], 0);

		assertEquals(false, document.isVertexSelected(idx, 0));
		document.setVertexSelected(idx, 0, true);
		assertEquals(true, document.isVertexSelected(idx, 0));
		assertEquals(false, document.isFaceSelected(idx, 0));
		document.setFaceSelected(idx, 0, true);
		assertEquals(true, document.isFaceSelected(idx, 0));

		document.objects.remove(obj);
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
		document.objects.remove(obj);
		let idx = document.objects.append(obj);
		assert(obj.id != 0, "id is set.");
		assertEquals(idx, obj.index);
		assertEquals(idx, document.objects.append(obj), "appendx2 (?)");
		document.objects.remove(obj);
		assertEquals(0, obj.id, "id is not set.");
		assertEquals(undefined, document.objects[idx], "remove");
	}
	{
		let idx = document.objects.append( new MQObject("test") );
		assertEquals("test", document.objects[idx].name, "append");

		document.objects.remove(idx);
		assertEquals(undefined, document.objects[idx], "remove");
	}
	{
		assertThrows(TypeError, ()=>{ document.objects.remove(new MQMaterial())});
		assertThrows(TypeError, ()=>{ document.objects.remove({}) });
		assertThrows(TypeError, ()=>{ document.objects.remove("hoge") });
		assertThrows(TypeError, ()=>{ document.objects.append("hoge") });
		assertThrows(TypeError, ()=>{ document.objects.append({}) });
		assertThrows(TypeError, ()=>{ document.objects.append(123) });
	}
});

test("MQObject verts/faces", () => {
		let obj = new MQObject("test");
		assertEquals(0, obj.verts.length, "empty");
		assertEquals(0, obj.faces.length, "empty");
		obj.verts.append(123,0,0);
		obj.verts.append({x: 456, y: 1, z: 2});
		obj.verts.append({x: "789.0", y: 2, z: 1});
		assertEquals(3, obj.verts.length);
		assertEquals(123.0, obj.verts[0].x);
		assertEquals(456.0, obj.verts[1].x);
		assertEquals(0, obj.verts[2].x, "0 if not a number");
		assertEquals("number", typeof obj.verts[0].id);

		obj.faces.append([0,1,2], 0);
		obj.faces.append([0,2,1], 0);
		assertEquals(2, obj.faces.length);
		assertEquals(1, obj.faces[0].points[1]);
		assertEquals(0, obj.faces[0].material);
		assertEquals(2, obj.faces[1].points[1]);
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

		document.materials.remove(obj);
		let idx = document.materials.append(obj);
		assert(obj.id != 0, "id is set.");
		assertEquals(idx, obj.index);
		document.materials.remove(obj);
		assertEquals(0, obj.id, "id is not set.");
		assertEquals(undefined, document.materials[idx], "removed");
	}
	{
		let idx = document.materials.append( new MQMaterial("test") );
		assertEquals("test", document.materials[idx].name, "append");

		document.materials.remove(idx);
		assertEquals(undefined, document.materials[idx], "remove");
	}
	{
		assertThrows(TypeError, ()=>{ document.materials.remove(new MQObject())});
		assertThrows(TypeError, ()=>{ document.materials.remove({})});
		assertThrows(TypeError, ()=>{ document.materials.remove("hoge") });
		assertThrows(TypeError, ()=>{ document.materials.append(new MQObject())});
		assertThrows(TypeError, ()=>{ document.materials.append({}) });
		assertThrows(TypeError, ()=>{ document.materials.append("hoge") });
		assertThrows(TypeError, ()=>{ document.materials.append(123) });
	}
});

test("MQScene", () => {
	assertEquals("number", typeof document.scene.cameraPosition.x);
	assertEquals("number", typeof document.scene.cameraPosition.y);
	assertEquals("number", typeof document.scene.cameraPosition.z);
	assertNotNull(document.scene.cameraLookAt.x);
	assertNotNull(document.scene.cameraLookAt.y);
	assertNotNull(document.scene.cameraLookAt.z);
	assertNotNull(document.scene.cameraAngle.head);
	assertNotNull(document.scene.rotationCenter.x);
	assertNotNull(document.scene.zoom);
	assertNotNull(document.scene.fov);
});

console.log("ok. "+ success + "/" + tests + " tests.");
