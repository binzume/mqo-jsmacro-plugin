
document.objects.forEach(function(o){
	console.log("name: " + o.name);
	console.log(" verts: " + o.verts.length);
	console.log(" faces: " + o.faces.length);
});

document.materials.forEach(function(m){
	console.log("name: " + m.name);
	var c = m.color;
	console.log(" color: " + [c.r, c.g, c.b, c.a]);
});

