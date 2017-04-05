"use strict";

// csg.js : https://evanw.github.io/csg.js/
module.include("./csg.js");

function findObjectByName(name) {
	return document.objects.find((o)=>{return o && o.name == name;});
}

function toCGSPolygons(obj) {
	return obj.faces.map((f) => {
		let v1 = new CSG.Vector(obj.verts[f.points[1]]).minus(obj.verts[f.points[0]]);
		let v2 = new CSG.Vector(obj.verts[f.points[2]]).minus(obj.verts[f.points[0]]);
		let n = v1.cross(v2).unit();
		let vs = f.points.map((i) => { return new CSG.Vertex(obj.verts[i], n); });
		return new CSG.Polygon(vs.reverse(), [f,obj]);
	});
}

function fromCSGPolygons(obj, polygons) {
	polygons.forEach((p)=>{
		let a = p.vertices.map((v)=>{ return obj.verts.append(v.pos); });
		obj.faces.append(a.reverse(), p.shared ? p.shared[0].material : null);
	});
}

function veq(v1, v2) {
	return v1.pos.minus(v2.pos).length() < 0.0000001;
}

function samev(v0, v1, v2) {
	return v1.pos.minus(v0.pos).unit().dot( v2.pos.minus(v0.pos).unit() ) > 0.99999999; // TODO eps
}

function overwrap(a1, a2, b1, b2) {
	let a1a2 = a2.pos.minus(a1.pos);
	let b1b2 = b2.pos.minus(b1.pos);
	if (a1a2.dot( b1b2 ) > a1a2.length()*b1b2.length() - 0.00000001) {
		let a1b1 = b1.pos.minus(a1.pos);
		if (a1b1.length() < a1a2.length() - 0.00000001) {
			if (a1a2.dot(a1b1) > a1a2.length()*a1b1.length() - 0.00000001) {
				return true;
			}
		}
		let b1a1 = a1.pos.minus(b1.pos);
		if (b1a1.length() < b1b2.length() - 0.0000001) {
			if (b1b2.dot(b1a1) > b1b2.length()*b1a1.length() - 0.00000001) {
				return true;
			}
		}
	}
	return false;
}

function tryMerge(p1, p2) {
	let vs1 = p1.vertices
	let vs2 = p2.vertices
	for (var i=0; i<vs1.length; i++) {
		for (var j=0; j<vs2.length; j++) {
			if (overwrap(vs1[i], vs1[(i + 1) % vs1.length], vs2[j], vs2[(j - 1 +vs2.length) % vs2.length])) {
				let i2 = (i + 1) % vs1.length;
				let j2 = (j - 1 + vs2.length) % vs2.length;
				let vv = (i2 > i) ? vs1.slice(i2).concat(vs1.slice(0,i+1)) : vs1.slice(i2, i+1);
				vv = (j > j2) ? vv.concat(vs2.slice(j)).concat( vs2.slice(0, j2+1) ) : vv.concat(vs2.slice(j,j2+1));
				do {
					var f = false;
					for (var k=0; k<vv.length; k++) {
						let d = (k+1)%vv.length;
						if (veq(vv[k],vv[d]) || overwrap(vv[k],vv[d], vv[(k+2)%vv.length], vv[d]) || samev(vv[k],vv[d], vv[(k+2)%vv.length])){
							vv = d == 0 ? vv.slice(1) : vv.slice(0,d).concat(vv.slice(d+1));
							f = true;
						}
					}
				} while(f);
				return new CSG.Polygon(vv, p1.shared);
			}
		}
	}
	return null;
}

function mergePolygons(polygons) {
	var byface = new Map();
	polygons.forEach((p) => {
		var k = p.shared[1].id +"," + p.shared[0].id;
		var a = byface.get(k) || [];
		a.push(p);
		byface.set(k, a);
	});

	let merged = true;
	while (merged) {
		merged = false;
		byface.forEach((fs)=>{
			for (var i=0; i<fs.length; i++) {
				if (!fs[i]) continue;
				for (var j=i+1; j<fs.length; j++) {
					if (!fs[j]) continue;
					var m = tryMerge(fs[i], fs[j]);
					if (m) {
						delete fs[j];
						fs[i] = m;
						merged = true;
					}
				}
			}
		});
	}

	var mpolygons = [];
	byface.forEach((fs)=>{
		mpolygons = mpolygons.concat(fs.filter((f)=>{return f}) );
	});
	console.log([polygons.length, mpolygons.length]);
	return mpolygons;
}


document.objects.forEach((o) => {
	var m = o.name.match("boolean:(.*)");
	if (!m) return;
	var exp = '(' + m[1].replace(/\s+/, '') + ')';

	var csgs = {}, seq = 0;
	do {
		console.log(exp);
		var lastexp = exp;
		exp = exp.replace(/\((\w+)\)/, "$1"); // (a) => a
		exp = exp.replace(/~(\w+)/g, (a, op1) => {
			csgs[op1] = csgs[op1] || CSG.fromPolygons(toCGSPolygons(findObjectByName(op1)));
			csgs["_tmp"+seq] = csgs[op1].invert();
			return "_tmp" + (seq++);
		});
		exp = exp.replace(/(\w+)([+\-&|])(\w+)/g, (a, op1, op, op2) => {
			csgs[op1] = csgs[op1] || CSG.fromPolygons(toCGSPolygons(findObjectByName(op1)));
			csgs[op2] = csgs[op2] || CSG.fromPolygons(toCGSPolygons(findObjectByName(op2)));
			if (op == "+" || op == "|") {
				csgs["_tmp"+seq] = csgs[op1].union(csgs[op2]);
			} else if (op == "-") {
				csgs["_tmp"+seq] = csgs[op1].subtract(csgs[op2]);
			} else if (op == "&") {
				csgs["_tmp"+seq] = csgs[op1].intersect(csgs[op2]);
			}
			return "_tmp" + (seq++);
		});
	} while (exp != lastexp);

	if (csgs[exp]) {
		// clone object without faces.
		var dst = o.clone();
		dst.verts.forEach((v,i)=>{delete dst.verts[i]});
		dst.compact();
		fromCSGPolygons(dst, mergePolygons(csgs[exp].toPolygons()));

		// swap
		console.log("replace: " + o.name);
		document.objects.remove(o);
		document.objects.append(dst);
	}
});
console.log("finished");

