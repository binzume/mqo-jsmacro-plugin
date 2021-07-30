// @ts-check
/// <reference path="mq_plugin.d.ts" />

import { Vector3, Matrix4 } from "geom";

mqdocument.compact();

// hide all obj.
mqdocument.objects.forEach((o) => {o.visible = false; } );

// remove old.
mqdocument.objects.filter((o)=> {return o.name.startsWith("game_")} ).forEach((o) => { mqdocument.objects.remove(o) } );
mqdocument.materials.filter((o)=> {return o.name.startsWith("game_")} ).forEach((o) => { mqdocument.materials.remove(o) } );


function planeXZ(obj, size, mat) {
	size = size / 2;
	var st = obj.verts.append(-size, 0, -size);
	obj.verts.append(size, 0, -size);
	obj.verts.append(size, 0, size);
	obj.verts.append(-size, 0, size);
	obj.faces.append([st + 0, st + 1, st + 2, st + 3], mat);
	return st;
}

function cube(obj, size, mat, tr) {
	size = size / 2;
	let points = [
		[-size, size, -size],
		[size, size, -size],
		[size, size, size],
		[-size, size, size],
		[-size, -size, -size],
		[size, -size, -size],
		[size, -size, size],
		[-size, -size, size]
	];
	var st = points.map((p) => {let v = new Vector3(p); return obj.verts.append(tr ? tr.applyTo(v) : v)})[0];
	obj.faces.append([st + 0, st + 1, st + 2, st + 3], mat);
	obj.faces.append([st + 7, st + 6, st + 5, st + 4], mat);
	obj.faces.append([st + 1, st + 0, st + 4, st + 5], mat);
	obj.faces.append([st + 3, st + 2, st + 6, st + 7], mat);
	obj.faces.append([st + 2, st + 1, st + 5, st + 6], mat);
	obj.faces.append([st + 0, st + 3, st + 7, st + 4], mat);
	return st;
}



var ground = new MQMaterial("game_ground");
ground.color = {r:0.3, g:0.3, b:0.35};
var groundIndex = mqdocument.materials.append(ground);

var building = new MQMaterial("game_building");
building.color = {r:0.9, g:0.9, b:0.9};
var buildingIndex = mqdocument.materials.append(building);

var carmat = new MQMaterial("game_car");
carmat.color = {r:1.0, g:0.0, b:0.0};
var carIndex = mqdocument.materials.append(carmat);

var field = new MQObject("game_field");
mqdocument.objects.append(field);

planeXZ(field, 1000, groundIndex);

var buildings = [
	{x: 50, y: 0, w1: 30, w2:50,  h:70},
	{x: 50, y: 60, w1: 30, w2:50,  h:60},
	{x: 50, y: 150, w1: 30, w2:70,  h:120},
	{x: -50, y: 100, w1: 30, w2:50,  h:70},
	{x: -50, y: 160, w1: 30, w2:50,  h:90},
	{x: 120, y: 0, w1: 70, w2:40,  h:120},
	{x: 0, y: 400, w1: 60, w2:40,  h:100}
];
buildings.forEach((b) => {
	var m = Matrix4.translateMatrix(b.x,0,b.y).mul( Matrix4.scaleMatrix(b.w1, b.h, b.w2).mul( Matrix4.translateMatrix(0,0.5,0) ));
	cube(field, 1, buildingIndex, m);
});

field.locked = true;


var car = new MQObject("game_car");
car.verts.append(0,0,10);
car.verts.append(-1,0,5);
car.verts.append(1,0,5);
car.faces.append([0,1,2], carIndex);
cube(car, 10, carIndex, Matrix4.translateMatrix(0,3,0).mul(Matrix4.scaleMatrix(1,0.4,2)));
cube(car, 10, carIndex, Matrix4.translateMatrix(0,6,-1).mul(Matrix4.scaleMatrix(1,0.6,1)));
mqdocument.objects.append(car);

var leftObj = new MQObject("game_select_to_LEFT");
var rightObj = new MQObject("game_select_to_RIGHT");
mqdocument.objects.append(leftObj);
mqdocument.objects.append(rightObj);
mqdocument.scene.fov = 0.1;

var carVec = {x: 0, y:0, z:0.8 };


setInterval(() => {
	var carPos = car.verts[0];
	var over = false;
	if (carPos.x > 500 || carPos.x < -500 || carPos.z > 500 || carPos.z < -500) {
		over = true;
	}
	buildings.forEach((b) => {
		if (carPos.x > b.x - b.w1 / 2  && carPos.x < b.x + b.w1 / 2 && carPos.z > b.y - b.w2 / 2 && carPos.z < b.y + b.w2 / 2) {
			over = true;
		}
	});
	if (over) {
		console.log("Game Over");
		throw "GameOver";
	}
	if (leftObj.selected) {
		var rot = Matrix4.rotateMatrix(0,1,0, 0.5);
		carVec = rot.applyTo(carVec);
		car.applyTransform(Matrix4.translateMatrix(carPos.x, carPos.y, carPos.z).mul(rot.mul(Matrix4.translateMatrix(-carPos.x, -carPos.y, -carPos.z)))  );
	}
	if (rightObj.selected) {
		var rot = Matrix4.rotateMatrix(0,1,0, -0.5);
		carVec = rot.applyTo(carVec);
		car.applyTransform( Matrix4.translateMatrix(carPos.x, carPos.y, carPos.z).mul(rot.mul(Matrix4.translateMatrix(-carPos.x, -carPos.y, -carPos.z)))  );
	}
	car.applyTransform(Matrix4.translateMatrix(carVec.x, carVec.y, carVec.z));
	mqdocument.scene.cameraLookAt = carPos;
	mqdocument.clearSelect();
}, 10);
