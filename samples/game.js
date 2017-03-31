"use strict";

document.compact();

// hide all obj.
document.objects.forEach((o) => {o.visible = false; } );

// remove old.
document.objects.filter((o)=> {return o.name.startsWith("game_")} ).forEach((o) => { document.objects.remove(o) } );
document.materials.filter((o)=> {return o.name.startsWith("game_")} ).forEach((o) => { document.materials.remove(o) } );


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
	var st = points.map((p) => {p = tr ? tr.transformV(p) : p; return obj.verts.append(p[0], p[1], p[2])})[0];
	obj.faces.append([st + 0, st + 1, st + 2, st + 3], mat);
	obj.faces.append([st + 7, st + 6, st + 5, st + 4], mat);
	obj.faces.append([st + 1, st + 0, st + 4, st + 5], mat);
	obj.faces.append([st + 3, st + 2, st + 6, st + 7], mat);
	obj.faces.append([st + 2, st + 1, st + 5, st + 6], mat);
	obj.faces.append([st + 0, st + 3, st + 7, st + 4], mat);
	return st;
}

function cubeR(obj, size, mat) {
	size = size / 2;
	let points = [
		[-size, -size, -size],
		[size, -size, -size],
		[size, -size, size],
		[-size, -size, size],
		[-size, size, -size],
		[size, size, -size],
		[size, size, size],
		[-size, size, size]
	];
	var st = points.map((p) => {return obj.verts.append(p[0], p[1], p[2])})[0];
	obj.faces.append([st + 0, st + 1, st + 2, st + 3], mat);
	obj.faces.append([st + 7, st + 6, st + 5, st + 4], mat);
	obj.faces.append([st + 1, st + 0, st + 4, st + 5], mat);
	obj.faces.append([st + 3, st + 2, st + 6, st + 7], mat);
	obj.faces.append([st + 2, st + 1, st + 5, st + 6], mat);
	obj.faces.append([st + 0, st + 3, st + 7, st + 4], mat);
	return st;
}


var sky = new MQMaterial("game_sky");
sky.color = {r:0.0, g:1.0, b:1.0};
var skyIndex = document.materials.append(sky);

var ground = new MQMaterial("game_ground");
ground.color = {r:0.3, g:0.3, b:0.35};
var groundIndex = document.materials.append(ground);

var building = new MQMaterial("game_building");
building.color = {r:0.9, g:0.9, b:0.9};
var buildingIndex = document.materials.append(building);

var car = new MQMaterial("game_car");
car.color = {r:1.0, g:0.0, b:0.0};
var carIndex = document.materials.append(car);

var field = new MQObject("game_field");
document.objects.append(field);

cubeR(field, 3000, skyIndex);
planeXZ(field, 1000, groundIndex);

var buildings = [
	{x: 150, y: 0, w1: 1, w2:1,  h:2},
	{x: 240, y: 0, w1: 0.6, w2:1,  h:1.5}
];

buildings.forEach((b) => {
	var m = MQMatrix.translateMatrix(b.x,0,b.y).mul( MQMatrix.scaleMatrix(b.w1, b.h, b.w2).mul( MQMatrix.translateMatrix(0,50,0) ));
	cube(field, 100, buildingIndex, m);
});

field.locked = true;


var car = new MQObject("game_car");
cube(car, 10, carIndex, MQMatrix.translateMatrix(0,3,0).mul(MQMatrix.scaleMatrix(1,0.5,2)));
cube(car, 10, carIndex, MQMatrix.translateMatrix(0,5,0).mul(MQMatrix.scaleMatrix(1,0.8,1)));
document.objects.append(car);

var leftObj = new MQObject("game_select_to_LEFT");
var rightObj = new MQObject("game_select_to_RIGHT");
document.objects.append(leftObj);
document.objects.append(rightObj);


var carVec = {x: 0, y:0, z:0.5 };


setInterval(() => {
	var v = car.verts[0];
	var carPos = { x:v[0], y:v[1], z:v[2] };
	if (carPos.x > 500 || carPos.x < -500 || carPos.z > 500 || carPos.z < -500) {
		console.log("Game Over");
		throw "GameOver";
	}
	if (leftObj.selected) {
		var rot = MQMatrix.rotateMatrix(0,1,0, 0.5);
		carVec = rot.transformV(carVec);
		car.transform( MQMatrix.translateMatrix(carPos.x, carPos.y, carPos.z).mul(rot.mul(MQMatrix.translateMatrix(-carPos.x, -carPos.y, -carPos.z)))  );
	}
	if (rightObj.selected) {
		var rot = MQMatrix.rotateMatrix(0,1,0, -0.5);
		carVec = rot.transformV(carVec);
		car.transform( MQMatrix.translateMatrix(carPos.x, carPos.y, carPos.z).mul(rot.mul(MQMatrix.translateMatrix(-carPos.x, -carPos.y, -carPos.z)))  );
	}
	car.transform(MQMatrix.translateMatrix(carVec.x, carVec.y, carVec.z));
	document.scene.cameraLookAt = carPos;
	document.clearSelect();
}, 10);


