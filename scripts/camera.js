var scene = document.scene;

var orgfov = scene.fov;
scene.cameraPosition = {x:500, y:500, z:500};
scene.cameraLookAt = {x:0, y:500, z:0};
scene.rotationCenter = {x:0, y:0, z:1};
scene.fov = orgfov;

var pos = scene.cameraPosition;
var angle = scene.cameraAngle;
var lookAt = scene.cameraLookAt;
var center = scene.rotationCenter;
var fov = scene.fov;

console.log("pos: " + JSON.stringify(pos));
console.log("angle: " + JSON.stringify(angle));
console.log("lookat: " + JSON.stringify(lookAt));
console.log("center: " + JSON.stringify(center));
console.log("fov: " + fov);

