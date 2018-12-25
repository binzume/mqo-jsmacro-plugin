"use strict";

// csg.js : https://evanw.github.io/csg.js/
module.include("./csg.js");

// CSG.Plane.EPSILON = 1e-6;
var epsilon = 1e-10;

function findObjectByName(name) {
	return document.objects.find((o) => { return o && o.name.split("=")[0] == name; });
}

function getObjectByName(name) {
	var obj = findObjectByName(name);
	if (!obj) {
		alert("Object not found : " + name);
		throw "Object not found : " + name;
	}
	return obj;
}

function isConvexPolygon(points) {
	return points.length == 3;
	let v1 = points[points.length - 1].minus(points[0]);
	let v2 = points[1].minus(points[0]);
	let n = v1.cross(v2).unit();
	for (let i = 0; i < points.length - 1; i++) {
		v1 = points[i].minus(points[i + 1]);
		v2 = points[(i + 2) % points.length].minus(points[i + 1]);
		let n2 = v1.cross(v2).unit();
		if (n2.minus(n).length > epsilon) return false;
		n = n2;
	}
	return true;
}

function collinear(v0, v1, v2) {
	let v01 = v1.pos.minus(v0.pos);
	let v02 = v2.pos.minus(v0.pos);
	let d = v01.length() * v02.length() - Math.abs(v01.dot(v02));
	return d < epsilon;
}

function overwrap(a1, a2, b1, b2) {
	let a1a2 = a2.pos.minus(a1.pos);
	let b1b2 = b2.pos.minus(b1.pos);
	if (a1a2.dot(b1b2) > a1a2.length() * b1b2.length() - epsilon) {
		let a1b1 = b1.pos.minus(a1.pos);
		if (a1b1.length() < a1a2.length() - epsilon) {
			if (a1a2.dot(a1b1) > a1a2.length() * a1b1.length() - epsilon) {
				return true;
			}
		}
		let b1a1 = a1.pos.minus(b1.pos);
		if (b1a1.length() < b1b2.length() - epsilon) {
			if (b1b2.dot(b1a1) > b1b2.length() * b1a1.length() - epsilon) {
				return true;
			}
		}
	}
	return false;
}

function toCSGPolygons(obj) {
	return obj.faces.reduce((acc, f) => {
		if (!f || f.points.length < 3) return acc;
		let points = f.points.map((i) => { return new CSG.Vector(obj.verts[i]) });
		if (isConvexPolygon(points)) {
			let n = new CSG.Vector(0, 0, 0);
			let vs = points.map((p) => { return new CSG.Vertex(p, n); });
			acc.push(new CSG.Polygon(vs.reverse(), [f, obj]));
		} else {
			let n = new CSG.Vector(0, 0, 0);
			let trpoints = document.triangulate(points).map((i) => { return points[i] });
			for (let i = 0; i < trpoints.length / 3; i++) {
				let vs = trpoints.slice(i * 3, i * 3 + 3).map((p) => { return new CSG.Vertex(p, n); });
				acc.push(new CSG.Polygon(vs.reverse(), [f, obj]));
			}
		}
		return acc;
	}, []);
}

function cleanEdge(vv) {
	do {
		var f = false;
		for (let k = 0; k < vv.length; k++) {
			let d = (k + 1) % vv.length;
			if (collinear(vv[k], vv[d], vv[(k + 2) % vv.length])) {
				vv.splice(d, 1);
				f = true;
			}
		}
	} while (f);
	return vv;
}

function tryMerge(p1, p2) {
	let vs1 = p1.vertices
	let vs2 = p2.vertices
	for (var i = 0; i < vs1.length; i++) {
		let i2 = (i + 1) % vs1.length;
		for (var j = 0; j < vs2.length; j++) {
			let j2 = (j - 1 + vs2.length) % vs2.length;
			if (overwrap(vs1[i], vs1[i2], vs2[j], vs2[j2])) {
				let vv = (i2 > i) ? vs1.slice(i2).concat(vs1.slice(0, i + 1)) : vs1.slice(i2, i + 1);
				vv = (j > j2) ? vv.concat(vs2.slice(j)).concat(vs2.slice(0, j2 + 1)) : vv.concat(vs2.slice(j, j2 + 1));
				vv = cleanEdge(vv);
				if (vv.length < 3) {
					return null;
				}
				return new CSG.Polygon(vv, p1.shared);
			}
		}
	}
	return null;
}

function mergePolygons(polygons) {
	var mpolygons = [];

	var byface = new Map();
	polygons.forEach((p) => {
		if (!p.shared) {
			mpolygons.push(p);
			return;
		}
		var k = p.shared[1].id + "," + p.shared[0].id;
		var a = byface.get(k) || [];
		a.push(p);
		byface.set(k, a);
	});

	let merged = true;
	while (merged) {
		merged = false;
		byface.forEach((fs) => {
			for (var i = 0; i < fs.length; i++) {
				if (!fs[i]) continue;
				for (var j = i + 1; j < fs.length; j++) {
					if (!fs[j]) continue;
					if (fs[i].vertices.length < 3) continue;
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

	byface.forEach((fs) => {
		mpolygons = mpolygons.concat(fs);
	});
	console.log(["merged", polygons.length, mpolygons.length]);
	return mpolygons;
}

function getVerts(csg, pos) {
	if (csg === null) {
		return [];
	}
	var t = csg.plane.normal.dot(pos) - csg.plane.w;
	if (t < -CSG.Plane.EPSILON) {
		return getVerts(csg.back, pos);
	} else if (t > CSG.Plane.EPSILON) {
		return getVerts(csg.front, pos);
	}
	let verts = getVerts(csg.back, pos).concat(getVerts(csg.front, pos));
	csg.polygons.forEach(p => {
		p.vertices.forEach(v => {
			if (v.pos.minus(pos).length() < CSG.Plane.EPSILON * 5) {
				verts.push(v);
			}
		});
	});
	return verts;
}

function getVertIndex(pos, obj, csgNode, map) {
	let i = map.get(pos);
	if (i === undefined) {
		i = obj.verts.append(pos);
		getVerts(csgNode, pos).forEach(v => map.set(v.pos, i));
	}
	return i;
}

function csgToObject(dst, csg, mergeFaces, mergeVerts) {
	let polygons = csg.clone().polygons;
	let csgNode = new CSG.Node(polygons);
	if (mergeFaces) {
		polygons = mergePolygons(polygons);
	}
	if (mergeVerts) {
		let vv = new Map();
		polygons.forEach((p) => {
			let a = p.vertices.map((v) => getVertIndex(v.pos, dst, csgNode, vv));
			dst.faces.append(a.reverse(), p.shared ? p.shared[0].material : null);
		});
	} else {
		polygons.forEach((p) => {
			let a = p.vertices.map((v) => dst.verts.append(v.pos));
			dst.faces.append(a.reverse(), p.shared ? p.shared[0].material : null);
		});
	}
}

var functions = {};
var methods = {};

functions["sphere"] = function (p) {
	return CSG.sphere({});
}

functions["cube"] = function (p) {
	return CSG.cube({});
}

functions["cylinder"] = function (p) {
	return CSG.cylinder({});
}

methods["scale"] = function (polygons, p) {
	var x = p[0] * 1.0;
	var y = p[1] * 1.0;
	var z = p[2] * 1.0;
	polygons.forEach((p) => {
		p.vertices.forEach((v) => {
			v.pos = new CSG.Vector(v.pos.x * x, v.pos.y * y, v.pos.z * z);
		});
	});
	return CSG.fromPolygons(polygons);
}

methods["tr"] = function (polygons, p) {
	var x = p[0] * 1.0;
	var y = p[1] * 1.0;
	var z = p[2] * 1.0;
	polygons.forEach((p) => {
		p.vertices.forEach((v) => {
			v.pos = new CSG.Vector(v.pos.x + x, v.pos.y + y, v.pos.z + z);
		});
	});
	return CSG.fromPolygons(polygons);
}

document.objects.forEach((o) => {
	var m = o.name.match("csg:(.*)");
	if (!m || o.locked) return;
	var exp = '(' + m[1].replace(/\s+/, '') + ')';

	var prefix = "_tmp";
	var csgs = {}, seq = 0;
	do {
		console.log(exp);
		var lastexp = exp;
		exp = exp.replace(/\((\w+)\)/, "$1"); // (a) => a
		exp = exp.replace(/(\w+)\.(\w+)\(([^)]*)\)/g, (a, op1, f, params) => {
			if (!methods[f]) {
				alert("ERROR func: " + op1 + ":" + f + " " + params);
				throw ("ERROR func: " + op1 + ":" + f + " " + params);
			}
			var polygons = csgs[op1] ? csgs[op1].clone().polygons : toCSGPolygons(getObjectByName(op1));
			csgs[prefix + seq] = methods[f](polygons, params.split(","));
			return prefix + (seq++);
		});
		exp = exp.replace(/\.?(\w+)\(([^)]*)\)/g, (a, f, params) => {
			if (a.startsWith(".")) return a;
			if (!functions[f]) {
				alert("ERROR func: " + f + " " + params);
				throw ("ERROR func: " + f + " " + params);
			}
			csgs[prefix + seq] = functions[f](params.split(","));
			return prefix + (seq++);
		});
		exp = exp.replace(/~(\w+)/g, (a, op1) => {
			csgs[op1] = csgs[op1] || CSG.fromPolygons(toCSGPolygons(getObjectByName(op1)));
			csgs[prefix + seq] = csgs[op1].invert();
			return prefix + (seq++);
		});
		exp = exp.replace(/(\w+)([+\-&|])(\w+)/, (a, op1, op, op2) => {
			csgs[op1] = csgs[op1] || CSG.fromPolygons(toCSGPolygons(getObjectByName(op1)));
			csgs[op2] = csgs[op2] || CSG.fromPolygons(toCSGPolygons(getObjectByName(op2)));
			if (op == "+" || op == "|") {
				csgs[prefix + seq] = csgs[op1].union(csgs[op2]);
			} else if (op == "-") {
				csgs[prefix + seq] = csgs[op2].polygons.length > 0 ? csgs[op1].subtract(csgs[op2]) : csgs[op1];
			} else if (op == "&") {
				csgs[prefix + seq] = csgs[op1].intersect(csgs[op2]);
			}
			return prefix + (seq++);
		});
	} while (exp != lastexp);

	if (csgs[exp]) {
		// clone object without faces.
		var dst = o; // o.clone();
		dst.verts.forEach((v, i) => { delete dst.verts[i] });
		dst.compact();
		csgToObject(dst, csgs[exp], true, true);

		// swap
		// console.log("replace: " + o.name);
		// document.objects.remove(o);
		// document.objects.append(dst);
	} else {
		alert(exp);
	}
});
console.log("finished");

