
var str = JSON.stringify;

document.objects.forEach(function(o){
	console.log("object: " + o.name);
	console.log(" id: " + o.id);
	console.log(" verts: " + o.verts.length);
	console.log(" faces: " + o.faces.length);
	console.log(" visible: " + o.visible);
	console.log(" selected: " + o.selected);
});

document.materials.forEach(function(m){
	console.log("material: " + m.name);
	console.log(" id: " + m.id);
	console.log(" color: " + str(m.color));
	console.log(" selected: " + m.selected);
});

