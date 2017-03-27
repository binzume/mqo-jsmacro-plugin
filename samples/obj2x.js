
console.log('document.objects');
console.log(document.objects.length);

document.objects.forEach(function(o){
	console.log("name: " + o.name);
	console.log("verts: " + o.verts.length);
});

var obj = document.objects[0];

console.log('clone');
var n = obj.clone();
n.compact();
console.log('setname');
n.name= obj.name + "x2";
for(var i=0; i< n.verts.length; i++) {
	var v = n.verts[i];
	console.log(" point" + v);
	n.verts[i] = [v[0] * 2, v[1] * 2, v[2] * 2];
}

console.log('add');
document.addObject(n);
console.log('ok');
