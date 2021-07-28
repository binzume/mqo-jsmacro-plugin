"use strict";

// csg.js : https://evanw.github.io/csg.js/
module.include("./csg.js");

var geom = module.require("geom");

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

function veq(v0, v1) {
	return v1.pos.minus(v0.pos).length() < epsilon;
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
	let dummy = new CSG.Vector(0, 0, 0);
	return obj.faces.reduce((acc, f) => {
		if (!f || f.points.length < 3) return acc;
		let points = f.points.map((i) => { return new CSG.Vector(obj.verts[i]) });
		if (isConvexPolygon(points)) {
			let vs = points.map((p) => { return new CSG.Vertex(p, dummy); }).reverse();
			let p = new CSG.Polygon(vs, [f, obj]);
			if (p.plane.w == p.plane.w) {
				acc.push(p);
			}
		} else {
			let trpoints = document.triangulate(points).map((i) => { return points[i] });
			for (let i = 0; i < trpoints.length / 3; i++) {
				let vs = trpoints.slice(i * 3, i * 3 + 3).map((p) => { return new CSG.Vertex(p, dummy); }).reverse();
				let p = new CSG.Polygon(vs, [f, obj]);
				if (p.plane.w == p.plane.w) {
					acc.push(p);
				}
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
	do {
		var f = false;
		var pp = new Map();
		for (let k = 0; k < vv.length; k++) {
			let id = vv[k].pos.toString();
			if (pp.get(id)) {
				let kk = pp.get(id);
				if (veq(vv[k - 1], vv[kk + 1]) && veq(vv[(k + 1) % vv.length], vv[(kk + vv.length - 1) % vv.length])) {
					// TODO: check intersect
					console.log("remove:" + id);
					vv.splice(k, 1);
					vv.splice(kk, 1);
					f = true;
					break;
				}
			} else {
				pp.set(id, k);
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
	} else if (t > CSG.Plane.PSILON) {
		return getVerts(csg.front, pos);
	}
	let verts = getVerts(csg.back, pos).concat(getVerts(csg.front, pos));
	const threshold = CSG.Plane.EPSILON * CSG.Plane.EPSILON * 100;
	csg.polygons.forEach(p => {
		p.vertices.forEach(v => {
			var d = v.pos.minus(pos);
			if (d.dot(d) < threshold) {
				verts.push(v);
			}
		});
	});
	return verts;
}

function getVertIndex2(pos, obj, csgNode, map) {
	let i = map.get(pos);
	if (i === undefined) {
		i = obj.verts.append(pos);
		getVerts(csgNode, pos).forEach(v => map.set(v.pos, i));
	}
	return i;
}


function getVertIndex(pos, obj, csgNode, verts) {
	const threshold = CSG.Plane.EPSILON * CSG.Plane.EPSILON * 100;
	let v = verts.find(function (e) {
		var d = e[0].minus(pos);
		return d.dot(d) < threshold;
	});
	if (v !== undefined) {
		return v[1];
	}
	let i = obj.verts.append(pos);
	verts.push([pos, i]);
	return i;
}

function csgToObject(dst, csg, mergeFaces, mergeVerts) {
	let polygons = csg.clone().polygons;
	if (mergeFaces) {
		polygons = mergePolygons(polygons);
	}
	if (mergeVerts) {
		let vv = [];
		let csgNode = null; // new CSG.Node(polygons);
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

function transformPolygons(polygons, mat) {
	let n = new CSG.Vector(0, 0, 0);
	return polygons.map((p) => {
		return new CSG.Polygon(p.vertices.map((v) => {
			return new CSG.Vertex(new CSG.Vector(mat.transform(v.pos)), n);
		}), p.shared);
	});
}

var functions = {};
var methods = {};

functions["sphere"] = function (p) {
	return CSG.sphere({});
}

functions["cube"] = function (p) {
	return CSG.cube({ radius: 0.5 });
}

functions["box"] = function (p) {
	let polygons = CSG.cube({ radius: 0.5 }).polygons;
	let mat = geom.Matrix.scaleMatrix(p[0] * 1, p[1] * 1, p[2] * 1);
	return CSG.fromPolygons(transformPolygons(polygons, mat));
}

functions["cyl"] = function (p) { // [r, h, segments]
	let l = p[1] || 1;
	return CSG.cylinder({ slices: (p[2] || 32) | 0, radius: (p[0] || 1) * 1, start: [0, -l * 0.5, 0], end: [0, l * 0.5, 0] });
}

functions["cylinder"] = function (p) {
	return CSG.cylinder({ slices: (p[1] || 32) | 0, radius: (p[0] || 1) * 1 });
}

methods["scale"] = function (polygons, p) {
	let mat = geom.Matrix.scaleMatrix(p[0] * 1, p[1] * 1, p[2] * 1);
	return CSG.fromPolygons(transformPolygons(polygons, mat));
}

methods["mv"] = function (polygons, p) {
	let mat = geom.Matrix.translateMatrix(p[0] * 1, p[1] * 1, p[2] * 1);
	return CSG.fromPolygons(transformPolygons(polygons, mat));
}

methods["tr"] = function (polygons, p) {
	let mat = geom.Matrix.translateMatrix(p[0] * 1, p[1] * 1, p[2] * 1);
	return CSG.fromPolygons(transformPolygons(polygons, mat));
}

methods["rotate"] = function (polygons, p) {
	let mat = geom.Matrix.rotateMatrix(p[0] * 1, p[1] * 1, p[2] * 1, p[3] * 1);
	return CSG.fromPolygons(transformPolygons(polygons, mat));
}

methods["rotateX"] = function (polygons, p) {
	let mat = geom.Matrix.rotateMatrix(1, 0, 0, p[0] * 1);
	return CSG.fromPolygons(transformPolygons(polygons, mat));
}

methods["rotateY"] = function (polygons, p) {
	let mat = geom.Matrix.rotateMatrix(0, 1, 0, p[0] * 1);
	return CSG.fromPolygons(transformPolygons(polygons, mat));
}

methods["rotateZ"] = function (polygons, p) {
	let mat = geom.Matrix.rotateMatrix(0, 0, 1, p[0] * 1);
	return CSG.fromPolygons(transformPolygons(polygons, mat));
}

let startTime = Date.now();

document.objects.forEach((o) => {
	var m = o.name.match("csg:(.*)");
	if (!m || o.locked) return;
	var exp = '(' + m[1].replace(/\s+/, '') + ')';
	console.log(exp);

	var prefix = "_tmp";
	var csgs = {}, seq = 0;
	var needclean = false;
	do {
		console.log(exp);
		var lastexp = exp;
		exp = exp.replace(/\(([0-9]+)\)/, "($1,)"); // (123) => (123,)
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
		if (exp != lastexp) {
			console.log("Contune " + exp);
			continue;
		}
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
			needclean = true;
			return prefix + (seq++);
		});
	} while (exp != lastexp);

	csgs[exp] = csgs[exp] || CSG.fromPolygons(toCSGPolygons(getObjectByName(exp)));
	if (csgs[exp]) {
		// clone object without faces.
		var dst = o; // o.clone();
		for (var i = 0; i < dst.verts.length; i++) {
			delete dst.verts[i];
		}
		dst.compact();
		csgToObject(dst, csgs[exp], needclean, needclean);

		// swap
		// console.log("replace: " + o.name);
		// document.objects.remove(o);
		// document.objects.append(dst);
	} else {
		alert(exp);
	}
});
console.log("finished (" + (Date.now() - startTime) + "ms)");

