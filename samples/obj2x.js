
document.compact();

var obj = document.objects[0];

console.log('clone');
var n = obj.clone();

console.log('setname');
n.name= obj.name + "x2";

console.log('transform');
for(var i=0; i< n.verts.length; i++) {
	var v = n.verts[i];
	console.log(" point" + v);
	n.verts[i] = [v[0] * 2, v[1] * 2, v[2] * 2];
}

console.log('add');
document.objects.append(n);

console.log('complete');
